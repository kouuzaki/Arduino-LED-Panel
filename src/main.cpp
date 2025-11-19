#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <DeviceSystemInfo.h>
#include <PubSubClient.h>
#include <HUB08Panel.h>
#include "handlers/api_handler.h"
#include "handlers/mqtt_handler.h"

// ============================================
// LED Panel Configuration
// ============================================
#define DATA_PIN_R1 2
#define DATA_PIN_R2 3
#define CLOCK_PIN 4
#define LATCH_PIN 5
#define ENABLE_PIN 6
#define ADDR_A 7
#define ADDR_B 8
#define ADDR_C 9
#define ADDR_D 10

#define PANEL_WIDTH 64
#define PANEL_HEIGHT 32
#define PANEL_CHAIN 2
#define PANEL_SCAN 16

HUB08_Panel ledPanel(PANEL_WIDTH, PANEL_HEIGHT, PANEL_CHAIN);

// ============================================
// Network Configuration
// ============================================
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress device_ip(192, 168, 1, 60);
IPAddress subnet_mask(255, 255, 255, 0);
IPAddress gateway(192, 168, 1, 1);
IPAddress dns_primary(8, 8, 8, 8);

const char *mqtt_server = "192.168.1.1";
const uint16_t mqtt_port = 1884;
const char *mqtt_username = "edgeadmin";
const char *mqtt_password = "edge123";
const char *device_name = "iot_led_panel";

// ============================================
// Handlers
// ============================================
EthernetClient ethClient;
PubSubClient mqttClient(ethClient);

ApiHandler apiHandler;
MqttHandler mqttHandler(mqttClient, device_name);

// Status monitoring
unsigned long lastStatusCheck = 0;
const unsigned long STATUS_INTERVAL = 5000; // 5 seconds

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("Start");

  // Initialize LED Panel
  if (ledPanel.begin(DATA_PIN_R1, DATA_PIN_R2, CLOCK_PIN, LATCH_PIN, ENABLE_PIN,
                     ADDR_A, ADDR_B, ADDR_C, ADDR_D,
                     PANEL_WIDTH, PANEL_HEIGHT, PANEL_CHAIN, PANEL_SCAN))
  {
    Serial.println("Panel OK");
    ledPanel.startScanning(100);
    ledPanel.clearScreen();
    ledPanel.setBrightness(200);
  }

  // Initialize Ethernet
  Ethernet.begin(mac, device_ip, dns_primary, gateway, subnet_mask);
  delay(1000);
  Serial.print("Eth:");
  Serial.println(Ethernet.localIP());

  // Initialize API Server
  apiHandler.begin();
  Serial.println("API:8080");

  // Initialize MQTT
  mqttHandler.connect(mqtt_server, mqtt_port, mqtt_username, mqtt_password);
  Serial.println("MQTT:Setup");
}

void loop()
{
  // Handle API requests
  apiHandler.handleClient();

  // Handle MQTT
  mqttHandler.update();

  // Print status every 5 seconds
  unsigned long now = millis();
  if (now - lastStatusCheck > STATUS_INTERVAL)
  {
    lastStatusCheck = now;
    
    // Check Ethernet (simple: check if localIP is not 0.0.0.0)
    Serial.print("Eth:");
    IPAddress ip = Ethernet.localIP();
    if (ip != INADDR_NONE && ip[0] != 0)
    {
      Serial.println("OK");
    }
    else
    {
      Serial.println("DOWN");
    }
    
    // Check MQTT
    Serial.print("MQTT:");
    if (mqttHandler.isConnected())
    {
      Serial.println("OK");
    }
    else
    {
      Serial.println("FAIL");
    }
  }

  delay(10);
}
