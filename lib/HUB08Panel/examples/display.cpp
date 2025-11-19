/**
 * @file main.cpp
 * @brief Main application for HUB08 LED panel demo
 *
 * This example demonstrates:
 *   - Hardware initialization with dual data pins (R1/R2)
 *   - Double buffering with back/front buffer swapping
 *   - Centered multiline text rendering with custom fonts
 *   - Brightness control via serial commands
 *   - Type-safe font namespace usage
 */

#include <Arduino.h>
#include "HUB08Panel.h"
#include "fonts.h" // Type-safe font namespace (HUB08Fonts::*)

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

/// Create display instance (64×32 pixels, single panel, 1/16 scan)
HUB08_Panel display(64, 32, 1);

void setup()
{
    Serial.begin(115200);

    /// Initialize 64×32 panel with dual data pins configuration
    /// Parameters: R1, R2, CLK, LAT, OE, A, B, C, D, width, height, chain, scan_rows
    display.begin(R1, R2, CLK, LAT, OE, A, B, C, D, 64, 32, 1, 16);

    /// Set brightness to maximum (255 = full brightness)
    /// Range: 0 (off) to 255 (maximum)
    display.setBrightness(255);

    /// Select font for display
    /// Use type-safe namespace for clean, auto-complete friendly syntax
    display.setFont(HUB08Fonts::Roboto_Bold_15);
    display.setTextSize(1);

    /// Draw centered multiline text
    /// Text supports \n for line breaks
    /// Buffer operations are non-blocking (ISR handles refresh)
    display.drawTextMultilineCentered("HOWIEE\nKIAW ^^");

    /// Note: swapBuffers() is called automatically inside drawTextMultilineCentered()
    /// The display will now show the text smoothly without flickering

    Serial.println("Display initialized and displaying text");
}

void loop()
{
    // NOTHING TO DO HERE - DISPLAY HANDLED BY ISR
}
