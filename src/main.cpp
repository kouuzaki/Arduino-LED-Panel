/**
 * @file main.cpp
 * @brief HUB08 LED Panel - Production Firmware
 *
 * ==============================================================================
 * DOKUMENTASI MQTT (SINGLE TOPIC)
 * ==============================================================================
 *
 * TOPIC COMMAND:  device/iot_led_panel/cmd/display
 *
 * 1. TAMPILKAN TEKS:
 *    Payload: {"action":"text", "text":"HALO\nDUNIA", "x":0, "y":8}
 *
 * 2. BERSIHKAN LAYAR:
 *    Payload: {"action":"clear"}
 *
 * 3. ATUR KECERAHAN:
 *    Payload: {"action":"brightness", "value":255}
 *
 * ==============================================================================
 */

#include <Arduino.h>
#include <Ethernet.h>
#include <PubSubClient.h>

// --- Custom Libraries ---
#include "HUB08Panel.h"
#include "fonts.h"
#include "managers/MqttManager.h"        // Menangani Koneksi & Heartbeat
#include "handlers/MqttDisplayHandler.h" // Menangani Parsing JSON & Tampilan
#include "handlers/api_handler.h"        // Menangani REST API

/// ========== Hardware Pin Mapping (MEGA 2560 OPTIMIZED) ==========
/// PORT A (Pin 22-25) agar aman dari Ethernet Shield
#define R1 22  // PA0
#define R2 23  // PA1
#define CLK 24 // PA2
#define LAT 25 // PA3
#define OE 3   // PE5 (Timer 3 PWM)

#define A A0 // PF0
#define B A1 // PF1
#define C A2 // PF2
#define D A3 // PF3

/// Pin Kontrol SPI (Safety)
#define ETH_CS_PIN 10
#define SD_CS_PIN 4
#define MEGA_HW_SS 53

/// ========== Network Configuration ==========
byte mac[] = {0x02, 0x00, 0x00, 0x01, 0x02, 0x03};
// Default values as requested
IPAddress ip(192, 168, 1, 60);

IPAddress gateway(1, 1, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(8, 8, 8, 8);

IPAddress mqttBroker(192, 168, 1, 1);
uint16_t mqttPort = 1884;

// MQTT credentials (defaults)
String mqttUser = "edgeadmin";
String mqttPass = "edge123";
char deviceId[32] = "led_lilygo";

/// ========== Global Objects ==========
HUB08_Panel display(64, 32, 2);
EthernetClient ethClient;
PubSubClient mqttClient(ethClient);

/// ========== Managers & Handlers ==========
// MqttManager mengurus koneksi, reconnect, dan heartbeat
MqttManager mqttManager(mqttClient, deviceId);

// MqttDisplayHandler mengurus parsing JSON dan update layar
MqttDisplayHandler displayHandler(mqttClient, display, deviceId);

// ApiHandler mengurus HTTP Request
ApiHandler apiHandler;

/// ========== Callbacks ==========

// 1. Dipanggil oleh PubSubClient saat ada pesan MQTT masuk
void globalMqttCallback(char *topic, byte *payload, unsigned int length)
{
    // Router: Teruskan pesan ke Display Handler
    displayHandler.handleMessage(topic, payload, length);
}

// 2. Dipanggil oleh MqttManager saat berhasil Connect/Reconnect
void onMqttConnected()
{
    Serial.println("MAIN: MQTT Connected -> Subscribing Topics...");

    // Subscribe ulang topic display agar bisa terima perintah
    displayHandler.subscribe();

    // Update status di layar (opsional)
    display.fillScreen(0);
    display.drawTextMultilineCentered("SYSTEM\nONLINE");
    display.swapBuffers(true);
}

/// ========== Setup ==========
void setup()
{
    Serial.begin(115200);
    delay(500);
    Serial.println("\n\n=== HUB08 IOT SYSTEM ===");

    // 1. SPI Safety Config
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    pinMode(MEGA_HW_SS, OUTPUT);
    digitalWrite(MEGA_HW_SS, HIGH);

    // Init FileStorage (EEPROM)
    if (FileStorage::begin())
    {
        Serial.println("✓ Storage (EEPROM) Init OK");

        // Load Config
        JsonDocument doc;
        if (FileStorage::loadDeviceConfig(doc))
        {
            Serial.println("✓ Config Loaded from EEPROM");
            if (doc.containsKey("device_id"))
            {
                strlcpy(deviceId, doc["device_id"], sizeof(deviceId));
                // Update Managers with new ID
                mqttManager.setDeviceId(deviceId);
                displayHandler.setDeviceId(deviceId);
            }
            if (doc.containsKey("mqtt_server"))
            {
                String s = doc["mqtt_server"].as<String>();
                mqttBroker.fromString(s);
            }
            if (doc.containsKey("mqtt_port"))
                mqttPort = doc["mqtt_port"];
            if (doc.containsKey("mqtt_username"))
                mqttUser = doc["mqtt_username"].as<String>();
            if (doc.containsKey("mqtt_password"))
                mqttPass = doc["mqtt_password"].as<String>();

            // Network Config
            if (doc.containsKey("ip"))
            {
                String s = doc["ip"].as<String>();
                ip.fromString(s);
            }
            if (doc.containsKey("gateway"))
            {
                String s = doc["gateway"].as<String>();
                gateway.fromString(s);
            }
            if (doc.containsKey("subnet_mask"))
            {
                String s = doc["subnet_mask"].as<String>();
                subnet.fromString(s);
            }
            if (doc.containsKey("dns_primary"))
            {
                String s = doc["dns_primary"].as<String>();
                dns.fromString(s);
            }

            // MQTT Heartbeat Interval (optional, default 10s)
            if (doc.containsKey("mqtt_heartbeat_interval"))
            {
                unsigned long interval = doc["mqtt_heartbeat_interval"];
                mqttManager.setHeartbeatInterval(interval);
            }
        }
        else
        {
            Serial.println("! Config Not Found, using defaults");
        }
    }
    else
    {
        Serial.println("✗ Storage Init Failed");
    }

    // 2. Init Display
    Serial.println("1. Init Display...");
    if (display.begin(R1, R2, CLK, LAT, OE, A, B, C, D, 64, 32, 2, 16))
    {
        display.setBrightness(255);
        display.setFont(HUB08Fonts::Roboto_Bold_15);
        display.setTextSize(1);
        display.setTextColor(1);
        display.fillScreen(0);
        display.drawTextMultilineCentered("BOOTING...");
        display.swapBuffers(true);
    }
    else
    {
        Serial.println("✗ Display Error");
        while (1)
            ;
    }

    // 3. Init Ethernet
    Serial.println("2. Init Ethernet...");
    Ethernet.begin(mac, ip, dns, gateway, subnet);
    if (Ethernet.hardwareStatus() == EthernetNoHardware)
    {
        Serial.println("✗ No Ethernet Hardware");
        display.drawTextMultilineCentered("ERR: LAN");
        display.swapBuffers(true);
        while (1)
            ;
    }
    Serial.print("✓ IP: ");
    Serial.println(Ethernet.localIP());

    // 4. Init API
    apiHandler.begin();

    // 5. Init MQTT
    Serial.println("3. Init MQTT...");
    // Set callback untuk pesan masuk
    mqttClient.setCallback(globalMqttCallback);

    // Set callback agar kita tahu kapan harus subscribe ulang
    mqttManager.setReconnectCallback(onMqttConnected);

    // Mulai Manager (Koneksi & Auth)
    mqttManager.begin(mqttBroker, mqttPort, mqttUser.c_str(), mqttPass.c_str());

    Serial.println("✓ System Ready");
    display.drawTextMultilineCentered("SYSTEM\nREADY");
    display.swapBuffers(true);
}

/// ========== Main Loop ==========
void loop()
{
    // 1. Jaga koneksi Ethernet
    Ethernet.maintain();

    // 2. Update MQTT Manager
    // Ini otomatis menangani: Reconnect, Loop, dan Heartbeat
    mqttManager.update();

    // 3. Update API
    apiHandler.handleClient();

    delay(10);
}