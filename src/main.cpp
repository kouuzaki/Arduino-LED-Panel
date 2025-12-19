/**
 * @file main.cpp
 * @brief HUB08 LED Panel - Production Firmware
 *
 * MQTT TOPIC: device/{id}/cmd/display
 * PAYLOADS:
 * 1. Text:       {"action":"text", "text":"HELLO\nWORLD", "x":0, "y":8, "brightness":128}
 * 2. Clear:      {"action":"clear"}
 */

#include <Arduino.h>
#include <Ethernet.h>
#include <PubSubClient.h>

#include "HUB08Panel.h"
#include "fonts.h"
#include "managers/MqttManager.h"
#include "handlers/MqttDisplayHandler.h"
#include "handlers/MqttResponseHandler.h"
#include "handlers/api_handler.h"
#include "storage/FileStorage.h"

// Pin defines moved into library mapping; main.cpp uses explicit pin numbers

// SPI Safety Pins
#define ETH_CS_PIN 10
#define SD_CS_PIN 4
#define MEGA_HW_SS 53

// --- Default Configuration ---
byte mac[] = {0x02, 0x00, 0x00, 0x01, 0x02, 0x03};
IPAddress ip(10, 10, 10, 60);
IPAddress gateway(10, 10, 10, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(8, 8, 8, 8);

IPAddress mqttBroker(10, 10, 10, 1);
uint16_t mqttPort = 1884;

String mqttUser = "edgeadmin";
String mqttPass = "edge123";
char deviceId[32] = "iot_led_panel";

// --- Global Objects ---
HUB08_Panel display(64, 32, 2);
EthernetClient ethClient;
PubSubClient mqttClient(ethClient);

// --- Managers & Handlers ---
MqttManager mqttManager(mqttClient, deviceId);
MqttDisplayHandler displayHandler(mqttClient, display, deviceId);
MqttResponseHandler responseHandler(mqttClient, deviceId);
ApiHandler apiHandler;

// --- Ethernet Initialization ---
// Note: linkStatus() is unreliable on W5100 and some clone chips
// We skip link detection and let MQTT connection be the indicator

/**
 * @brief Initialize Ethernet - simple init without link detection
 * @return true if hardware detected, false otherwise
 */
bool initEthernet()
{
    Serial.print("Ethernet Init... ");

    // Display status on LED panel
    display.fillScreen(0);
    display.drawTextMultilineCentered("ETHERNET\nINIT");
    display.swapBuffers(true);

    // Initialize Ethernet
    Ethernet.begin(mac, ip, dns, gateway, subnet);

    // Small delay for chip to stabilize
    delay(1000);

    // Check hardware only (link status unreliable)
    if (Ethernet.hardwareStatus() == EthernetNoHardware)
    {
        Serial.println("NO HARDWARE!");
        display.fillScreen(0);
        display.drawTextMultilineCentered("ERR: NO\nHARDWARE");
        display.swapBuffers(true);
        return false;
    }

    // Log hardware type
    switch (Ethernet.hardwareStatus())
    {
    case EthernetW5100:
        Serial.print("W5100, ");
        break;
    case EthernetW5200:
        Serial.print("W5200, ");
        break;
    case EthernetW5500:
        Serial.print("W5500, ");
        break;
    default:
        Serial.print("Unknown, ");
        break;
    }

    Serial.print("IP: ");
    Serial.println(Ethernet.localIP());

    display.fillScreen(0);
    display.drawTextMultilineCentered("ETHERNET\nOK");
    display.swapBuffers(true);
    delay(500);

    return true;
}

// --- Callbacks ---

void globalMqttCallback(char *topic, byte *payload, unsigned int length)
{
    // Route message to display handler
    displayHandler.handleMessage(topic, payload, length);
}

void onMqttConnected()
{
    Serial.println("MQTT: Connected. Subscribing...");
    displayHandler.subscribe();

    // Visual feedback
    display.fillScreen(0);
    display.drawTextMultilineCentered("SYSTEM\nONLINE");
    display.swapBuffers(true);
}

// --- Setup ---

void setup()
{
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== HUB08 IOT SYSTEM ===");

    // 1. SPI Pin Safety
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    pinMode(MEGA_HW_SS, OUTPUT);
    digitalWrite(MEGA_HW_SS, HIGH);

    // 2. Load Configuration
    if (FileStorage::begin())
    {
        JsonDocument doc;
        if (FileStorage::loadDeviceConfig(doc))
        {
            Serial.println("Config: Loaded from Storage");

            // Update Device ID
            if (doc.containsKey("device_id"))
            {
                strlcpy(deviceId, doc["device_id"], sizeof(deviceId));
                mqttManager.setDeviceId(deviceId);
                displayHandler.setDeviceId(deviceId);
            }

            // Update Network & MQTT Config
            if (doc.containsKey("ip"))
                ip.fromString(doc["ip"].as<String>());
            if (doc.containsKey("gateway"))
                gateway.fromString(doc["gateway"].as<String>());
            if (doc.containsKey("subnet_mask"))
                subnet.fromString(doc["subnet_mask"].as<String>());
            if (doc.containsKey("dns_primary"))
                dns.fromString(doc["dns_primary"].as<String>());

            if (doc.containsKey("mqtt_server"))
                mqttBroker.fromString(doc["mqtt_server"].as<String>());
            if (doc.containsKey("mqtt_port"))
                mqttPort = doc["mqtt_port"];
            if (doc.containsKey("mqtt_username"))
                mqttUser = doc["mqtt_username"].as<String>();
            if (doc.containsKey("mqtt_password"))
                mqttPass = doc["mqtt_password"].as<String>();

            if (doc.containsKey("mqtt_heartbeat_interval"))
            {
                mqttManager.setHeartbeatInterval(doc["mqtt_heartbeat_interval"]);
            }
        }
        else
        {
            Serial.println("Config: Not found, using defaults");
        }
    }

    // 3. Init Display
    Serial.print("Init Display... ");
    // Use explicit pin numbers here (no defines in main.cpp)
    // Data R1..LAT: D5, D6, D7, D8; OE: D3; Address pins: A0..A3
    if (display.begin(5, 6, 7, 8, 3, A0, A1, A2, A3, 64, 32, 2, 16))
    {
        Serial.println("OK");
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
        Serial.println("FAILED");
        while (1)
            ;
    }

    // 4. Init Ethernet (simplified - no link detection)
    if (!initEthernet())
    {
        Serial.println("FATAL: No Ethernet hardware");
        while (1)
            ; // Halt on fatal error
    }

    // 5. Init Services
    apiHandler.begin();
    apiHandler.setDisplayHandler(&displayHandler);

    // Connect response handler to display handler (for publishing command responses)
    displayHandler.setResponseHandler(&responseHandler);

    Serial.println("Init MQTT...");
    mqttClient.setCallback(globalMqttCallback);
    mqttManager.setReconnectCallback(onMqttConnected);
    mqttManager.begin(mqttBroker, mqttPort, mqttUser.c_str(), mqttPass.c_str());

    Serial.println("System Ready.");
    display.drawTextMultilineCentered("SYSTEM\nREADY");
    display.swapBuffers(true);
}

// --- Loop ---

void loop()
{
    Ethernet.maintain();
    mqttManager.update();
    apiHandler.handleClient();
    delay(10);
}