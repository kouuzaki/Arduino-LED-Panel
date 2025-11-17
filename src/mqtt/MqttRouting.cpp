#include "MqttRouting.h"
#include <DeviceConfig.h>
#include <MqttManager.h>
#include <NetworkManager.h>
#include <time.h>
#include "interface/system-info-builder.h"

// External references
extern MqttManager *mqttManager;

// Device ID
String device_id;

// Helper functions
namespace MqttTopics
{
    String getDeviceBaseTopic()
    {
        return device_id;
    }

    String getDeviceSystemTopic(const String &action)
    {
        return SYSTEM_BASE + "/" + device_id + "/" + action;
    }

    String getDeviceNetworkTopic(const String &action)
    {
        return NETWORK_BASE + "/" + device_id + "/" + action;
    }

    String getDeviceQRCodeTopic(const String &action)
    {
        return QRCODE_BASE + "/" + device_id + "/" + action;
    }
}

MqttRouting &MqttRouting::getInstance()
{
    static MqttRouting instance;
    return instance;
}

bool MqttRouting::init()
{
    if (initialized)
    {
        return true;
    }

    // Get device_id from DeviceConfig
    DeviceConfig &deviceConfig = DeviceConfig::getInstance();
    device_id = deviceConfig.getDeviceID();

    setupDefaultRoutes();
    initialized = true;

    Serial.println("✅ MQTT Routing initialized successfully");
    return true;
}

void MqttRouting::registerRoute(const String &topic, MqttMessageHandler handler, int qos)
{
    if (routeCount < MAX_MQTT_ROUTES)
    {
        routes[routeCount] = MqttRoute(topic, handler, qos);
        routeCount++;
        Serial.println("📋 Registered MQTT route: " + topic + " (QoS: " + String(qos) + ")");
    }
    else
    {
        Serial.println("❌ Cannot register route: max routes reached");
    }
}

void MqttRouting::setupDefaultRoutes()
{
    Serial.println("🔧 Setting up default MQTT routes...");

    // System command: system/{device_id}/cmd
    // Using static lambda wrapper to call member function
    static auto systemHandler = [](const String &topic, const String &message) {
        MqttRouting::getInstance().handleSystemCommand(topic, message);
    };
    registerRoute(MqttTopics::getDeviceSystemTopic("cmd"), systemHandler, 2);

    // Network command: network/{device_id}/cmd
    // Using static lambda wrapper to call member function
    static auto networkHandler = [](const String &topic, const String &message) {
        MqttRouting::getInstance().handleNetworkCommand(topic, message);
    };
    registerRoute(MqttTopics::getDeviceNetworkTopic("cmd"), networkHandler, 1);

    Serial.println("✅ Default MQTT routes setup completed");
    Serial.print("📋 Device ID: ");
    Serial.println(device_id);
    Serial.println("📋 Registered MQTT topics:");
    Serial.println("   🔄 System command: " + MqttTopics::getDeviceSystemTopic("cmd"));
    Serial.println("   🌐 Network command: " + MqttTopics::getDeviceNetworkTopic("cmd"));
    Serial.println("");
}

bool MqttRouting::subscribeAllRoutes()
{
    if (!mqttManager)
    {
        Serial.println("❌ Cannot subscribe: MQTT manager is null");
        return false;
    }

    bool allSubscribed = true;
    for (int i = 0; i < routeCount; i++)
    {
        if (!mqttManager->subscribe(routes[i].topic))
        {
            Serial.println("❌ Failed to subscribe to: " + routes[i].topic);
            allSubscribed = false;
        }
        else
        {
            routes[i].subscribed = true;
        }
    }

    return allSubscribed;
}

void MqttRouting::unsubscribeAllRoutes()
{
    if (!mqttManager)
        return;

    for (int i = 0; i < routeCount; i++)
    {
        if (routes[i].subscribed)
        {
            mqttManager->unsubscribe(routes[i].topic);
            routes[i].subscribed = false;
        }
    }

    Serial.println("✓ Unsubscribed from all MQTT routes");
}

void MqttRouting::handleMessage(const String &topic, const String &message)
{
    // Trim whitespace from message
    String trimmedMessage = message;
    trimmedMessage.trim();

    // Ignore empty messages
    if (trimmedMessage.length() == 0)
    {
        Serial.println("⚠️ Received empty message, ignoring");
        return;
    }

    // Parse device ID from topic if present
    String incomingDevice = "";
    int firstSlash = topic.indexOf('/');
    if (firstSlash > 0)
    {
        int secondSlash = topic.indexOf('/', firstSlash + 1);
        if (secondSlash > firstSlash)
        {
            incomingDevice = topic.substring(firstSlash + 1, secondSlash);
        }
    }

    // Validate device ID
    if (incomingDevice.length() > 0 && !isMessageForDevice(incomingDevice))
    {
        Serial.println("⚠️ Wrong device. Expected: " + device_id + ", Got: " + incomingDevice);
        return;
    }

    // Find and call matching handler
    for (int i = 0; i < routeCount; i++)
    {
        if (routes[i].topic == topic && routes[i].handler != nullptr)
        {
            routes[i].handler(topic, trimmedMessage);
            return;
        }
    }

    Serial.println("⚠️ No handler registered for topic: " + topic);
}

bool MqttRouting::isMessageForDevice(const String &deviceId)
{
    return (deviceId == device_id);
}

bool MqttRouting::publishDeviceStatus(const String &status)
{
    if (!mqttManager || !mqttManager->isConnected())
    {
        return false;
    }

    String topic = MqttTopics::getDeviceSystemTopic("status");
    return mqttManager->publish(topic, status);
}

JsonDocument MqttRouting::buildDeviceSystemInfo()
{
    JsonDocument doc;
    ::buildDeviceSystemInfo(doc); // Use global helper function (:: prefix to avoid name conflict)
    return doc;
}

void MqttRouting::handleSystemCommand(const String &topic, const String &message)
{
    Serial.println("📥 System Command received: " + message);

    // Rate limiting
    unsigned long now = millis();
    if (now - lastSystemCommand < COMMAND_COOLDOWN)
    {
        Serial.println("⚠️ System command rate limited");
        return;
    }
    lastSystemCommand = now;

    // Parse JSON command
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error)
    {
        Serial.println("❌ Failed to parse JSON: " + String(error.c_str()));
        return;
    }

    // Handle commands
    if (doc.containsKey("action"))
    {
        String action = doc["action"];
        Serial.println("   Action: " + action);

        if (action == "restart")
        {
            Serial.println("🔄 Restarting device...");
            delay(1000);
            // Arduino restart
        }
        else if (action == "status")
        {
            JsonDocument statusDoc = buildDeviceSystemInfo();
            String statusJson;
            serializeJson(statusDoc, statusJson);
            mqttManager->publish(MqttTopics::getDeviceSystemTopic("status"), statusJson);
        }
    }
}

void MqttRouting::handleNetworkCommand(const String &topic, const String &message)
{
    Serial.println("📥 Network Command received: " + message);

    // Rate limiting
    unsigned long now = millis();
    if (now - lastNetworkCommand < COMMAND_COOLDOWN)
    {
        Serial.println("⚠️ Network command rate limited");
        return;
    }
    lastNetworkCommand = now;

    // Parse JSON command
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error)
    {
        Serial.println("❌ Failed to parse JSON: " + String(error.c_str()));
        return;
    }

    if (doc.containsKey("action"))
    {
        String action = doc["action"];
        Serial.println("   Action: " + action);

        if (action == "status")
        {
            JsonDocument networkInfo;
            networkInfo["network"] = NetworkManager::getInstance().getNetworkInfoJson();
            String networkJson;
            serializeJson(networkInfo, networkJson);
            mqttManager->publish(MqttTopics::getDeviceNetworkTopic("status"), networkJson);
        }
    }
}

void MqttRouting::handleQRCodeCommand(const String &topic, const String &message)
{
    Serial.println("📥 QR Code Command received: " + message);
}

void MqttRouting::handleResponse(const String &topic, const String &message)
{
    Serial.println("📥 Response received from: " + topic);
    lastResponse = message;
}

bool MqttRouting::publishToTopic(const String &topic, const String &payload, bool retained, int qos)
{
    if (!mqttManager || !mqttManager->isConnected())
    {
        Serial.println("❌ Cannot publish: MQTT not connected");
        return false;
    }

    return mqttManager->publish(topic, payload);
}
