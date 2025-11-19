// ============================================
// LED Panel 3-Second Text Demo
// ============================================
// Simple demo that displays text for 3 seconds
// No serial commands - just startup display
// ============================================

#include <Arduino.h>
#include <HUB08Panel.h>
#include <Fonts/FreeSans9pt7b.h>

// ============================================
// LED Panel Configuration - Arduino Mega (ATmega2560)
// ============================================
// IMPORTANT: ENABLE_PIN MUST be a PWM-capable pin!
// PWM pins on Mega: 2-13, 44-46
#define DATA_PIN_R1 22 // Upper half data
#define DATA_PIN_R2 23 // Lower half data
#define CLOCK_PIN 24   // Shift clock
#define LATCH_PIN 25   // Latch
#define ENABLE_PIN 3   // OE - MUST BE PWM PIN! (Pin 3 is PWM on Mega)
#define ADDR_A 27      // Address A
#define ADDR_B 28      // Address B
#define ADDR_C 29      // Address C
#define ADDR_D 30      // Address D

#define PANEL_WIDTH 64  // Single panel width
#define PANEL_HEIGHT 32 // Single panel height
#define PANEL_CHAIN 2   // Number of panels chained
#define PANEL_SCAN 16   // 1/16 scan rate

HUB08_Panel ledPanel(PANEL_WIDTH, PANEL_HEIGHT, PANEL_CHAIN);

void setup()
{
  // Initialize serial dengan delay untuk stabilisasi
  Serial.begin(115200);
  delay(500); // CRITICAL: Wait untuk serial port siap

  // Flush any garbage data
  while (Serial.available())
    Serial.read();

  Serial.println("\n\n=== LED Panel Test ===");
  Serial.println("Starting...");

  // Initialize LED Panel
  if (ledPanel.begin(DATA_PIN_R1, DATA_PIN_R2, CLOCK_PIN, LATCH_PIN, ENABLE_PIN,
                     ADDR_A, ADDR_B, ADDR_C, ADDR_D,
                     PANEL_WIDTH, PANEL_HEIGHT, PANEL_CHAIN, PANEL_SCAN))
  {
    Serial.println("Panel initialized!");
    delay(100);

    Serial.println("Starting scanning...");
    // Start scanning first
    // Optimized port manipulation allows higher refresh rates
    // ledPanel.startScanning(100);
    Serial.println("Scanning started.");
    delay(100); // Set brightness
    Serial.println("Setting brightness...");
    ledPanel.setBrightness(50);
    delay(50);

    Serial.println("Starting pattern tests...");

    // TEST 1: Fill screen (should see solid rectangle)
    Serial.println("TEST 1: Full screen ON");
    ledPanel.fillScreen(1);
    delay(2000);

    // TEST 2: Clear screen
    Serial.println("TEST 2: Clear screen");
    ledPanel.fillScreen(0);
    delay(500);

    // TEST 3: Draw simple rectangle
    Serial.println("TEST 3: Rectangle");
    ledPanel.drawRect(5, 5, 50, 20, 1);
    delay(2000);

    // TEST 4: Clear and test text WITHOUT custom font first
    Serial.println("TEST 4: Default font text");
    ledPanel.fillScreen(0);
    ledPanel.setTextColor(1);
    ledPanel.setCursor(2, 8);
    ledPanel.print("TEST"); // Default font
    delay(2000);

    // TEST 5: Now try Adafruit font
    Serial.println("TEST 5: Adafruit font");
    ledPanel.fillScreen(0);
    ledPanel.setAdafruitFont(&FreeSans9pt7b);
    ledPanel.setTextColor(1);
    ledPanel.setCursor(2, 20);
    ledPanel.print("Hello");

    Serial.println("Tests complete!");
    delay(3000);

    ledPanel.fillScreen(0);
  }
  else
  {
    Serial.println("✗ Panel init FAILED!");
    Serial.println("Check wiring!");
  }
}

void loop()
{
  // Nothing to do - just keep the panel running
  delay(100);
}