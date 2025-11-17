#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <PubSubClient.h>
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
        // Build JSON manually untuk hemat RAM
        unsigned long ms = millis();
        unsigned long seconds = ms / 1000;
        unsigned long hours = (seconds % 86400) / 3600;
        unsigned long minutes = (seconds % 3600) / 60;
        unsigned long secs = seconds % 60;
        
        // Calculate free RAM
        extern int __heap_start, *__brkval;
        int v;
        int freeRam = (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
        
        // IP Address
        IPAddress ip = Ethernet.localIP();
        
        // Compact JSON for MQTT heartbeat
        String json = "{";
        json += "\"device_id\":\"iot_led_panel\",";
        json += "\"ip\":\"" + String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]) + "\",";
        json += "\"uptime_ms\":" + String(ms) + ",";
        json += "\"free_memory\":" + String(freeRam) + "";
        json += "}";
        
        client.publish(infoTopic, json.c_str());
    }
};

#endif
