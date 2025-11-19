/**
 * @file main.cpp
 * @brief Main application for HUB08 LED panel with Ethernet + MQTT integration
 *
 * This example demonstrates:
 *   - Hardware initialization with dual data pins (R1/R2)
 *   - Double buffering with back/front buffer swapping
 *   - Type-safe font namespace usage
 *   - Ethernet with static IP configuration
 *   - MQTT text rendering control via network
 *   - REST API for device info
 */

#include <Arduino.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include "HUB08Panel.h"
#include "fonts.h" // Type-safe font namespace (HUB08Fonts::*)
#include "handlers/mqtt_text_handler.h"
#include "handlers/mqtt_handler.h"
#include "handlers/api_handler.h"

/// ========== Hardware Pin Mapping (Arduino Uno) ==========
/// NOTE: OE MUST be D3 (OC2B) for Timer2 PWM brightness control

#define R1 8   // Data upper half (rows 0-15)   → PORTB[0]
#define R2 9   // Data lower half (rows 16-31)  → PORTB[1]
#define CLK 10 // Shift clock                   → PORTB[2]
#define LAT 11 // Latch enable                  → PORTB[3]
#define OE 3   // Output enable (MUST be D3)    → PORTD[3] (OC2B)

#define A A0 // Row address bit 0 (LSB)       → PORTC[0]
#define B A1 // Row address bit 1             → PORTC[1]
#define C A2 // Row address bit 2             → PORTC[2]
#define D A3 // Row address bit 3 (MSB)       → PORTC[3]

/// ========== Ethernet Configuration ==========
byte mac[] = {0x02, 0x00, 0x00, 0x01, 0x02, 0x03};
IPAddress ip(192, 168, 1, 60);        // Static IP
IPAddress gateway(192, 168, 1, 1);    // Gateway
IPAddress subnet(255, 255, 255, 0);   // Subnet mask
IPAddress mqttBroker(192, 168, 1, 1); // MQTT broker IP
uint16_t mqttPort = 1883;

/// ========== MQTT Configuration ==========
const char *deviceName = "iot_led_panel";
const char *mqttUser = "edgeadmin"; // Leave empty if no auth
const char *mqttPass = "edge123";   // Leave empty if no auth

/// Create display instance (64×32 pixels, single panel, 1/16 scan)
HUB08_Panel display(64, 32, 1);

/// Network clients and handlers
EthernetClient ethClient;
PubSubClient mqttClient(ethClient);
MqttHandler mqttHandler(mqttClient, deviceName);
MqttTextHandler textHandler(mqttClient, display, deviceName);
ApiHandler apiHandler;

/// MQTT callback to route messages to handlers
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("MQTT: [");
    Serial.print(topic);
    Serial.println("]");

    // Route to text handler
    textHandler.handleMessage(topic, payload, length);
}

void setup()
{
    Serial.begin(115200);
    delay(100);
    Serial.println("\n\n=== HUB08 LED Panel - Ethernet + MQTT ===");

    /// ========== Initialize LED Panel ==========
    Serial.println("Initializing LED panel...");
    display.begin(R1, R2, CLK, LAT, OE, A, B, C, D, 64, 32, 1, 16);
    display.setBrightness(255);
    display.setFont(HUB08Fonts::Roboto_Bold_15);
    display.setTextSize(1);
    display.setTextColor(1);

    // Show boot message
    display.fillScreen(0);
    // display.setCursor(15, 15);
    display.drawTextMultilineCentered("Booting...");
    display.swapBuffers(true);

    Serial.println("✓ LED panel initialized");

    /// ========== Initialize Ethernet ==========
    Serial.println("Initializing Ethernet...");
    Ethernet.begin(mac, ip, gateway, gateway, subnet);
    delay(1000);

    Serial.print("✓ IP Address: ");
    Serial.println(Ethernet.localIP());

    /// ========== Initialize REST API Server ==========
    Serial.println("Starting API server...");
    apiHandler.begin();

    /// ========== Initialize MQTT ==========
    Serial.println("Connecting to MQTT broker...");
    mqttClient.setServer(mqttBroker, mqttPort);
    mqttClient.setCallback(mqttCallback);
    mqttHandler.connect(mqttBroker, mqttPort, mqttUser, mqttPass);

    delay(2000);

    /// ========== Display ready ==========
    display.fillScreen(0);
    display.setCursor(0, 5);
    display.print("READY");
    display.setCursor(0, 20);
    display.print("192.168.1.200");
    display.swapBuffers(true);

    Serial.println("✓ System ready!");
    Serial.println("  - Send text via MQTT: device/iot_led_panel/command/text");
    Serial.println("  - API Info: http://192.168.1.200:8080/api/device/info");
}

void loop()
{
    /// Handle Ethernet + MQTT connection and updates
    Ethernet.maintain();

    if (!mqttClient.connected())
    {
        unsigned long now = millis();
        static unsigned long lastReconnect = 0;

        if (now - lastReconnect > 10000)
        {
            lastReconnect = now;
            if (mqttClient.connect(deviceName))
            {
                Serial.println("✓ MQTT connected");
                textHandler.subscribe();
            }
            else
            {
                Serial.println("✗ MQTT connection failed");
            }
        }
    }
    else
    {
        mqttClient.loop();
    }

    /// Handle REST API requests
    apiHandler.handleClient();

    /// Small delay to prevent watchdog timeout
    delay(10);
}
