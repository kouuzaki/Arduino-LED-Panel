#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <Ethernet.h>
#include <ArduinoJson.h>

// Tipe data untuk callback function pointer
typedef void (*ReconnectCallback)();

class MqttManager
{
private:
    PubSubClient &client;
    const char *device_name;

    // Auth Storage
    String mqtt_user;
    String mqtt_pass;
    bool has_auth;
    IPAddress server_ip;

    // Timers
    unsigned long lastHeartbeat;
    unsigned long lastReconnectAttempt;
    unsigned long heartbeatInterval;                        // Configurable heartbeat interval in ms
    const unsigned long RECONNECT_INTERVAL = 5000;          // 5 Detik
    const unsigned long DEFAULT_HEARTBEAT_INTERVAL = 10000; // 10 Seconds default

    // Topics
    char infoTopic[64];

    // Callback saat reconnect berhasil (untuk resubscribe)
    ReconnectCallback onReconnectCb;

public:
    MqttManager(PubSubClient &mqttClient, const char *name);

    // Setup awal
    void begin(IPAddress host, uint16_t port, const char *user = nullptr, const char *pass = nullptr);

    // Update Device ID dynamically
    void setDeviceId(const char *newId);

    // Loop utama (panggil di main loop)
    void update();

    // Set callback agar main program tahu kapan harus resubscribe topic
    void setReconnectCallback(ReconnectCallback cb);

    // Set heartbeat interval dynamically (in milliseconds)
    void setHeartbeatInterval(unsigned long intervalMs);

    bool isConnected();
    bool publish(const char *topic, const char *payload, bool retained = false);

    String getMqttServerIP() const
    {
        return String(server_ip[0]) + "." + String(server_ip[1]) + "." +
               String(server_ip[2]) + "." + String(server_ip[3]);
    }
    String getConnectionStatus() { return client.connected() ? "connected" : "disconnected"; }

private:
    bool connect();
    void publishHeartbeat();
};

#endif