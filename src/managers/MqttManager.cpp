#include "MqttManager.h"
#include "DeviceSystemInfo.h" // Mengambil data sistem untuk heartbeat

MqttManager::MqttManager(PubSubClient &mqttClient, const char *name)
    : client(mqttClient), device_name(name),
      lastHeartbeat(0), lastReconnectAttempt(0), has_auth(false), onReconnectCb(nullptr)
{
    // Topic sistem: device/nama/info & device/nama/status
    snprintf(infoTopic, sizeof(infoTopic), "device/%s/info", device_name);
    snprintf(statusTopic, sizeof(statusTopic), "device/%s/status", device_name);
}

void MqttManager::begin(IPAddress host, uint16_t port, const char *user, const char *pass)
{
    Serial.print("MQTT: Init Manager -> ");
    Serial.print(host);
    Serial.print(":");
    Serial.println(port);

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
}

void MqttManager::setReconnectCallback(ReconnectCallback cb)
{
    onReconnectCb = cb;
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
        // Publish status online (Retained)
        client.publish(statusTopic, "online", true);

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
        if (now - lastHeartbeat > HEARTBEAT_INTERVAL)
        {
            lastHeartbeat = now;
            publishHeartbeat();
        }
    }
}

void MqttManager::publishHeartbeat()
{
    char buffer[200];
    // Menggunakan fungsi central dari DeviceSystemInfo.h
    SystemInfo::buildHeartbeatJSON(buffer, sizeof(buffer));

    client.publish(infoTopic, buffer);
}

bool MqttManager::isConnected() { return client.connected(); }

bool MqttManager::publish(const char *topic, const char *payload, bool retained)
{
    if (!client.connected())
        return false;
    return client.publish(topic, payload, retained);
}