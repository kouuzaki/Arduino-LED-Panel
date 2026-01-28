/**
 * @file main.cpp
 * @brief HUB12 LED Panel - Production Firmware (HTTP API Only)
 *
 * HTTP REST API endpoints for display control, using HUB12 P10 32×16 panels
 * Uses Default Adafruit Font (5x7)
 */

#include <Arduino.h>
#include <Ethernet.h>
#include "HUB12Panel.h"
#include "Roboto_Bold_12.h"
#include "handlers/api_handler.h"
#include "storage/FileStorage.h"

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

// --- Watchdog Configuration ---
const unsigned long LAN_CHECK_INTERVAL = 2000; // 2 seconds
unsigned long lastLanCheck = 0;
bool lanWasConnected = false;

// --- Global Objects ---
// Lebar 32, Tinggi 16, Chain 2 (Total 64x16)
HUB12_Panel display(32, 16, 2);
ApiHandler apiHandler;

bool initEthernet() {
  Serial.print("Ethernet Init ");

  // Tampilkan status di LED
  display.fillScreen(0);
  // Default font pas 2 baris (8px + 8px = 16px)
  display.drawTextMultilineCentered("LAN INIT...");

  // Initialize Ethernet
  Ethernet.begin(mac, ip, dns, gateway, subnet);

  // Delay sebentar untuk stabilitas
  delay(1000);

  // Cek hardware (link status tidak selalu akurat di W5100)
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("NO HARDWARE!");
    display.fillScreen(0);
    display.drawTextMultilineCentered("ERR: LAN NO HW.");
    return false;
  }

  // Log hardware type
  switch (Ethernet.hardwareStatus()) {
  case EthernetW5100: Serial.print("W5100, "); break;
  case EthernetW5200: Serial.print("W5200, "); break;
  case EthernetW5500: Serial.print("W5500, "); break;
  default: Serial.print("Unknown, "); break;
  }

  Serial.print("IP: ");
  Serial.println(Ethernet.localIP());

  display.fillScreen(0);
  display.drawTextMultilineCentered("LAN OK.");
  delay(500);

  return true;
}

// --- LAN Watchdog ---
void checkLanConnection() {
  auto linkStat = Ethernet.linkStatus();
  bool isConnected = (linkStat == LinkON);

  if (isConnected && !lanWasConnected) {
    Serial.println("LAN: Link UP.");
    Ethernet.maintain();
  } else if (!isConnected && lanWasConnected) {
    Serial.println("LAN: Link DOWN.");
    display.fillScreen(0);
    display.drawTextMultilineCentered("LAN DOWN.");
    Ethernet.maintain();
  }

  lanWasConnected = isConnected;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== HUB12 IOT SYSTEM ===");

  // 1. SPI Pin Safety
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  pinMode(MEGA_HW_SS, OUTPUT);
  digitalWrite(MEGA_HW_SS, HIGH);

  // 2. Load Configuration
  if (FileStorage::begin()) {
    JsonDocument doc;
    if (FileStorage::loadDeviceConfig(doc)) {
      Serial.println("Config: Loaded from Storage");
      if (doc.containsKey("ip")) ip.fromString(doc["ip"].as<String>());
      if (doc.containsKey("gateway")) gateway.fromString(doc["gateway"].as<String>());
      if (doc.containsKey("subnet_mask")) subnet.fromString(doc["subnet_mask"].as<String>());
      if (doc.containsKey("dns_primary")) dns.fromString(doc["dns_primary"].as<String>());
    } else {
      Serial.println("Config: Not found, using defaults");
    }
  }

  // 3. Init Display (HUB12 P10)
  Serial.print("Init Display ");
  // Parameter: R=5, CLK=7, LAT=8, OE=3, A=A0, B=A1, W=32, H=16, Chain=2
  if (display.begin(5, 7, 8, 3, A0, A1, 32, 16, 2)) {
    Serial.println("OK");
    
    display.setBrightness(10);
    // display.setCursor(0, 0);
    display.setFont(&Roboto_Bold_12);
    display.setTextSize(1);
    display.setTextColor(1);
    
    display.fillScreen(0);
    delay(500);
  } else {
    Serial.println("FAILED");
    while (1);
  }

  // 4. Init Ethernet
  if (!initEthernet()) {
    Serial.println("FATAL: No Ethernet hardware");
    while (1);
  }

  // 5. Init API
  apiHandler.begin();
  apiHandler.setDisplay(&display);

  Serial.println("System Ready.");
  display.fillScreen(0);
  
  display.drawTextMultilineCentered("READY.");
}

// --- Loop ---

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastLanCheck >= LAN_CHECK_INTERVAL) {
    lastLanCheck = currentMillis;
    checkLanConnection();
  }

  Ethernet.maintain();
  apiHandler.handleClient();
  delay(10);
}