#ifndef FILE_STORAGE_H
#define FILE_STORAGE_H

#include <Arduino.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

namespace FileStorage
{

    bool begin();

    bool saveDeviceConfig(const JsonDocument &doc);

    bool loadDeviceConfig(JsonDocument &doc);

}

#endif
