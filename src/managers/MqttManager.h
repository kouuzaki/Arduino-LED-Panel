#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <Ethernet.h>

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

    // Timers
    unsigned long lastHeartbeat;
    unsigned long lastReconnectAttempt;
    const unsigned long HEARTBEAT_INTERVAL = 30000; // 30 Detik
    const unsigned long RECONNECT_INTERVAL = 5000;  // 5 Detik

    // Topics
    char infoTopic[64];
    char statusTopic[64];

    // Callback saat reconnect berhasil (untuk resubscribe)
    ReconnectCallback onReconnectCb;

public:
    MqttManager(PubSubClient &mqttClient, const char *name);

    // Setup awal
    void begin(IPAddress host, uint16_t port, const char *user = nullptr, const char *pass = nullptr);

    // Loop utama (panggil di main loop)
    void update();

    // Set callback agar main program tahu kapan harus resubscribe topic
    void setReconnectCallback(ReconnectCallback cb);

    bool isConnected();
    bool publish(const char *topic, const char *payload, bool retained = false);

private:
    bool connect();
    void publishHeartbeat();
};

#endif