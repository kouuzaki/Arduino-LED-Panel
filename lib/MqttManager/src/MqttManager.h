#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Ethernet.h>
#include <PubSubClient.h>
#include <DeviceConfig.h>
#include <NetworkManager.h>

class MqttManager
{
public:
    MqttManager();
    ~MqttManager();

    bool init();
    bool connect();
    void disconnect();
    bool isConnected();
    void loop();

    bool setConfig(const String &username, const String &password, const String &server, int port = 1883);
    bool getConfig(String &username, String &password, String &server, int &port);
    void clearConfig();

    bool publish(const String &topic, const String &payload);
    bool subscribe(const String &topic);
    bool unsubscribe(const String &topic);

    String getConnectionStatus();
    String getMqttServerIP();

    // Set message handler callback
    void setMessageHandler(void (*handler)(const String &topic, const String &message));

private:
    EthernetClient ethernetClient;
    PubSubClient mqttClient;
    NetworkManager &networkManager;

    String mqtt_username;
    String mqtt_password;
    String mqtt_server;
    int mqtt_port;

    bool config_loaded;
    unsigned long lastReconnectAttempt;
    static const unsigned long RECONNECT_INTERVAL = 5000; // 5 seconds

    // Message handler callback
    void (*messageHandler)(const String &topic, const String &message);

    void loadConfig();
    void saveConfig();
    void callback(char *topic, byte *payload, unsigned int length);
    static void staticCallback(char *topic, byte *payload, unsigned int length);
    static MqttManager *instance;
};

#endif
