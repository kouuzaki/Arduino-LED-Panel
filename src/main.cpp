/**
 * @file main.cpp
 * @brief HUB08 LED Panel - Production Firmware (HTTP API Only)
 *
 * HTTP REST API endpoints for display control
 */

#include <Arduino.h>
#include <Ethernet.h>

#include "HUB08Panel.h"
#include "fonts.h"
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

// --- Watchdog Configuration ---
const unsigned long LAN_CHECK_INTERVAL = 2000; // 2 seconds
unsigned long lastLanCheck = 0;
bool lanWasConnected = false;

// --- Global Objects ---
HUB08_Panel display(64, 32, 2);
ApiHandler apiHandler;

// --- Ethernet Initialization ---
// LAN monitoring via watchdog timer in loop()

/**
 * @brief Initialize Ethernet - simple init without link detection
 * @return true if hardware detected, false otherwise
 */
bool initEthernet() {
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
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("NO HARDWARE!");
    display.fillScreen(0);
    display.drawTextMultilineCentered("ERR: NO\nHARDWARE");
    display.swapBuffers(true);
    return false;
  }

  // Log hardware type
  switch (Ethernet.hardwareStatus()) {
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

// --- LAN Watchdog ---

/**
 * @brief Check LAN link status and handle reconnection
 * Uses Ethernet.linkStatus() if available (W5500)
 * Falls back to simple maintain() for older chips
 */
void checkLanConnection() {
  // Check link status (works reliably on W5500)
  auto linkStat = Ethernet.linkStatus();
  bool isConnected = (linkStat == LinkON);

  // Detect state change
  if (isConnected && !lanWasConnected) {
    Serial.println("LAN: Link UP");
    // Reinitialize connection if needed
    Ethernet.maintain();
  } else if (!isConnected && lanWasConnected) {
    Serial.println("LAN: Link DOWN - attempting recovery");
    display.fillScreen(0);
    display.drawTextMultilineCentered("LAN\nDOWN");
    display.swapBuffers(true);

    // Attempt to maintain/renew DHCP
    Ethernet.maintain();
  }

  lanWasConnected = isConnected;
}

// --- Setup ---

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== HUB08 IOT SYSTEM ===");

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

      // Update Network Config
      if (doc.containsKey("ip"))
        ip.fromString(doc["ip"].as<String>());
      if (doc.containsKey("gateway"))
        gateway.fromString(doc["gateway"].as<String>());
      if (doc.containsKey("subnet_mask"))
        subnet.fromString(doc["subnet_mask"].as<String>());
      if (doc.containsKey("dns_primary"))
        dns.fromString(doc["dns_primary"].as<String>());
    } else {
      Serial.println("Config: Not found, using defaults");
    }
  }

  // 3. Init Display
  Serial.print("Init Display... ");
  // Use explicit pin numbers here (no defines in main.cpp)
  // Data R1..LAT: D5, D6, D7, D8; OE: D3; Address pins: A0..A3
  if (display.begin(5, 6, 7, 8, 3, A0, A1, A2, A3, 64, 32, 2, 16)) {
    Serial.println("OK");
    display.setBrightness(255);
    display.setFont(HUB08Fonts::Roboto_Bold_15);
    display.setTextSize(1);
    display.setTextColor(1);
    display.fillScreen(0);
    display.drawTextMultilineCentered("BOOTING...");
    display.swapBuffers(true);
  } else {
    Serial.println("FAILED");
    while (1)
      ;
  }

  // 4. Init Ethernet (simplified - no link detection)
  if (!initEthernet()) {
    Serial.println("FATAL: No Ethernet hardware");
    while (1)
      ; // Halt on fatal error
  }

  // 5. Init HTTP API Handler
  apiHandler.begin();
  apiHandler.setDisplay(&display);

  Serial.println("System Ready.");
  display.fillScreen(0);
  display.drawTextMultilineCentered("SYSTEM\nREADY");
  display.swapBuffers(true);
}

// --- Loop ---

void loop() {
  unsigned long currentMillis = millis();

  // LAN Watchdog - check every 2 seconds
  if (currentMillis - lastLanCheck >= LAN_CHECK_INTERVAL) {
    lastLanCheck = currentMillis;
    checkLanConnection();
  }

  Ethernet.maintain();
  apiHandler.handleClient();
  delay(10);
}