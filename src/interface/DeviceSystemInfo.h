#ifndef DEVICE_SYSTEM_INFO_H
#define DEVICE_SYSTEM_INFO_H

#include <Arduino.h>
#include <Ethernet.h>
#include <ArduinoJson.h>
#include "storage/FileStorage.h"
#include "managers/MqttManager.h"

// __heap_start and __brkval are provided by avr-libc; declare them in global namespace
extern int __heap_start;
extern int *__brkval;

// External references to global objects
extern MqttManager mqttManager;
extern byte mac[];

namespace SystemInfo
{

    // Helper: Hitung Free RAM (AVR specific)
    inline int getFreeMemory()
    {
        int v;
        return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
    }

    // Helper: Format Uptime ke "HH:MM:SS"
    inline void getUptimeString(char *buffer, size_t size)
    {
        unsigned long seconds = millis() / 1000;
        unsigned long hours = (seconds % 86400) / 3600;
        unsigned long minutes = (seconds % 3600) / 60;
        unsigned long secs = seconds % 60;
        snprintf(buffer, size, "%02lu:%02lu:%02lu", hours, minutes, secs);
    }

    // Helper: Get Device ID from Storage
    inline void getStoredDeviceId(char *buffer, size_t size)
    {
        strlcpy(buffer, "led_lilygo", size); // Default from user request
        JsonDocument doc;
        if (FileStorage::loadDeviceConfig(doc))
        {
            if (doc.containsKey("device_id"))
            {
                strlcpy(buffer, doc["device_id"], size);
            }
        }
    }

    // Helper: Convert IPAddress to String
    inline String ipToString(IPAddress ip)
    {
        return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
    }

    // 1. JSON Compact (Untuk MQTT Heartbeat) - Hemat Data
    inline void buildHeartbeatJSON(char *buffer, size_t bufferSize)
    {
        IPAddress ip = Ethernet.localIP();
        int freeRam = getFreeMemory();
        unsigned long uptime = millis();

        char devId[32];
        getStoredDeviceId(devId, sizeof(devId));

        snprintf(buffer, bufferSize,
                 "{\"device_id\":\"%s\",\"ip\":\"%d.%d.%d.%d\",\"uptime_ms\":%lu,\"free_mem\":%d}",
                 devId, ip[0], ip[1], ip[2], ip[3], uptime, freeRam);
    }

    // 2. JSON Full (Untuk REST API) - Detail Lengkap
    // Builds device info data object
    // Note: On Mega 2560, be careful with stack usage.
    inline void buildDeviceInfoData(JsonObject &dataObj)
    {
        char devId[32];
        getStoredDeviceId(devId, sizeof(devId));
        dataObj["device_id"] = devId;

        // MAC Address
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        dataObj["mac_address"] = macStr;

        // Uptime in "YYYY-MM-DD HH:mm:ss" format (fallback to 0000-00-00)
        char uptimeStr[20];
        unsigned long ms = millis();
        unsigned long seconds = ms / 1000;
        unsigned long hours = (seconds % 86400) / 3600;
        unsigned long minutes = (seconds % 3600) / 60;
        unsigned long secs = seconds % 60;
        snprintf(uptimeStr, sizeof(uptimeStr), "0000-00-00 %02lu:%02lu:%02lu", hours, minutes, secs);
        dataObj["uptime"] = uptimeStr;

        // Free Memory in KB
        char memStr[16];
        snprintf(memStr, sizeof(memStr), "%d KB", getFreeMemory() / 1024);
        dataObj["free_memory"] = memStr;

        // Network information
        JsonObject network = dataObj["network"].to<JsonObject>();
        network["type"] = "ethernet";
        network["status"] = mqttManager.getConnectionStatus();
        network["connected"] = true; // Connected if we have IP
        network["ethernet_available"] = true;
        network["ip"] = ipToString(Ethernet.localIP());
        network["mac"] = macStr;
        network["subnet_mask"] = ipToString(Ethernet.subnetMask());
        network["gateway"] = ipToString(Ethernet.gatewayIP());
        network["dns_primary"] = ipToString(Ethernet.dnsServerIP());

        // Load dns_secondary from storage
        JsonDocument configDoc;
        if (FileStorage::loadDeviceConfig(configDoc))
        {
            if (configDoc.containsKey("dns_secondary"))
            {
                network["dns_secondary"] = configDoc["dns_secondary"].as<String>();
            }
            else
            {
                network["dns_secondary"] = ipToString(Ethernet.dnsServerIP());
            }
        }
        else
        {
            network["dns_secondary"] = ipToString(Ethernet.dnsServerIP());
        }

        // MQTT Server IP
        dataObj["mqtt_server_ip"] = mqttManager.getMqttServerIP();

        // Service status
        JsonObject services = dataObj["services"].to<JsonObject>();
        services["api"] = "running";
        services["mqtt"] = mqttManager.getConnectionStatus();
    }

    // Wrapper untuk API response dengan format standard
    inline void buildFullApiResponse(JsonDocument &response)
    {
        response["message"] = "Device information retrieved successfully";
        response["timestamp"] = millis();
        response["encrypted"] = false;

        JsonArray dataArray = response["data"].to<JsonArray>();
        JsonObject dataObj = dataArray.add<JsonObject>();
        buildDeviceInfoData(dataObj);
    }

}

#endif