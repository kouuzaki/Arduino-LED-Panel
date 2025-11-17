#ifndef MQTT_ROUTING_H
#define MQTT_ROUTING_H

#include <Arduino.h>
#include <ArduinoJson.h>

// Topic structure constants
namespace MqttTopics
{
    // Device-specific base topic pattern: category/device_id/action
    String getDeviceSystemTopic(const String &action);
    String getDeviceNetworkTopic(const String &action);
    String getDeviceQRCodeTopic(const String &action);
    String getDeviceBaseTopic();

    // System topics
    const String SYSTEM_BASE = "system";

    // Network topics
    const String NETWORK_BASE = "network";

    // QR Code topics
    const String QRCODE_BASE = "qrcode";
}

// Message handler function pointer type
typedef void (*MqttMessageHandler)(const String &topic, const String &message);

// Maximum routes (reduced from 10 to 5 to save memory)
#define MAX_MQTT_ROUTES 5

// Topic route structure
struct MqttRoute
{
    String topic;
    MqttMessageHandler handler;
    bool subscribed;
    int qos;

    MqttRoute(const String &t = "", MqttMessageHandler h = nullptr, int q = 0)
        : topic(t), handler(h), subscribed(false), qos(q) {}
};

class MqttRouting
{
public:
    static MqttRouting &getInstance();

    // Initialize routing system
    bool init();

    // Register route handlers
    void registerRoute(const String &topic, MqttMessageHandler handler, int qos = 0);

    // Setup default routes
    void setupDefaultRoutes();

    // Subscribe to all registered routes
    bool subscribeAllRoutes();

    // Unsubscribe from all routes
    void unsubscribeAllRoutes();

    // Handle incoming message with device validation
    void handleMessage(const String &topic, const String &message);

    // Validate if message is for this device
    bool isMessageForDevice(const String &deviceId);

    // Publish structured messages
    bool publishDeviceStatus(const String &status);

    // Get last received response payload (for debugging / local inspection)
    String getLastResponse() const { return lastResponse; }

    // Build complete device system info JSON (shared between API and MQTT)
    JsonDocument buildDeviceSystemInfo();

private:
    MqttRouting() = default;
    ~MqttRouting() = default;
    MqttRouting(const MqttRouting &) = delete;
    MqttRouting &operator=(const MqttRouting &) = delete;

    MqttRoute routes[MAX_MQTT_ROUTES];
    int routeCount = 0;
    bool initialized = false;

    // Rate limiting for all commands (3 seconds cooldown)
    unsigned long lastSystemCommand = 0;
    unsigned long lastNetworkCommand = 0;
    unsigned long lastQRCodeCommand = 0;
    static const unsigned long COMMAND_COOLDOWN = 3000; // 3 seconds between commands

    // Command handlers
    void handleSystemCommand(const String &topic, const String &message);
    void handleNetworkCommand(const String &topic, const String &message);
    void handleQRCodeCommand(const String &topic, const String &message);
    void handleResponse(const String &topic, const String &message);

    // Utility functions
    bool publishToTopic(const String &topic, const String &payload, bool retained = false, int qos = 0);
    // Store last received response (JSON string)
    String lastResponse;
};

#endif
