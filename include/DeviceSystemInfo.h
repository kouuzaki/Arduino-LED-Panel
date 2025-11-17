#ifndef DEVICE_SYSTEM_INFO_H
#define DEVICE_SYSTEM_INFO_H

#include <Arduino.h>
#include <Ethernet.h>
#include <ArduinoJson.h>

// RAM-efficient JSON builder untuk Arduino Uno
// Use StaticJsonDocument - no dynamic allocation
// Capacity 256 bytes sudah cukup untuk response kita

inline void serializeDeviceSystemInfoJson(EthernetClient &client)
{
    // Create static document (no malloc, fixed size)
    StaticJsonDocument<256> doc;

    // Device info
    doc["device_id"] = "iot_led_panel";

    // Calculate uptime
    unsigned long ms = millis();
    unsigned long seconds = ms / 1000;
    unsigned long hours = (seconds % 86400) / 3600;
    unsigned long minutes = (seconds % 3600) / 60;
    unsigned long secs = seconds % 60;

    // IP Address
    IPAddress ip = Ethernet.localIP();
    char ipBuf[16];
    snprintf(ipBuf, sizeof(ipBuf), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    doc["ip"] = ipBuf;

    doc["mac_address"] = "DE:AD:BE:EF:FE:ED";

    // Uptime string HH:mm:ss
    char uptimeBuf[12];
    snprintf(uptimeBuf, sizeof(uptimeBuf), "%02lu:%02lu:%02lu", hours, minutes, secs);
    doc["uptime"] = uptimeBuf;

    doc["uptime_ms"] = ms;

    // Free memory
    extern int __heap_start, *__brkval;
    int v;
    int freeRam = (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
    doc["free_memory"] = freeRam;
    doc["total_memory"] = 2048;

    // Network config
    JsonObject network = doc.createNestedObject("network");
    network["gateway"] = "192.168.1.1";
    network["subnet_mask"] = "255.255.255.0";
    network["dns_primary"] = "192.168.1.1";
    network["dns_secondary"] = "8.8.8.8";

    // Services
    JsonObject services = doc.createNestedObject("services");
    services["api"] = "running";
    services["mqtt"] = "active";
    services["led_panel"] = "scanning";

    // Serialize to client
    serializeJson(doc, client);
}

#endif
