#ifndef SYSTEM_INFO_BUILDER_H
#define SYSTEM_INFO_BUILDER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <DeviceConfig.h>
#include <NetworkManager.h>

// Forward declaration
extern class MqttManager *mqttManager;

// Build complete device system info JSON (shared between API and MQTT)
inline void buildDeviceSystemInfo(JsonDocument &doc)
{
    DeviceConfig &deviceConfig = DeviceConfig::getInstance();
    NetworkManager &networkManager = NetworkManager::getInstance();

    // Device info - directly assign String (ArduinoJson will handle it)
    doc["device_id"] = deviceConfig.getDeviceID();
    doc["mac_address"] = networkManager.getMacAddress();

    // Uptime
    unsigned long ms = millis();
    doc["uptime_ms"] = (long)ms;
    
    unsigned long seconds = ms / 1000;
    unsigned long hours = (seconds % 86400) / 3600;
    unsigned long minutes = (seconds % 3600) / 60;
    unsigned long secs = seconds % 60;
    
    char uptimeBuf[20];
    snprintf(uptimeBuf, sizeof(uptimeBuf), "%02lu:%02lu:%02lu", hours, minutes, secs);
    doc["uptime"] = uptimeBuf;

    // Free RAM
    extern int __heap_start, *__brkval;
    int v;
    doc["free_memory"] = (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);

    // Network
    doc["network_type"] = "ethernet";
    doc["ip"] = networkManager.getLocalIP();
    doc["gateway"] = networkManager.getGatewayIP();
    doc["subnet"] = networkManager.getSubnetMask();

    // Service status
    doc["api_status"] = "running";
    if (mqttManager) {
        doc["mqtt_status"] = mqttManager->getConnectionStatus();
        doc["mqtt_server"] = mqttManager->getMqttServerIP();
    } else {
        doc["mqtt_status"] = "not_initialized";
    }
}

#endif
