#include "DeviceConfig.h"

DeviceConfig &DeviceConfig::getInstance()
{
    static DeviceConfig instance;
    return instance;
}

bool DeviceConfig::init()
{
    return init(false); // Default: load from EEPROM if exists
}

bool DeviceConfig::init(bool forceDefaults)
{
    if (initialized)
        return true;

    Serial.println("🔧 Initializing Device Configuration (EEPROM)...");

    // Arduino UNO EEPROM doesn't need begin(), just use directly

    // Check if config exists (first byte is version marker)
    byte version = EEPROM.read(CONFIG_START_ADDR);

    if (version != CONFIG_VERSION && !forceDefaults)
    {
        Serial.println("⚠️  No valid config found in EEPROM");
        Serial.println("💡 Call setCustomDefaults() before init(true) to set initial values");
    }
    else if (!forceDefaults && version == CONFIG_VERSION)
    {
        if (loadConfig())
        {
            Serial.println("✅ Configuration loaded from EEPROM");
        }
    }
    else
    {
        Serial.println("🔄 Forcing custom defaults, saving to EEPROM...");
        saveConfig();
    }

    initialized = true;
    return true;
}

void DeviceConfig::setCustomDefaults(const String &id, const String &ip, const String &subnet,
                                     const String &gateway, const String &dns1,
                                     const String &dns2, int mqttPort,
                                     const String &mqttServer, const String &mqttUser,
                                     const String &mqttPass)
{
    config.device_id = id;
    config.device_ip = ip;
    config.subnet_mask = subnet;
    config.gateway = gateway;
    config.dns_primary = dns1;
    config.dns_secondary = dns2;
    config.mqtt_port = mqttPort;
    config.mqtt_server = mqttServer;
    config.mqtt_username = mqttUser;
    config.mqtt_password = mqttPass;

    Serial.println("✏️  Custom defaults applied:");
    Serial.print("   Device ID: ");
    Serial.println(config.device_id);
    Serial.print("   Device IP: ");
    Serial.println(config.device_ip);
    Serial.print("   Subnet: ");
    Serial.println(config.subnet_mask);
    Serial.print("   Gateway: ");
    Serial.println(config.gateway);
    Serial.print("   MQTT Server: ");
    Serial.println(config.mqtt_server.length() > 0 ? config.mqtt_server : "(empty)");
}

bool DeviceConfig::loadConfig()
{
    byte version = EEPROM.read(CONFIG_START_ADDR);
    if (version != CONFIG_VERSION)
    {
        Serial.println("⚠️  Invalid config version in EEPROM");
        return false;
    }

    // Baca config dari EEPROM
    // Format: [version:1][device_id_len:1][device_id:n]...
    // Untuk simplicity, kita use fixed-size strings

    int addr = CONFIG_START_ADDR + 1; // Skip version byte

    // Helper lambda untuk baca string dari EEPROM
    auto readString = [&addr](int maxLen) -> String
    {
        byte len = EEPROM.read(addr++);
        if (len == 0xFF || len > maxLen)
            len = 0;

        String result = "";
        for (byte i = 0; i < len; i++)
        {
            result += (char)EEPROM.read(addr++);
        }

        // Skip remaining bytes
        addr += (maxLen - len);
        return result;
    };

    config.device_id = readString(32);
    config.device_ip = readString(16);
    config.subnet_mask = readString(16);
    config.gateway = readString(16);
    config.dns_primary = readString(16);
    config.dns_secondary = readString(16);
    config.mqtt_server = readString(32);
    config.mqtt_username = readString(32);
    config.mqtt_password = readString(32);

    // Read MQTT port (2 bytes)
    config.mqtt_port = (EEPROM.read(addr) << 8) | EEPROM.read(addr + 1);
    addr += 2;

    return true;
}

bool DeviceConfig::saveConfig()
{
    Serial.println("💾 Saving configuration to EEPROM...");

    int addr = CONFIG_START_ADDR;

    // Write version marker
    EEPROM.write(addr++, CONFIG_VERSION);

    // Helper lambda untuk tulis string ke EEPROM
    auto writeString = [&addr](const String &str, int maxLen)
    {
        byte len = str.length();
        if (len > maxLen)
            len = maxLen;

        EEPROM.write(addr++, len);
        for (byte i = 0; i < len; i++)
        {
            EEPROM.write(addr++, str[i]);
        }

        // Pad dengan 0xFF
        for (byte i = len; i < maxLen; i++)
        {
            EEPROM.write(addr++, 0x00);
        }
    };

    writeString(config.device_id, 32);
    writeString(config.device_ip, 16);
    writeString(config.subnet_mask, 16);
    writeString(config.gateway, 16);
    writeString(config.dns_primary, 16);
    writeString(config.dns_secondary, 16);
    writeString(config.mqtt_server, 32);
    writeString(config.mqtt_username, 32);
    writeString(config.mqtt_password, 32);

    // Write MQTT port (2 bytes)
    EEPROM.write(addr++, (config.mqtt_port >> 8) & 0xFF);
    EEPROM.write(addr++, config.mqtt_port & 0xFF);

    // Arduino UNO EEPROM doesn't need commit()
    Serial.println("✅ Configuration saved to EEPROM");
    return true;
}

void DeviceConfig::resetConfig()
{
    Serial.println("🗑️  Resetting configuration...");

    // Clear config marker
    EEPROM.write(CONFIG_START_ADDR, 0xFF);

    Serial.println("✅ Configuration reset - device will use defaults on next boot");
}

String DeviceConfig::toJSON() const
{
    // Simple JSON concatenation untuk hemat memory di Arduino
    String json = "{";
    json += "\"device_id\":\"" + config.device_id + "\",";
    json += "\"device_ip\":\"" + config.device_ip + "\",";
    json += "\"subnet_mask\":\"" + config.subnet_mask + "\",";
    json += "\"gateway\":\"" + config.gateway + "\",";
    json += "\"dns_primary\":\"" + config.dns_primary + "\",";
    json += "\"dns_secondary\":\"" + config.dns_secondary + "\",";
    json += "\"mqtt_server\":\"" + config.mqtt_server + "\",";
    json += "\"mqtt_port\":" + String(config.mqtt_port) + ",";
    json += "\"mqtt_username\":\"" + config.mqtt_username + "\"";
    json += "}";

    return json;
}
