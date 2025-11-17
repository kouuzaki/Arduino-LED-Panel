#include "MqttManager.h"
#include <DeviceConfig.h>

MqttManager *MqttManager::instance = nullptr;

MqttManager::MqttManager()
    : mqttClient(ethernetClient), networkManager(NetworkManager::getInstance()),
      config_loaded(false), lastReconnectAttempt(0), mqtt_port(1883), messageHandler(nullptr)
{
    instance = this;

    // Optimize MQTT client settings for better throughput
    mqttClient.setCallback(staticCallback);
    mqttClient.setKeepAlive(60);     // Keep alive every 60 seconds
    mqttClient.setSocketTimeout(10); // Socket timeout 10 seconds

    // Increase buffer size for better message handling (default is 256)
    mqttClient.setBufferSize(1024); // Increase to 1KB for larger messages

    Serial.println("🔧 MQTT Client optimized with 1KB buffer, 60s keepalive, 10s timeout");
}

MqttManager::~MqttManager()
{
    disconnect();
    instance = nullptr;
}

bool MqttManager::init()
{
    // Load config from DeviceConfig (which uses EEPROM)
    loadConfig();
    Serial.println("✓ MQTT Manager initialized successfully");
    return true;
}

void MqttManager::loadConfig()
{
    // Load from DeviceConfig singleton (EEPROM-backed)
    DeviceConfig &deviceConfig = DeviceConfig::getInstance();

    mqtt_username = deviceConfig.getMqttUsername();
    mqtt_password = deviceConfig.getMqttPassword();
    mqtt_server = deviceConfig.getMqttServer();
    mqtt_port = deviceConfig.getMqttPort();

    config_loaded = mqtt_server.length() > 0;

    if (config_loaded)
    {
        Serial.println("✓ MQTT config loaded from EEPROM");
        Serial.println("Server: " + mqtt_server + ":" + String(mqtt_port));
        Serial.println("Username: " + mqtt_username);
    }
    else
    {
        Serial.println("⚠ No MQTT config found");
    }
}

void MqttManager::saveConfig()
{
    // Save config via DeviceConfig (EEPROM-backed)
    DeviceConfig &deviceConfig = DeviceConfig::getInstance();

    deviceConfig.setMqttUsername(mqtt_username);
    deviceConfig.setMqttPassword(mqtt_password);
    deviceConfig.setMqttServer(mqtt_server);
    deviceConfig.setMqttPort(mqtt_port);
    deviceConfig.saveConfig();

    Serial.println("✓ MQTT config saved to EEPROM");
}

bool MqttManager::setConfig(const String &username, const String &password, const String &server, int port)
{
    mqtt_username = username;
    mqtt_password = password;
    mqtt_server = server;
    mqtt_port = port;

    saveConfig();
    config_loaded = true;

    // Disconnect if currently connected to apply new config
    if (isConnected())
    {
        disconnect();
    }

    // Set server and port
    mqttClient.setServer(mqtt_server.c_str(), mqtt_port);

    Serial.println("✓ MQTT config updated");
    return true;
}

bool MqttManager::getConfig(String &username, String &password, String &server, int &port)
{
    if (!config_loaded)
    {
        return false;
    }

    username = mqtt_username;
    password = mqtt_password;
    server = mqtt_server;
    port = mqtt_port;
    return true;
}

void MqttManager::clearConfig()
{
    mqtt_username = "";
    mqtt_password = "";
    mqtt_server = "";
    mqtt_port = 1883;
    config_loaded = false;

    disconnect();

    // Clear from DeviceConfig
    DeviceConfig &deviceConfig = DeviceConfig::getInstance();
    deviceConfig.setMqttUsername("");
    deviceConfig.setMqttPassword("");
    deviceConfig.setMqttServer("");
    deviceConfig.setMqttPort(1883);
    deviceConfig.saveConfig();

    Serial.println("✓ MQTT config cleared");
}

bool MqttManager::connect()
{
    if (!config_loaded)
    {
        Serial.println("❌ No MQTT config available");
        return false;
    }

    if (!networkManager.isConnected())
    {
        Serial.println("❌ Network not connected for MQTT");
        return false;
    }

    if (isConnected())
    {
        return true;
    }

    // Set server if not already set
    mqttClient.setServer(mqtt_server.c_str(), mqtt_port);

    // Generate client ID based on network type and MAC
    String networkType = "ETH"; // Always Ethernet for Arduino UNO
    String clientId = "ARDUINO_UNO_" + networkType + "_" + networkManager.getMacAddress();
    clientId.replace(":", "");

    Serial.print("🔌 Connecting to MQTT broker: " + mqtt_server + ":" + String(mqtt_port) + "...");

    bool connected = false;
    if (mqtt_username.length() > 0)
    {
        // Connect with username and password
        connected = mqttClient.connect(clientId.c_str(), mqtt_username.c_str(), mqtt_password.c_str());
    }
    else
    {
        // Connect without credentials
        connected = mqttClient.connect(clientId.c_str());
    }

    if (connected)
    {
        Serial.println(" ✓ Connected!");
        lastReconnectAttempt = 0;
        return true;
    }
    else
    {
        Serial.println(" ❌ Failed! Error code: " + String(mqttClient.state()));
        return false;
    }
}

void MqttManager::disconnect()
{
    if (mqttClient.connected())
    {
        mqttClient.disconnect();
        Serial.println("✓ Disconnected from MQTT broker");
    }
}

bool MqttManager::isConnected()
{
    return mqttClient.connected();
}

void MqttManager::loop()
{
    if (!config_loaded)
    {
        return;
    }

    // Check if we still have network connection
    if (!networkManager.isConnected())
    {
        if (isConnected())
        {
            disconnect();
            Serial.println("⚠ Lost network connection, disconnected from MQTT");
        }
        return;
    }

    if (mqttClient.connected())
    {
        mqttClient.loop();
    }
    else
    {
        // Try to reconnect if enough time has passed
        unsigned long now = millis();
        if (now - lastReconnectAttempt > RECONNECT_INTERVAL)
        {
            lastReconnectAttempt = now;
            Serial.println("🔄 Attempting MQTT reconnection...");
            connect();
        }
    }
}

bool MqttManager::publish(const String &topic, const String &payload)
{
    if (!isConnected())
    {
        Serial.println("❌ Cannot publish: MQTT not connected");
        return false;
    }

    bool result = mqttClient.publish(topic.c_str(), payload.c_str());
    if (result)
    {
        Serial.println("✓ Published to " + topic + ": " + payload);
    }
    else
    {
        Serial.println("❌ Failed to publish to " + topic);
    }
    return result;
}

bool MqttManager::subscribe(const String &topic)
{
    if (!isConnected())
    {
        Serial.println("❌ Cannot subscribe: MQTT not connected");
        return false;
    }

    bool result = mqttClient.subscribe(topic.c_str());
    if (result)
    {
        Serial.println("✓ Subscribed to " + topic);
    }
    else
    {
        Serial.println("❌ Failed to subscribe to " + topic);
    }
    return result;
}

bool MqttManager::unsubscribe(const String &topic)
{
    if (!isConnected())
    {
        Serial.println("❌ Cannot unsubscribe: MQTT not connected");
        return false;
    }

    bool result = mqttClient.unsubscribe(topic.c_str());
    if (result)
    {
        Serial.println("✓ Unsubscribed from " + topic);
    }
    else
    {
        Serial.println("❌ Failed to unsubscribe from " + topic);
    }
    return result;
}

String MqttManager::getConnectionStatus()
{
    if (!config_loaded)
    {
        return "not_configured";
    }

    if (!networkManager.isConnected())
    {
        return "no_network";
    }

    if (isConnected())
    {
        return "connected";
    }
    else
    {
        switch (mqttClient.state())
        {
        case MQTT_CONNECTION_TIMEOUT:
            return "timeout";
        case MQTT_CONNECTION_LOST:
            return "connection_lost";
        case MQTT_CONNECT_FAILED:
            return "connect_failed";
        case MQTT_DISCONNECTED:
            return "disconnected";
        case MQTT_CONNECT_BAD_PROTOCOL:
            return "bad_protocol";
        case MQTT_CONNECT_BAD_CLIENT_ID:
            return "bad_client_id";
        case MQTT_CONNECT_UNAVAILABLE:
            return "unavailable";
        case MQTT_CONNECT_BAD_CREDENTIALS:
            return "bad_credentials";
        case MQTT_CONNECT_UNAUTHORIZED:
            return "unauthorized";
        default:
            return "unknown_error";
        }
    }
}

String MqttManager::getMqttServerIP()
{
    return mqtt_server;
}

void MqttManager::setMessageHandler(void (*handler)(const String &topic, const String &message))
{
    messageHandler = handler;
}

void MqttManager::callback(char *topic, byte *payload, unsigned int length)
{
    String message = "";
    for (unsigned int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }

    Serial.println("📥 MQTT Message received [" + String(topic) + "]: " + message);

    // Call message handler if set
    if (messageHandler)
    {
        messageHandler(String(topic), message);
    }
}

void MqttManager::staticCallback(char *topic, byte *payload, unsigned int length)
{
    if (instance)
    {
        instance->callback(topic, payload, length);
    }
}
