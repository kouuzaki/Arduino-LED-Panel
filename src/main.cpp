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
  Serial.println("\n--- Ethernet Initialization ---");
  Serial.print("MAC: ");
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 16) Serial.print("0");
    Serial.print(mac[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.println();

  // Tampilkan status di LED
  display.fillScreen(0);
  display.drawTextMultilineCentered("LAN INIT.");

  // Reset W5100 dengan toggle CS pin
  Serial.println("Resetting W5100...");
  digitalWrite(ETH_CS_PIN, LOW);
  delay(10);
  digitalWrite(ETH_CS_PIN, HIGH);
  delay(100);

  // Init Ethernet dengan retry untuk DC jack compatibility
  Serial.println("Starting Ethernet.begin()...");
  Ethernet.begin(mac, ip, dns, gateway, subnet);
  delay(200);
  
  // Cek status hardware dengan retry mechanism
  auto hwStatus = Ethernet.hardwareStatus();
  Serial.print("Hardware Status: ");
  
  // Jika tidak terdetect, tunggu sebentar dan coba lagi (DC jack kadang butuh waktu ekstra)
  if (hwStatus == EthernetNoHardware) {
    Serial.println("Not detected on first try...");
    Serial.println("Waiting extra 500ms for W5100 stabilization...");
    delay(500);
    
    // Retry init
    Ethernet.begin(mac, ip, dns, gateway, subnet);
    delay(200);
    hwStatus = Ethernet.hardwareStatus();
    Serial.print("Hardware Status (retry): ");
  }
  
  switch (hwStatus) {
    case EthernetNoHardware:
      Serial.println("NO HARDWARE DETECTED!");
      Serial.println("Check: 1) Shield mounted? 2) Pin 53 as OUTPUT? 3) SPI pins free?");
      Serial.println("       4) Try manual reset button on Arduino");
      display.fillScreen(0);
      display.drawTextMultilineCentered("ERR: LAN.");
      return false;
    case EthernetW5100:
      Serial.println("W5100 Detected");
      break;
    case EthernetW5200:
      Serial.println("W5200 Detected");
      break;
    case EthernetW5500:
      Serial.println("W5500 Detected");
      break;
    default:
      Serial.println("Unknown Chip");
      break;
  }

  // Check link status (informational only - tidak block karena pakai static IP)
  Serial.print("Checking Link Status... ");
  auto linkStat = Ethernet.linkStatus();
  bool linkUp = (linkStat == LinkON);
  
  if (linkStat == LinkON) {
    Serial.println("✓ Cable Connected");
  } else if (linkStat == LinkOFF) {
    Serial.println("✗ No Cable (OK - using Static IP)");
  } else {
    Serial.println("? Unknown (OK - using Static IP)");
  }

  // Final configuration status
  Serial.println("\n--- Network Configuration ---");
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.print("Gateway: ");
  Serial.println(gateway);
  Serial.print("Subnet: ");
  Serial.println(subnet);
  Serial.print("DNS: ");
  Serial.println(dns);
  Serial.println("Mode: Static IP (ready immediately)");
  Serial.println("--- Configuration OK ---\n");

  display.fillScreen(0);
  display.drawTextMultilineCentered("NET OK");
  delay(500);

  return true;
}

// --- LAN Watchdog ---
void checkLanConnection() {
  auto linkStat = Ethernet.linkStatus();
  bool isConnected = (linkStat == LinkON);

  if (isConnected && !lanWasConnected) {
    Serial.println("LAN: Link UP.");
    // If we regain link but have no IP, try re-init to ensure settings
    IPAddress cur = Ethernet.localIP();
    if (cur == IPAddress(0,0,0,0)) {
      Serial.println("No IP after link up, re-initializing Ethernet");
      initEthernet();
    } else {
      Ethernet.maintain();
    }
  } else if (!isConnected && lanWasConnected) {
    Serial.println("LAN: Link DOWN.");
    display.fillScreen(0);
    display.drawTextMultilineCentered("LAN DOWN.");
    // Let maintain run but don't spam; controller will try to re-init when link returns
    Ethernet.maintain();
  }

  lanWasConnected = isConnected;
}

void setup() {
  // *** CRITICAL: Configure SPI pins FIRST before anything else ***
  // Pin 53 MUST be OUTPUT on Arduino Mega for SPI to work
  pinMode(MEGA_HW_SS, OUTPUT);      // Pin 53 - Hardware SS (CRITICAL!)
  digitalWrite(MEGA_HW_SS, HIGH);
  
  pinMode(SD_CS_PIN, OUTPUT);       // Pin 4 - SD Card CS
  digitalWrite(SD_CS_PIN, HIGH);    // Disable SD Card
  
  pinMode(ETH_CS_PIN, OUTPUT);      // Pin 10 - Ethernet CS
  digitalWrite(ETH_CS_PIN, HIGH);   // Deselect Ethernet initially
  
  // *** CRITICAL FOR DC JACK POWER ***
  // W5100 chip needs time after power-on to stabilize (voltage regulator, oscillator, etc)
  // DC jack 9V 2A: Arduino boots VERY fast, but W5100 needs significant stabilization time
  // USB Type-A: Has natural delay from USB enumeration (~500-1000ms)
  // This extended delay ensures W5100 internal circuitry is fully ready before SPI communication
  // Especially important for cheap/unstable DC adapters that have slow voltage rise time
  delay(3000);  // 3 second delay for maximum DC jack compatibility

  // Now safe to initialize Serial
  Serial.begin(115200);
  delay(100);
  Serial.println("\n=== HUB12 IOT SYSTEM ===");
  Serial.println("SPI Pins Configured:");
  Serial.println("  - Pin 53 (MEGA_HW_SS): OUTPUT/HIGH");
  Serial.println("  - Pin 10 (ETH_CS): OUTPUT/HIGH");
  Serial.println("  - Pin 4 (SD_CS): OUTPUT/HIGH");
  Serial.println("W5100 Power-Up Delay: 3000ms (3 sec - DC jack 9V max compatibility)");

  // 1. Load Configuration
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

  // 2. Init Display (HUB12 P10)
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

  // 3. Init Ethernet (dengan delay untuk stabilisasi SPI)
  delay(200);
  if (!initEthernet()) {
    Serial.println("FATAL: No Ethernet hardware");
    while (1);
  }

  // 4. Init API
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