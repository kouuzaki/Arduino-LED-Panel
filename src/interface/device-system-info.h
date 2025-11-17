#ifndef DEVICE_SYSTEM_INFO_H
#define DEVICE_SYSTEM_INFO_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <DeviceConfig.h>
#include <NetworkManager.h>
#include <time.h>

// Forward declaration
extern class MqttManager *mqttManager;

// Helper function to build complete device system info JSON
// This is shared between API (ApiRouting) and MQTT (MqttRouting) to ensure consistency
inline JsonDocument buildDeviceSystemInfoJson()
{
    JsonDocument doc;
    DeviceConfig &deviceConfig = DeviceConfig::getInstance();
    NetworkManager &networkManager = NetworkManager::getInstance();

    // Device info
    doc["device_id"] = deviceConfig.getDeviceID();
    doc["mac_address"] = networkManager.getMacAddress();

    // Format uptime as: elapsed time
    unsigned long ms = millis();
    unsigned long seconds = ms / 1000;
    unsigned long hours = (seconds % 86400) / 3600;
    unsigned long minutes = (seconds % 3600) / 60;
    unsigned long secs = seconds % 60;
    char buf[24];
    snprintf(buf, sizeof(buf), "0000-00-00 %02lu:%02lu:%02lu", hours, minutes, secs);
    doc["uptime"] = String(buf);
    doc["uptime_ms"] = (long)ms;

    // Estimate free RAM for Arduino UNO (basic calculation)
    extern int __heap_start, *__brkval;
    int v;
    int freeRam = (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
    doc["free_memory"] = String(freeRam) + " bytes";

    // Network information - now get as JSON directly
    JsonDocument networkDoc = networkManager.getNetworkInfoJson();
    doc["network"] = networkDoc.as<JsonObject>();

    if (mqttManager)
    {
        doc["mqtt_server_ip"] = mqttManager->getMqttServerIP();
    }

    // Service status
    JsonObject services = doc["services"].to<JsonObject>();
    services["api"] = "running";
    services["mqtt"] = mqttManager ? mqttManager->getConnectionStatus() : "not_initialized";

    return doc;
}

#endif
