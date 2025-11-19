#ifndef DEVICE_SYSTEM_INFO_H
#define DEVICE_SYSTEM_INFO_H

#include <Arduino.h>
#include <Ethernet.h>

inline void buildMqttCompactJson(char* buffer, size_t bufferSize)
{
    unsigned long ms = millis();
    IPAddress ip = Ethernet.localIP();

    extern int __heap_start, *__brkval;
    int v;
    int freeRam = (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);

    snprintf(buffer, bufferSize,
        "{\"device_id\":\"iot_led_panel\",\"ip\":\"%d.%d.%d.%d\",\"uptime_ms\":%lu,\"free_memory\":%d}",
        ip[0], ip[1], ip[2], ip[3], ms, freeRam);
}

#endif
