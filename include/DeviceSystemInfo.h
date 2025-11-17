#ifndef DEVICE_SYSTEM_INFO_H
#define DEVICE_SYSTEM_INFO_H

#include <Arduino.h>
#include <Ethernet.h>

// Ultra-optimized untuk Arduino Uno - use minimal JSON building
// Serialize langsung tanpa intermediate objects
inline void serializeDeviceSystemInfoJson(EthernetClient &client)
{
    // Get uptime
    unsigned long ms = millis();
    unsigned long seconds = ms / 1000;
    unsigned long hours = (seconds % 86400) / 3600;
    unsigned long minutes = (seconds % 3600) / 60;
    unsigned long secs = seconds % 60;
    
    // Calculate free RAM
    extern int __heap_start, *__brkval;
    int v;
    int freeRam = (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
    
    // IP Address
    IPAddress ip = Ethernet.localIP();
    
    // Build JSON directly
    client.print("{");
    client.print("\"device_id\":\"iot_led_panel\",");
    client.print("\"ip\":\"");
    client.print(ip[0]); client.print(".");
    client.print(ip[1]); client.print(".");
    client.print(ip[2]); client.print(".");
    client.print(ip[3]);
    client.print("\",");
    
    client.print("\"mac_address\":\"DE:AD:BE:EF:FE:ED\",");
    
    client.print("\"uptime\":\"");
    if (hours < 10) client.print("0");
    client.print(hours); client.print(":");
    if (minutes < 10) client.print("0");
    client.print(minutes); client.print(":");
    if (secs < 10) client.print("0");
    client.print(secs);
    client.print("\",");
    
    client.print("\"uptime_ms\":"); client.print(ms); client.print(",");
    
    client.print("\"free_memory\":"); client.print(freeRam); client.print(",");
    client.print("\"total_memory\":2048,");
    
    // Network config
    client.print("\"network\":{");
    client.print("\"gateway\":\"192.168.1.1\",");
    client.print("\"subnet_mask\":\"255.255.255.0\",");
    client.print("\"dns_primary\":\"192.168.1.1\",");
    client.print("\"dns_secondary\":\"8.8.8.8\"");
    client.print("},");
    
    // Services
    client.print("\"services\":{");
    client.print("\"api\":\"running\",");
    client.print("\"mqtt\":\"active\",");
    client.print("\"led_panel\":\"scanning\"");
    client.print("}");
    
    client.print("}");
}

#endif

