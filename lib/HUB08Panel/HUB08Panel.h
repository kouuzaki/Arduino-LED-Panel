#ifndef HUB08_PANEL_H
#define HUB08_PANEL_H

#include <Arduino.h>
#include <Adafruit_GFX.h>

// ============================================
// PIN CONFIGURATION DOCUMENTATION
// ============================================
// ATmega2560 / Arduino Mega Pin Mapping:
// 
// Data Pins (Dual pins for upper/lower half):
//   - DATA_PIN_R1: D22 (Upper half - rows 0-15)
//   - DATA_PIN_R2: D23 (Lower half - rows 16-31)
//
// Control Pins:
//   - CLOCK_PIN:  D24 (Shift clock)
//   - LATCH_PIN:  D25 (Latch enable / LAT)
//   - ENABLE_PIN: D26 (Output enable / OE - active LOW)
//
// Address Pins (1/16 scan = 4 address lines):
//   - ADDR_A: D27 (Address line A - LSB)
//   - ADDR_B: D28 (Address line B)
//   - ADDR_C: D29 (Address line C)
//   - ADDR_D: D30 (Address line D - MSB)
//
// Panel Configuration:
//   - Resolution: 64x32 pixels per panel
//   - Chain: 2 panels (128x32 total)
//   - Scan: 1/16 multiplexing
//
// ============================================

// Timer support untuk Arduino Uno & Mega
#ifdef ARDUINO_AVR_UNO
  #define USE_TIMER1 1
#endif
#ifdef ARDUINO_AVR_MEGA2560
  #define USE_TIMER1 1
#endif

// Forward declaration untuk interrupt callback
class HUB08_Panel;

// Pin definitions untuk HUB08 dengan dual data pins (R1/R2) untuk 64x32 panel
struct HUB08_Config
{
    // Dual data pins untuk upper/lower half
    int8_t data_pin_r1; // Upper half data (row 0-15)
    int8_t data_pin_r2; // Lower half data (row 16-31)
    int8_t clock_pin;   // Shift clock

    // Control pins
    int8_t latch_pin;  // Latch enable (LAT)
    int8_t enable_pin; // Output enable (OE) - active LOW

    // Address pins (untuk 1/16 scan = 4 address lines)
    int8_t addr_a; // Address line A (LSB)
    int8_t addr_b; // Address line B
    int8_t addr_c; // Address line C
    int8_t addr_d; // Address line D (MSB)

    // Panel dimensions
    uint16_t panel_width;
    uint16_t panel_height;
    uint16_t chain_length;

    // Custom scan configuration
    uint8_t panel_scan; // Panel scan rate (4, 8, 16, 32, etc.)
};

class HUB08_Panel : public Adafruit_GFX
{
private:
    HUB08_Config config;
    uint8_t *frameBuffer;
    uint16_t bufferSize;
    uint8_t brightness;
    bool initialized;

    // Row scanning untuk timer interrupt
    volatile uint16_t currentRow;
    volatile bool scanEnabled;

    // Custom scan configuration
    uint8_t addressBits; // Number of address bits needed for scan
    uint8_t maxRows;     // Maximum rows based on scan configuration

    // Pin storage untuk Arduino Uno compatibility
    int8_t stored_data_r1;
    int8_t stored_data_r2;
    int8_t stored_clock;
    int8_t stored_latch;
    int8_t stored_enable;
    int8_t stored_addr_a;
    int8_t stored_addr_b;
    int8_t stored_addr_c;
    int8_t stored_addr_d;

    // Internal methods - SPI pure implementation
    void shiftRow(uint16_t row);
    void selectRowFast(uint16_t row);   // Fast GPIO version
    void latchDataFast();               // Fast GPIO version
    void enableOutputFast(bool enable); // Fast GPIO version
    void clearDisplay();

    // Static methods untuk timer interrupt
    static void scanISR();

public:
    // Static instance untuk ISR (public untuk akses dari ISR)
    static HUB08_Panel *instance;

    // Constructor
    HUB08_Panel(uint16_t width, uint16_t height, uint16_t chain = 1);

    // Initialization
    bool begin(const HUB08_Config &cfg);
    bool begin(int8_t data_r1, int8_t data_r2, int8_t clk, int8_t lat, int8_t oe,
               int8_t a, int8_t b, int8_t c, int8_t d,
               uint16_t width = 64, uint16_t height = 32, uint16_t chain = 1,
               uint8_t panelScan = 16); // Dual data pins R1/R2 configuration

    // GFX overrides
    virtual void drawPixel(int16_t x, int16_t y, uint16_t color) override;
    virtual void fillScreen(uint16_t color) override;

    // HUB08 specific methods
    void setBrightness(uint8_t brt);
    void scan();                                  // Scan single row (called by timer)
    void startScanning(uint32_t frequency = 100); // Start timer scanning
    void stopScanning();                          // Stop timer scanning
    void clearScreen();

    // HUB08SPI compatibility methods
    void drawPoint(uint16_t x, uint16_t y, uint8_t color);
    void drawRect(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h, uint8_t color);

    // Adafruit GFX font methods (simplified - gunakan Adafruit_GFX built-in features)
    void setAdafruitFont(const GFXfont *font = nullptr); // Set font Adafruit (nullptr = default font)
    void printAdafruitText(const char *text, int16_t x, int16_t y, uint16_t color = 1, int16_t paddingX = 0, int16_t paddingY = 0);
    void printAdafruitTextCentered(const char *text, int16_t y, uint16_t color = 1, int16_t paddingX = 0, int16_t paddingY = 0);
    int16_t getAdafruitTextWidth(const char *text);
    int16_t getAdafruitTextHeight(const char *text);

    // Note: Untuk text wrapping, gunakan display.setTextWrap(true) dari Adafruit_GFX

    // Pixel drawing dengan thickness
    void drawThickPixel(int16_t x, int16_t y, uint8_t thickness, uint16_t color);

    // Advanced drawing methods
    void drawThickLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t thickness, uint16_t color);
    void drawThickRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t thickness, uint16_t color, bool filled = false);

    // (Scrolling animation removed) Use Adafruit_GFX drawing functions
    // directly (setCursor/print) for static or custom text rendering.

    // Utility
    static uint16_t color565(uint8_t r, uint8_t g, uint8_t b);
};
#endif