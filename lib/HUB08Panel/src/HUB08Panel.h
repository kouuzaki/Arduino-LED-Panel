#ifndef HUB08_PANEL_H
#define HUB08_PANEL_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <avr/pgmspace.h>

/**
 * @file HUB08Panel.h
 * @brief High-performance driver for HUB08 LED matrix panels (P4.75 64×32)
 * @details Implements ISR-based scanning with Timer1 @ 10kHz for 625 Hz refresh rate.
 *          Uses double buffering and hardware PWM for smooth, flicker-free display.
 *          Auto-detects MCU and applies correct port/timer mapping.
 *
 * @warning **SUPPORTED MCU ONLY:**
 *          - Arduino Uno (ATmega328P) ✓
 *          - Arduino Nano (ATmega328P) ✓
 *          - Arduino Pro Mini 5V/16MHz (ATmega328P) ✓
 *          - Arduino Mega 2560 (ATmega2560) ✓
 *
 *          Other boards (Leonardo, ESP32, ATtiny, etc.) are NOT supported.
 *          This library requires:
 *          1. Direct PORTA/B/C/D/E/F register access
 *          2. 16-bit Timer1 (for ISR @ 10kHz)
 *          3. 8-bit Timer2 or Timer3 (for PWM brightness)
 *          4. Sufficient RAM (≥2KB for buffers)
 *
 * @see README.md for MCU compatibility table and supported boards list
 */

/// ========== MCU Auto-Detection & Port Mapping ==========
/// This library ONLY supports specific Arduino boards with compatible hardware.
/// See README.md for full compatibility table.

#if defined(__AVR_ATmega2560__)
// ========== Arduino Mega 2560 (ATmega2560) ==========
// Pin to PORT mapping (verified against ATmega2560 datasheet):
// D8  → PORTH5 (R1)       - Data upper half
// D9  → PORTH6 (R2)       - Data lower half
// D10 → PORTB4 (CLK)      - Shift clock
// D11 → PORTB5 (LAT)      - Latch signal
// D3  → PORTE5 (OE)       - Output enable (Timer3 OC3C PWM)
// A0  → PORTF0 (ADDR_A)   - Row address bit 0
// A1  → PORTF1 (ADDR_B)   - Row address bit 1
// A2  → PORTF2 (ADDR_C)   - Row address bit 2
// A3  → PORTF3 (ADDR_D)   - Row address bit 3
//
// CRITICAL: OE uses Timer3 (OC3C), NOT Timer2
// This is a hardware constraint of the Mega 2560 pinout

#define HUB_R1_PORT PORTH
#define HUB_R1_DDR DDRH
#define HUB_R1_BIT 5

#define HUB_R2_PORT PORTH
#define HUB_R2_DDR DDRH
#define HUB_R2_BIT 6

#define HUB_CLK_PORT PORTB
#define HUB_CLK_DDR DDRB
#define HUB_CLK_BIT 4

#define HUB_LAT_PORT PORTB
#define HUB_LAT_DDR DDRB
#define HUB_LAT_BIT 5

#define HUB_OE_PORT PORTE
#define HUB_OE_DDR DDRE
#define HUB_OE_BIT 5

#define HUB_ADDR_PORT PORTF
#define HUB_ADDR_DDR DDRF

#define MCU_NAME "Arduino Mega 2560 (ATmega2560)"

#elif defined(__AVR_ATmega328P__)
// ========== Arduino Uno / Nano / Pro Mini (ATmega328P) ==========
// All these boards use ATmega328P with identical pinout:
// - Arduino Uno
// - Arduino Nano
// - Arduino Pro Mini (5V, 16MHz)
//
// Pin to PORT mapping (verified against ATmega328P datasheet):
// D8  → PORTB0 (R1)       - Data upper half
// D9  → PORTB1 (R2)       - Data lower half
// D10 → PORTB2 (CLK)      - Shift clock
// D11 → PORTB3 (LAT)      - Latch signal
// D3  → PORTD3 (OE)       - Output enable (Timer2 OC2B PWM)
// A0  → PORTC0 (ADDR_A)   - Row address bit 0
// A1  → PORTC1 (ADDR_B)   - Row address bit 1
// A2  → PORTC2 (ADDR_C)   - Row address bit 2
// A3  → PORTC3 (ADDR_D)   - Row address bit 3

#define HUB_R1_PORT PORTB
#define HUB_R1_DDR DDRB
#define HUB_R1_BIT 0

#define HUB_R2_PORT PORTB
#define HUB_R2_DDR DDRB
#define HUB_R2_BIT 1

#define HUB_CLK_PORT PORTB
#define HUB_CLK_DDR DDRB
#define HUB_CLK_BIT 2

#define HUB_LAT_PORT PORTB
#define HUB_LAT_DDR DDRB
#define HUB_LAT_BIT 3

#define HUB_OE_PORT PORTD
#define HUB_OE_DDR DDRD
#define HUB_OE_BIT 3

#define HUB_ADDR_PORT PORTC
#define HUB_ADDR_DDR DDRC

#define MCU_NAME "Arduino Uno / Nano / Pro Mini (ATmega328P)"

#else
#error "╔════════════════════════════════════════════════════════════╗"
#error "║ UNSUPPORTED MCU                                            ║"
#error "║ HUB08Panel library only supports:                          ║"
#error "║   ✓ Arduino Uno (ATmega328P)                               ║"
#error "║   ✓ Arduino Nano (ATmega328P)                              ║"
#error "║   ✓ Arduino Pro Mini 5V/16MHz (ATmega328P)                 ║"
#error "║   ✓ Arduino Mega 2560 (ATmega2560)                         ║"
#error "║                                                            ║"
#error "║ Your board is incompatible. See README.md for details.     ║"
#error "╚════════════════════════════════════════════════════════════╝"
#endif // Perceptual brightness curve (gamma correction) for linear brightness sensation
// Values are stored in PROGMEM to save RAM
const uint8_t dim_curve[] PROGMEM =
    {
        0, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3,
        3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6,
        6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8,
        8, 8, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 11, 11, 11,
        11, 11, 12, 12, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 14, 15,
        15, 15, 16, 16, 16, 16, 17, 17, 17, 18, 18, 18, 19, 19, 19, 20,
        20, 20, 21, 21, 22, 22, 22, 23, 23, 24, 24, 25, 25, 25, 26, 26,
        27, 27, 28, 28, 29, 29, 30, 30, 31, 32, 32, 33, 33, 34, 35, 35,
        36, 36, 37, 38, 38, 39, 40, 40, 41, 42, 43, 43, 44, 45, 46, 47,
        48, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62,
        63, 64, 65, 66, 68, 69, 70, 71, 73, 74, 75, 76, 78, 79, 81, 82,
        83, 85, 86, 88, 90, 91, 93, 94, 96, 98, 99, 101, 103, 105, 107, 109,
        110, 112, 114, 116, 118, 121, 123, 125, 127, 129, 132, 134, 136, 139, 141, 144,
        146, 149, 151, 154, 157, 159, 162, 165, 168, 171, 174, 177, 180, 183, 186, 190,
        193, 196, 200, 203, 207, 211, 214, 218, 222, 226, 230, 234, 238, 242, 247, 252};

/**
 * @struct HUB08_Config
 * @brief Hardware pin configuration for HUB08 LED panel
 */
struct HUB08_Config
{
  int8_t data_pin_r1; ///< Upper half data line (rows 0-15)
  int8_t data_pin_r2; ///< Lower half data line (rows 16-31)
  int8_t clock_pin;   ///< Shift clock (CLK)
  int8_t latch_pin;   ///< Latch enable (LAT)
  int8_t enable_pin;  ///< Output enable (OE) - MUST be D3 for Timer2 PWM, active LOW

  int8_t addr_a; ///< Row address bit 0 (LSB) - A0 recommended
  int8_t addr_b; ///< Row address bit 1 - A1 recommended
  int8_t addr_c; ///< Row address bit 2 - A2 recommended
  int8_t addr_d; ///< Row address bit 3 (MSB) - A3 recommended

  uint16_t panel_width;  ///< Panel width in pixels (typically 64)
  uint16_t panel_height; ///< Panel height in pixels (typically 32)
  uint16_t chain_length; ///< Number of panels chained horizontally (typically 1)
  uint8_t panel_scan;    ///< Scan rate divisor (16 for 1/16 scan)
};

/**
 * @class HUB08_Panel
 * @brief ISR-driven HUB08 LED matrix controller with double buffering
 * @details
 *   - Timer1: 10 kHz ISR for row scanning (625 Hz refresh @ 16 rows)
 *   - Timer2: 31 kHz PWM on OE pin for brightness control
 *   - Double buffer: Back buffer for drawing, front buffer for display
 *   - Adafruit_GFX compatible: Full drawing API + custom fonts
 *
 * @note OE pin MUST be D3 (OC2B) for Timer2 PWM to function
 */
class HUB08_Panel : public Adafruit_GFX
{
public:
  /// Static instance pointer for ISR access
  static HUB08_Panel *instance;

private:
  HUB08_Config config;

  uint8_t *bufferFront; ///< Front buffer (read by ISR during scan)
  uint8_t *bufferBack;  ///< Back buffer (written by Adafruit_GFX drawing functions)
  uint16_t bufferSize;  ///< Size of each buffer in bytes

  uint8_t brightness; ///< Current brightness level (0-255)
  bool initialized;   ///< Initialization flag

public:
  /**
   * @brief Constructor for HUB08_Panel display
   * @param width Panel width in pixels (default 64)
   * @param height Panel height in pixels (default 32)
   * @param chain Number of panels chained horizontally (default 1)
   */
  HUB08_Panel(uint16_t width, uint16_t height, uint16_t chain = 1);

  /**
   * @brief Initialize with HUB08_Config structure
   * @param cfg Configuration struct with pin mappings
   * @return true if successful, false if memory allocation failed
   */
  bool begin(const HUB08_Config &cfg);

  /**
   * @brief Initialize with individual pin parameters
   * @param r1 Data R1 pin number (D8 recommended)
   * @param r2 Data R2 pin number (D9 recommended)
   * @param clk Clock pin number (D10 recommended)
   * @param lat Latch pin number (D11 recommended)
   * @param oe Output enable pin (D3 REQUIRED for Timer2 PWM)
   * @param A Row address A pin (A0 recommended)
   * @param B Row address B pin (A1 recommended)
   * @param C Row address C pin (A2 recommended)
   * @param D Row address D pin (A3 recommended)
   * @param width Panel width (default 64)
   * @param height Panel height (default 32)
   * @param chain Chained panels (default 1)
   * @param scan Scan divisor (default 16 for 1/16 scan)
   * @return true if successful, false if memory allocation failed
   */
  bool begin(int8_t r1, int8_t r2, int8_t clk, int8_t lat, int8_t oe,
             int8_t A, int8_t B, int8_t C, int8_t D,
             uint16_t width = 64, uint16_t height = 32, uint16_t chain = 1,
             uint8_t scan = 16);

  /**
   * @brief Get the pixel width of text in current font
   * @param text Text string to measure
   * @return Width in pixels
   */
  int16_t getTextWidth(const String &text);

  /**
   * @brief Get the pixel height of current font
   * @return Height in pixels
   */
  int16_t getTextHeight();

  /**
   * @brief Draw multiline centered text with newline support
   * @param text Text with optional \\n for line breaks
   */
  void drawTextMultilineCentered(const String &text);

  /**
   * @brief Scan single row (called by Timer1 ISR @ 10 kHz)
   * @note User should NOT call this directly; ISR manages it
   */
  void scan();

  /**
   * @brief Set or clear a pixel in back buffer (Adafruit_GFX override)
   * @param x X coordinate
   * @param y Y coordinate
   * @param c Color (1 = ON, 0 = OFF for monochrome)
   */
  void drawPixel(int16_t x, int16_t y, uint16_t c) override;

  /**
   * @brief Fill entire back buffer with color (Adafruit_GFX override)
   * @param c Color (1 = white/on, 0 = black/off)
   */
  void fillScreen(uint16_t c) override;

  /**
   * @brief Clear back buffer to black (convenience function)
   */
  void clearScreen();

  /**
   * @brief Atomically swap front and back buffers
   * @param copyFrontToBack If true, copy display buffer to drawing buffer after swap
   * @note Call after drawing to make changes visible; ISR-safe atomic operation
   */
  void swapBuffers(bool copyFrontToBack = false);

  /**
   * @brief Set display brightness via hardware PWM
   * @param b Brightness value (0-255, 0=off, 255=maximum)
   * @note Uses gamma-corrected curve for perceptually linear brightness
   * @details Controls Timer2 OCR2B register (OE pin PWM duty cycle)
   */
  void setBrightness(uint8_t b);
};

#endif
