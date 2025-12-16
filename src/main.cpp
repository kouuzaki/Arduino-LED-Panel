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

// --- Ethernet Link Monitoring ---

/**
 * @brief Wait for Ethernet link to become active
 * @param timeoutMs Maximum time to wait in milliseconds
 * @return true if link is up, false if timeout
 */
bool waitForLinkUp(unsigned long timeoutMs)
{
    unsigned long start = millis();
    while (millis() - start < timeoutMs)
    {
        auto link = Ethernet.linkStatus();
        if (link == LinkON)
        {
            return true;
        }
        delay(100);
    }
    return false;
}

/**
 * @brief Initialize Ethernet with link detection and retry mechanism
 * @return true if successfully initialized and linked, false otherwise
 */
bool initEthernetWithRetry()
{
    const int MAX_RETRIES = 3;
    const unsigned long LINK_TIMEOUT = 10000; // 10 seconds

    for (int attempt = 1; attempt <= MAX_RETRIES; attempt++)
    {
        Serial.print("Ethernet Init Attempt ");
        Serial.print(attempt);
        Serial.print("/");
        Serial.print(MAX_RETRIES);
        Serial.print("... ");

        // Display status on LED panel
        display.fillScreen(0);
        char statusMsg[32];
        snprintf(statusMsg, sizeof(statusMsg), "ETHERNET\nINIT %d/%d", attempt, MAX_RETRIES);
        display.drawTextMultilineCentered(statusMsg);
        display.swapBuffers(true);

        // Initialize Ethernet
        Ethernet.begin(mac, ip, dns, gateway, subnet);

        // Check hardware
        if (Ethernet.hardwareStatus() == EthernetNoHardware)
        {
            Serial.println("NO HARDWARE!");
            display.fillScreen(0);
            display.drawTextMultilineCentered("ERR: NO\nHARDWARE");
            display.swapBuffers(true);
            return false; // Fatal error, no retry
        }

        Serial.print("HW OK, IP: ");
        Serial.print(Ethernet.localIP());
        Serial.print(", Waiting for link... ");

        // Display waiting for link
        display.fillScreen(0);
        display.drawTextMultilineCentered("WAITING\nLINK UP");
        display.swapBuffers(true);

        // Wait for link up
        if (waitForLinkUp(LINK_TIMEOUT))
        {
            Serial.println("LINK UP!");
            return true;
        }

        Serial.println("TIMEOUT");

        // If not last attempt, wait before retry
        if (attempt < MAX_RETRIES)
        {
            Serial.println("Retrying in 2 seconds...");
            display.fillScreen(0);
            display.drawTextMultilineCentered("RETRY IN\n2 SEC");
            display.swapBuffers(true);
            delay(2000);
        }
    }

    // All retries failed
    Serial.println("FATAL: Failed to establish Ethernet link after all retries");
    display.fillScreen(0);
    display.drawTextMultilineCentered("ERR:\nNO LINK");
    display.swapBuffers(true);
    return false;
}

/**
 * @brief Check Ethernet link status and attempt recovery if down
 * Called periodically from loop()
 */
void checkEthernetLink()
{
    static unsigned long lastCheck = 0;
    static bool wasLinkUp = true;
    const unsigned long CHECK_INTERVAL = 5000; // Check every 5 seconds

    unsigned long now = millis();
    if (now - lastCheck < CHECK_INTERVAL)
    {
        return;
    }
    lastCheck = now;

    auto link = Ethernet.linkStatus();
    bool isLinkUp = (link == LinkON);

    // Detect link state change
    if (isLinkUp != wasLinkUp)
    {
        if (isLinkUp)
        {
            Serial.println("Ethernet: Link restored");
            display.fillScreen(0);
            display.drawTextMultilineCentered("LINK\nRESTORED");
            display.swapBuffers(true);
            delay(1000);
        }
        else
        {
            Serial.println("Ethernet: Link down detected!");
            display.fillScreen(0);
            display.drawTextMultilineCentered("LINK\nDOWN");
            display.swapBuffers(true);
        }
        wasLinkUp = isLinkUp;
    }

    // If link is down, try to recover
    if (!isLinkUp)
    {
        Serial.println("Ethernet: Attempting recovery...");
        display.fillScreen(0);
        display.drawTextMultilineCentered("RECOVERY\nLINK");
        display.swapBuffers(true);

        // Wait a bit for link to come back
        if (waitForLinkUp(5000))
        {
            Serial.println("Ethernet: Link recovered automatically");
            display.fillScreen(0);
            display.drawTextMultilineCentered("LINK\nOK");
            display.swapBuffers(true);
            delay(1000);
            wasLinkUp = true;
        }
    }
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

    // 4. Init Ethernet with Link Detection and Auto-Recovery
    Serial.println("Init Ethernet with Link Detection...");
    if (!initEthernetWithRetry())
    {
        Serial.println("FATAL: Cannot establish Ethernet connection");
        while (1)
            ; // Halt on fatal error
    }
    Serial.println("Ethernet: Ready and Linked");
    display.fillScreen(0);
    display.drawTextMultilineCentered("ETHERNET\nOK");
    display.swapBuffers(true);
    delay(1000);

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
    // Monitor and maintain Ethernet link
    checkEthernetLink();
    Ethernet.maintain();
    
    // Update services
    mqttManager.update();
    apiHandler.handleClient();
    
    delay(10);
}