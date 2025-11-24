#ifndef DEVICE_SYSTEM_INFO_H
#define DEVICE_SYSTEM_INFO_H

#include <Arduino.h>
#include <Ethernet.h>
// __heap_start and __brkval are provided by avr-libc; declare them in global namespace
extern int __heap_start;
extern int *__brkval;

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

    // 1. JSON Compact (Untuk MQTT Heartbeat) - Hemat Data
    inline void buildHeartbeatJSON(char *buffer, size_t bufferSize)
    {
        IPAddress ip = Ethernet.localIP();
        int freeRam = getFreeMemory();
        unsigned long uptime = millis();

        snprintf(buffer, bufferSize,
                 "{\"device_id\":\"iot_led_panel\",\"ip\":\"%d.%d.%d.%d\",\"uptime_ms\":%lu,\"free_mem\":%d}",
                 ip[0], ip[1], ip[2], ip[3], uptime, freeRam);
    }

    // 2. JSON Full (Untuk REST API) - Detail Lengkap
    inline void buildFullApiJSON(char *buffer, size_t bufferSize)
    {
        IPAddress ip = Ethernet.localIP();
        IPAddress gw = Ethernet.gatewayIP();
        IPAddress mask = Ethernet.subnetMask();
        IPAddress dns = Ethernet.dnsServerIP();

        int freeRam = getFreeMemory();
        unsigned long ms = millis();

        char uptimeStr[16];
        getUptimeString(uptimeStr, sizeof(uptimeStr));

        // Note: MAC Address hardcoded for display, or pass it in if needed
        // Buffer size must be large enough (>400 bytes recommended)
        snprintf(buffer, bufferSize,
                 "{"
                 "\"device_id\":\"iot_led_panel\","
                 "\"ip\":\"%d.%d.%d.%d\","
                 "\"uptime\":\"%s\","
                 "\"uptime_ms\":%lu,"
                 "\"memory\":{\"free\":%d,\"total\":8192}," // Mega has 8KB
                 "\"network\":{"
                 "\"gateway\":\"%d.%d.%d.%d\","
                 "\"mask\":\"%d.%d.%d.%d\","
                 "\"dns\":\"%d.%d.%d.%d\""
                 "},"
                 "\"services\":{\"api\":\"running\",\"mqtt\":\"active\",\"display\":\"ok\"}"
                 "}",
                 ip[0], ip[1], ip[2], ip[3],
                 uptimeStr,
                 ms,
                 freeRam,
                 gw[0], gw[1], gw[2], gw[3],
                 mask[0], mask[1], mask[2], mask[3],
                 dns[0], dns[1], dns[2], dns[3]);
    }
}

#endif