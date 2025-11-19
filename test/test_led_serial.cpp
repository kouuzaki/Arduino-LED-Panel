// ============================================
// LED Panel Serial CLI Test
// ============================================
// Simple test untuk LED Panel tanpa network
// Command via Serial Monitor (115200 baud):
//   text:Hello World    -> Display text
//   clear               -> Clear screen
//   bright:200          -> Set brightness (0-255)
//   cursor:10,20        -> Set cursor position
//   pixel:5,10,1        -> Set pixel at x,y with color (0/1)
//   fill:1              -> Fill screen with color (0/1)
// ============================================

#include <Arduino.h>
#include <HUB08Panel.h>
#include <Fonts/FreeSans9pt7b.h>

// ============================================
// LED Panel Configuration - ATmega2560
// ============================================
#define DATA_PIN_R1 22
#define DATA_PIN_R2 23
#define CLOCK_PIN 24
#define LATCH_PIN 25
#define ENABLE_PIN 26
#define ADDR_A 27
#define ADDR_B 28
#define ADDR_C 29
#define ADDR_D 30

#define PANEL_WIDTH 64
#define PANEL_HEIGHT 32
#define PANEL_CHAIN 2
#define PANEL_SCAN 16

HUB08_Panel ledPanel(PANEL_WIDTH, PANEL_HEIGHT, PANEL_CHAIN);

// Current cursor position
int cursorX = 0;
int cursorY = 16;

// Serial command buffer
char cmdBuffer[128];
uint8_t cmdIndex = 0;

void setup()
{
    Serial.begin(115200);
    delay(2000);

    Serial.println("=======================================");
    Serial.println("LED Panel Serial CLI Test");
    Serial.println("=======================================");

    // Initialize LED Panel
    if (ledPanel.begin(DATA_PIN_R1, DATA_PIN_R2, CLOCK_PIN, LATCH_PIN, ENABLE_PIN,
                       ADDR_A, ADDR_B, ADDR_C, ADDR_D,
                       PANEL_WIDTH, PANEL_HEIGHT, PANEL_CHAIN, PANEL_SCAN))
    {
        Serial.println("[OK] Panel initialized");
        ledPanel.startScanning(100);
        ledPanel.clearScreen();
        ledPanel.setBrightness(200);

        // Set font
        ledPanel.setAdafruitFont(&FreeSans9pt7b);
        ledPanel.setTextColor(1);

        // Welcome message
        ledPanel.setCursor(0, 16);
        ledPanel.print("Ready");

        Serial.println("[OK] Display ready");
    }
    else
    {
        Serial.println("[FAIL] Panel init failed!");
    }

    Serial.println("");
    Serial.println("Commands:");
    Serial.println("  text:Hello World    - Display text");
    Serial.println("  clear               - Clear screen");
    Serial.println("  bright:200          - Set brightness (0-255)");
    Serial.println("  cursor:10,20        - Set cursor position");
    Serial.println("  pixel:5,10,1        - Set pixel at x,y,color");
    Serial.println("  fill:1              - Fill screen");
    Serial.println("=======================================");
    Serial.println("");
}

void loop()
{
    // Read serial commands
    while (Serial.available() > 0)
    {
        char c = Serial.read();

        if (c == '\n' || c == '\r')
        {
            if (cmdIndex > 0)
            {
                cmdBuffer[cmdIndex] = '\0';
                processCommand(cmdBuffer);
                cmdIndex = 0;
            }
        }
        else if (cmdIndex < sizeof(cmdBuffer) - 1)
        {
            cmdBuffer[cmdIndex++] = c;
        }
    }

    delay(10);
}

void processCommand(char *cmd)
{
    Serial.print("> ");
    Serial.println(cmd);

    // Command: text:Hello World
    if (strncmp(cmd, "text:", 5) == 0)
    {
        char *text = cmd + 5;
        ledPanel.fillScreen(0);
        ledPanel.setCursor(cursorX, cursorY);
        ledPanel.print(text);
        Serial.print("[OK] Text: ");
        Serial.println(text);
    }

    // Command: clear
    else if (strcmp(cmd, "clear") == 0)
    {
        ledPanel.fillScreen(0);
        Serial.println("[OK] Screen cleared");
    }

    // Command: bright:200
    else if (strncmp(cmd, "bright:", 7) == 0)
    {
        int brightness = atoi(cmd + 7);
        if (brightness < 0)
            brightness = 0;
        if (brightness > 255)
            brightness = 255;
        ledPanel.setBrightness(brightness);
        Serial.print("[OK] Brightness: ");
        Serial.println(brightness);
    }

    // Command: cursor:10,20
    else if (strncmp(cmd, "cursor:", 7) == 0)
    {
        char *comma = strchr(cmd + 7, ',');
        if (comma)
        {
            *comma = '\0';
            cursorX = atoi(cmd + 7);
            cursorY = atoi(comma + 1);
            Serial.print("[OK] Cursor: ");
            Serial.print(cursorX);
            Serial.print(",");
            Serial.println(cursorY);
        }
        else
        {
            Serial.println("[ERROR] Format: cursor:x,y");
        }
    }

    // Command: pixel:5,10,1
    else if (strncmp(cmd, "pixel:", 6) == 0)
    {
        char *ptr = cmd + 6;
        char *comma1 = strchr(ptr, ',');
        if (comma1)
        {
            *comma1 = '\0';
            int x = atoi(ptr);

            char *comma2 = strchr(comma1 + 1, ',');
            if (comma2)
            {
                *comma2 = '\0';
                int y = atoi(comma1 + 1);
                int color = atoi(comma2 + 1);

                ledPanel.drawPixel(x, y, color);
                Serial.print("[OK] Pixel: ");
                Serial.print(x);
                Serial.print(",");
                Serial.print(y);
                Serial.print(" = ");
                Serial.println(color);
            }
            else
            {
                Serial.println("[ERROR] Format: pixel:x,y,color");
            }
        }
        else
        {
            Serial.println("[ERROR] Format: pixel:x,y,color");
        }
    }

    // Command: fill:1
    else if (strncmp(cmd, "fill:", 5) == 0)
    {
        int color = atoi(cmd + 5);
        ledPanel.fillScreen(color);
        Serial.print("[OK] Fill: ");
        Serial.println(color);
    }

    // Command: help
    else if (strcmp(cmd, "help") == 0)
    {
        Serial.println("");
        Serial.println("Available Commands:");
        Serial.println("  text:Hello World    - Display text at cursor");
        Serial.println("  clear               - Clear entire screen");
        Serial.println("  bright:200          - Set brightness (0-255)");
        Serial.println("  cursor:10,20        - Set cursor position (x,y)");
        Serial.println("  pixel:5,10,1        - Set pixel (x,y,color)");
        Serial.println("  fill:1              - Fill screen with color");
        Serial.println("  help                - Show this help");
        Serial.println("");
    }

    // Unknown command
    else
    {
        Serial.println("[ERROR] Unknown command. Type 'help' for commands.");
    }
}
