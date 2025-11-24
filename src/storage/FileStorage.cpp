#include "storage/FileStorage.h"

namespace FileStorage
{

    const uint16_t EEPROM_START_ADDR = 0;
    const uint8_t MAGIC_1 = 0xAB;
    const uint8_t MAGIC_2 = 0xCD;

    bool begin()
    {
        // EEPROM is always ready on AVR
        return true;
    }

    bool saveDeviceConfig(const JsonDocument &doc)
    {
        // Serialize JSON to buffer
        char buf[512];
        size_t len = serializeJson(doc, buf, sizeof(buf));
        if (len == 0) return false;

        // Write Magic
        int addr = EEPROM_START_ADDR;
        EEPROM.update(addr++, MAGIC_1);
        EEPROM.update(addr++, MAGIC_2);

        // Write Length
        EEPROM.put(addr, (uint16_t)len);
        addr += sizeof(uint16_t);

        // Write Data
        for (size_t i = 0; i < len; i++) {
            EEPROM.update(addr++, buf[i]);
        }
        
        return true;
    }

    bool loadDeviceConfig(JsonDocument &doc)
    {
        int addr = EEPROM_START_ADDR;
        
        // Check Magic
        if (EEPROM.read(addr++) != MAGIC_1) return false;
        if (EEPROM.read(addr++) != MAGIC_2) return false;

        // Read Length
        uint16_t len;
        EEPROM.get(addr, len);
        addr += sizeof(uint16_t);

        if (len == 0 || len > 1024) return false; // Sanity check

        // Read Data
        char *buf = (char *)malloc(len + 1);
        if (!buf) return false;

        for (size_t i = 0; i < len; i++) {
            buf[i] = EEPROM.read(addr++);
        }
        buf[len] = '\0';

        DeserializationError err = deserializeJson(doc, buf);
        free(buf);
        
        return !err;
    }

} // namespace FileStorage
