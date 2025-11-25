#include "MqttManager.h"
#include "../interface/DeviceSystemInfo.h"
#include "../storage/FileStorage.h"

MqttManager::MqttManager(PubSubClient &mqttClient, const char *name)
    : client(mqttClient), device_name(name),
      lastHeartbeat(0), lastReconnectAttempt(0), has_auth(false), onReconnectCb(nullptr),
      heartbeatInterval(DEFAULT_HEARTBEAT_INTERVAL)
{
    // Topic sistem: system/nama/info
    snprintf(infoTopic, sizeof(infoTopic), "system/%s/info", device_name);
}

void MqttManager::begin(IPAddress host, uint16_t port, const char *user, const char *pass)
{
    Serial.print("MQTT: Init Manager -> ");
    Serial.print(host);
    Serial.print(":");
    Serial.println(port);

    server_ip = host;
    client.setServer(host, port);

    if (user && strlen(user) > 0)
    {
        mqtt_user = String(user);
        mqtt_pass = String(pass);
        has_auth = true;
    }
    else
    {
        has_auth = false;
    }

    // Log default heartbeat interval
    Serial.print("MQTT: Heartbeat interval set to ");
    Serial.print(heartbeatInterval);
    Serial.println(" ms (default)");
}

void MqttManager::setDeviceId(const char *newId)
{
    device_name = newId;
    // Re-generate topics
    snprintf(infoTopic, sizeof(infoTopic), "system/%s/info", device_name);
}

void MqttManager::setReconnectCallback(ReconnectCallback cb)
{
    onReconnectCb = cb;
}

void MqttManager::setHeartbeatInterval(unsigned long intervalMs)
{
    // Allow heartbeat intervals from 1 ms up to 24 hours (in milliseconds)
    // 24 hours = 86_400_000 ms which fits in 32-bit unsigned long
    const unsigned long MAX_HEARTBEAT_MS = 86400000UL; // 24h

    if (intervalMs > 0 && intervalMs <= MAX_HEARTBEAT_MS)
    {
        heartbeatInterval = intervalMs;
        Serial.print("MQTT: Heartbeat interval updated to ");
        Serial.print(intervalMs);
        Serial.println(" ms");
        // Persist new heartbeat interval to storage so it's retained across reboots
        // Use a reasonably sized StaticJsonDocument to avoid dynamic allocation pressure
        StaticJsonDocument<1024> cfg;
        if (!FileStorage::loadDeviceConfig(cfg))
        {
            // start with empty object
            cfg.clear();
        }
        cfg["mqtt_heartbeat_interval"] = intervalMs;
        if (FileStorage::saveDeviceConfig(cfg))
        {
            Serial.println("MQTT: Heartbeat interval saved to storage");
        }
        else
        {
            Serial.println("MQTT: Failed to save heartbeat interval to storage");
        }
    }
    else
    {
        Serial.print("MQTT: Heartbeat interval rejected (must be 1..86400000 ms): ");
        Serial.println(intervalMs);
    }
}

bool MqttManager::connect()
{
    Serial.print("MQTT: Connecting... ");
    bool ok;

    if (has_auth)
    {
        ok = client.connect(device_name, mqtt_user.c_str(), mqtt_pass.c_str());
    }
    else
    {
        ok = client.connect(device_name);
    }

    if (ok)
    {
        Serial.println("OK");
        // Kirim heartbeat pertama segera
        publishHeartbeat();

        // Panggil callback eksternal untuk resubscribe topic (Display, dll)
        if (onReconnectCb)
        {
            onReconnectCb();
        }
    }
    else
    {
        Serial.print("Fail rc=");
        Serial.println(client.state());
    }
    return ok;
}

void MqttManager::update()
{
    // 1. Handle Reconnect
    if (!client.connected())
    {
        unsigned long now = millis();
        if (now - lastReconnectAttempt > RECONNECT_INTERVAL)
        {
            lastReconnectAttempt = now;
            if (connect())
            {
                lastReconnectAttempt = 0;
            }
        }
    }
    else
    {
        // 2. Client Loop
        client.loop();

        // 3. Heartbeat Loop
        unsigned long now = millis();
        if (now - lastHeartbeat > heartbeatInterval)
        {
            lastHeartbeat = now;
            publishHeartbeat();
        }
    }
}

void MqttManager::publishHeartbeat()
{
    // Build full API response (same as REST API)
    JsonDocument response;
    SystemInfo::buildFullApiResponse(response);

    // Serialize to string
    char buffer[1024];
    size_t len = serializeJson(response, buffer, sizeof(buffer));

    bool ok = client.publish(infoTopic, buffer, 1); // QoS 1

    // Log heartbeat with timestamp
    Serial.print("MQTT: [");
    Serial.print(millis());
    Serial.print("ms] Heartbeat (");
    Serial.print(len);
    Serial.print(" bytes) -> ");
    Serial.print(ok ? "OK" : "FAILED");
    Serial.println();
}

bool MqttManager::isConnected() { return client.connected(); }

bool MqttManager::publish(const char *topic, const char *payload, bool retained)
{
    if (!client.connected())
        return false;
    return client.publish(topic, payload, retained, 1); // retained, QoS 1
}