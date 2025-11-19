#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <Ethernet.h>
#include <DeviceSystemInfo.h>

class MqttHandler
{
private:
    PubSubClient &client;
    unsigned long lastHeartbeat;
    unsigned long lastReconnectAttempt;
    const unsigned long HEARTBEAT_INTERVAL = 30000;    // 30 seconds
    const unsigned long RECONNECT_INTERVAL = 10000;    // 10 seconds

    char cmdTopic[48];
    char infoTopic[48];
    char statusTopic[48];
    const char *device_name;

public:
    MqttHandler(PubSubClient &mqttClient, const char *name)
        : client(mqttClient), device_name(name), lastHeartbeat(0), lastReconnectAttempt(0)
    {
        snprintf(cmdTopic, sizeof(cmdTopic), "device/%s/cmd", device_name);
        snprintf(infoTopic, sizeof(infoTopic), "device/%s/info", device_name);
        snprintf(statusTopic, sizeof(statusTopic), "device/%s/status", device_name);
    }

    void connect(const char *server, uint16_t port, const char *username, const char *password)
    {
        client.setServer(server, port);
        lastReconnectAttempt = 0;
    }

    void update()
    {
        // Reconnect if needed
        if (!client.connected())
        {
            unsigned long now = millis();
            if (now - lastReconnectAttempt > RECONNECT_INTERVAL)
            {
                lastReconnectAttempt = now;
                reconnect();
            }
        }
        else
        {
            client.loop();

            // Send heartbeat
            unsigned long now = millis();
            if (now - lastHeartbeat > HEARTBEAT_INTERVAL)
            {
                lastHeartbeat = now;
                publishHeartbeat();
            }
        }
    }

    bool isConnected()
    {
        return client.connected();
    }

    bool publish(const char *topic, const char *payload)
    {
        if (!client.connected())
            return false;
        return client.publish(topic, payload);
    }

    bool subscribe(const char *topic)
    {
        if (!client.connected())
            return false;
        return client.subscribe(topic);
    }

    const char *getStatusTopic() { return statusTopic; }
    const char *getInfoTopic() { return infoTopic; }
    const char *getCmdTopic() { return cmdTopic; }

private:
    void reconnect()
    {
        if (client.connect(device_name))
        {
            client.publish(statusTopic, "online", true);
            client.subscribe(cmdTopic);
        }
    }

    void publishHeartbeat()
    {
        // Use compact JSON for MQTT heartbeat - optimized
        char buffer[128];
        buildMqttCompactJson(buffer, sizeof(buffer));
        
        client.publish(infoTopic, buffer);
    }
};

#endif
