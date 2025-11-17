#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

/**
 * @brief Device Configuration Manager
 * Mengelola konfigurasi perangkat (IP, MQTT, dll) yang disimpan di EEPROM (Arduino)
 */
class DeviceConfig
{
public:
    static DeviceConfig &getInstance();

    // Initialization
    bool init();
    bool init(bool forceDefaults); // Force use custom defaults even if config exists

    // Configuration structure
    struct Config
    {
        String device_id;
        String device_ip;     // Static IP untuk Ethernet
        String subnet_mask;   // Subnet mask (e.g., 255.255.255.0)
        String gateway;       // Gateway IP
        String dns_primary;   // Primary DNS
        String dns_secondary; // Secondary DNS (optional)
        String mqtt_server;   // MQTT Broker IP
        int mqtt_port;
        String mqtt_username;
        String mqtt_password;
    };

    // Load/Save
    bool loadConfig();
    bool saveConfig();
    void resetConfig();
    void setCustomDefaults(const String &id, const String &ip, const String &subnet = "255.255.255.0",
                           const String &gateway = "192.168.1.1", const String &dns1 = "8.8.8.8",
                           const String &dns2 = "8.8.8.8", int mqttPort = 1883,
                           const String &mqttServer = "", const String &mqttUser = "",
                           const String &mqttPass = "");

    // Getters
    String getDeviceID() const { return config.device_id; }
    String getDeviceIP() const { return config.device_ip; }
    String getSubnetMask() const { return config.subnet_mask; }
    String getGateway() const { return config.gateway; }
    String getDnsPrimary() const { return config.dns_primary; }
    String getDnsSecondary() const { return config.dns_secondary; }
    String getMqttServer() const { return config.mqtt_server; }
    int getMqttPort() const { return config.mqtt_port; }
    String getMqttUsername() const { return config.mqtt_username; }
    String getMqttPassword() const { return config.mqtt_password; }

    // Setters
    void setDeviceID(const String &id) { config.device_id = id; }
    void setDeviceIP(const String &ip) { config.device_ip = ip; }
    void setSubnetMask(const String &subnet) { config.subnet_mask = subnet; }
    void setGateway(const String &gw) { config.gateway = gw; }
    void setDnsPrimary(const String &dns) { config.dns_primary = dns; }
    void setDnsSecondary(const String &dns) { config.dns_secondary = dns; }
    void setMqttServer(const String &server) { config.mqtt_server = server; }
    void setMqttPort(int port) { config.mqtt_port = port; }
    void setMqttUsername(const String &user) { config.mqtt_username = user; }
    void setMqttPassword(const String &pass) { config.mqtt_password = pass; }

    // Get config as JSON
    String toJSON() const;

private:
    DeviceConfig() = default;
    ~DeviceConfig() = default;
    DeviceConfig(const DeviceConfig &) = delete;
    DeviceConfig &operator=(const DeviceConfig &) = delete;

    static const int EEPROM_SIZE = 1024;     // Arduino UNO punya 1024 bytes EEPROM
    static const int CONFIG_START_ADDR = 32; // Start at byte 32 (reserve first 32 for other uses)
    static const byte CONFIG_VERSION = 1;    // For versioning

    Config config;
    bool initialized = false;
};

#endif // DEVICE_CONFIG_H
