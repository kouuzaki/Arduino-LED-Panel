# HUB08 LED Panel Library - Professional Documentation

## ⚠️ IMPORTANT: Supported MCU Only

This library **ONLY** works on specific Arduino boards. It requires direct register access and specific timer hardware that not all boards have.

### ✅ **Fully Supported Boards**

| Board                           | MCU        | Status | Notes                           |
| ------------------------------- | ---------- | ------ | ------------------------------- |
| **Arduino Uno**                 | ATmega328P | ✓      | 16MHz, 2KB RAM, full support    |
| **Arduino Nano**                | ATmega328P | ✓      | Identical to Uno, fully support |
| **Arduino Pro Mini (5V/16MHz)** | ATmega328P | ✓      | Same pinout as Uno/Nano         |
| **Arduino Mega 2560**           | ATmega2560 | ✓      | 16MHz, 8KB RAM, full support    |

### 🟨 **Not Supported (Incompatible)**

| Board                   | MCU        | Reason                                             |
| ----------------------- | ---------- | -------------------------------------------------- |
| Leonardo / Micro        | ATmega32U4 | Different timer/port layout                        |
| Arduino Due             | ATSAM3X8E  | ARM-based, no AVR registers                        |
| STM32 (Blue Pill)       | STM32F103  | Different architecture, no direct register support |
| ESP8266 / ESP32         | Tensilica  | WiFi MCU, no stable timer ISR                      |
| Arduino MKR series      | SAMD21     | Different timer system                             |
| Nano Every / Mega Every | ATmega4809 | Different port system (VPORT), new timer (TCA0)    |
| ATtiny85 / ATtiny84     | ATtiny     | Too little RAM, no 16-bit Timer1                   |

### 🔴 **Why These Aren't Supported**

This library depends on:

1. **Direct PORTA/B/C/D/E/F register access** - Required for 10-20× speed improvement
2. **16-bit Timer1** - For ISR at 10kHz (CTC mode)
3. **8-bit Timer2 or Timer3** - For PWM brightness control
4. **Sufficient RAM** - Minimum 2KB for double buffers (64×32 × 2 bits = 512 bytes)
5. **Predictable interrupt timing** - ISR must execute within 100µs

Most modern boards (ARM-based, WiFi MCUs) don't have these exact requirements or have different architectures entirely.

---

- **625 Hz Refresh Rate**: Timer1-based ISR @ 10 kHz row scanning (16 rows × 625 Hz = flicker-free display)
- **Direct Port Manipulation**: PORTB/PORTC/PORTD register access (10-20× faster than digitalWrite)
- **Double Buffering**: Seamless back/front buffer swapping to prevent tearing artifacts
- **PWM Brightness Control**: Hardware Timer2 PWM on OE pin with gamma-corrected brightness curve (0-255)
- **Adafruit_GFX Compatible**: Full drawing API including text, shapes, and custom fonts
- **Minimal CPU Overhead**: ISR-based scanning frees up main loop for application logic

---

## Hardware Specifications

| Property            | Value                                         |
| ------------------- | --------------------------------------------- |
| **Resolution**      | 64 × 32 pixels                                |
| **Pixel Pitch**     | P4.75 (4.75 mm)                               |
| **Scan Mode**       | 1/16 (16 rows, 2 rows per address line)       |
| **Data Pins**       | R1, R2 (dual data lines for upper/lower half) |
| **Control Pins**    | CLK, LAT, OE                                  |
| **Address Pins**    | A, B, C, D (4 bits for row address)           |
| **Typical Voltage** | 5V DC                                         |
| **Typical Current** | 1-2 A @ max brightness (white full screen)    |

---

## Wiring Guide (Arduino Uno / Nano / Pro Mini)

### Pin Mapping Table

| Function          | Arduino Pin | AVR Port | Notes                                         |
| ----------------- | ----------- | -------- | --------------------------------------------- |
| **Data R1**       | D8          | PORTB[0] | Upper half data (rows 0-15)                   |
| **Data R2**       | D9          | PORTB[1] | Lower half data (rows 16-31)                  |
| **Clock**         | D10         | PORTB[2] | Shift clock                                   |
| **Latch**         | D11         | PORTB[3] | Latch enable (LAT)                            |
| **Output Enable** | D3          | PORTD[3] | Active LOW, **MUST use D3 (OC2B/Timer2 PWM)** |
| **Address A**     | A0          | PORTC[0] | Row address bit 0 (LSB)                       |
| **Address B**     | A1          | PORTC[1] | Row address bit 1                             |
| **Address C**     | A2          | PORTC[2] | Row address bit 2                             |
| **Address D**     | A3          | PORTC[3] | Row address bit 3 (MSB)                       |
| **GND**           | GND         | —        | Panel common ground                           |
| **+5V**           | +5V         | —        | Panel power supply                            |

### Schematic Connection Diagram

#### For Arduino Uno / Nano / Pro Mini (ATmega328P)

```
Arduino Uno/Nano                 HUB08 Panel Connector
──────────────────────────────────────────────────────

D8  ─────────────────────────► R1   (Data upper half)
D9  ─────────────────────────► R2   (Data lower half)
D10 ─────────────────────────► CLK  (Shift clock)
D11 ─────────────────────────► LAT  (Latch)
D3  ─────────────────────────► OE   (Output enable, active LOW, Timer2 PWM)

A0  ─────────────────────────► A    (Address bit 0)
A1  ─────────────────────────► B    (Address bit 1)
A2  ─────────────────────────► C    (Address bit 2)
A3  ─────────────────────────► D    (Address bit 3)

GND ─────────────────────────► GND  (Common ground)
+5V ─────────────────────────► +5V  (Power supply)
```

#### For Arduino Mega 2560 (ATmega2560)

```
Arduino Mega 2560                HUB08 Panel Connector
──────────────────────────────────────────────────────

D8  ─────────────────────────► R1   (Data upper half - PORTH5)
D9  ─────────────────────────► R2   (Data lower half - PORTH6)
D10 ─────────────────────────► CLK  (Shift clock - PORTB4)
D11 ─────────────────────────► LAT  (Latch - PORTB5)
D3  ─────────────────────────► OE   (Output enable, active LOW, Timer3 PWM - PORTE5)

A0  ─────────────────────────► A    (Address bit 0 - PORTF0)
A1  ─────────────────────────► B    (Address bit 1 - PORTF1)
A2  ─────────────────────────► C    (Address bit 2 - PORTF2)
A3  ─────────────────────────► D    (Address bit 3 - PORTF3)

GND ─────────────────────────► GND  (Common ground)
+5V ─────────────────────────► +5V  (Power supply)

NOTE: Mega uses different port mapping but SAME pin numbers (D8-11, D3, A0-A3)
      Library auto-detects MCU and applies correct port registers.
```

### Important Notes on Pin Selection

1. **OE Pin (D3) is Critical**: The output enable pin **must** be connected to **D3** because:

   - **Arduino Uno/Nano**: Uses Timer2 OC2B (only PWM timer on D3)
   - **Arduino Mega 2560**: Uses Timer3 OC3C (D3 = PORTE5)
   - Using any other pin will result in no display output or constant brightness

2. **Address Pins**: Analog pins (A0-A3) can be used as digital GPIO. The library automatically configures them as outputs.

3. **Power Supply**: Use a dedicated 5V supply capable of delivering 2+ amps. Do NOT power the LED panel from Arduino's 5V pin.

4. **Current Limiting**: Consider adding 1kΩ resistors in series with data and control lines if running over long cables (>30 cm).

---

## Initialization

### Basic Setup Example

```cpp
#include "HUB08Panel.h"
#include "fonts.h"  // Type-safe font namespace (HUB08Fonts::*)

// Define pin connections
#define R1       8
#define R2       9
#define CLK      10
#define LAT      11
#define OE       3    // MUST be D3 for Timer2 PWM
#define ADDR_A   A0
#define ADDR_B   A1
#define ADDR_C   A2
#define ADDR_D   A3

// Create display instance (64×32 pixels, 1 panel, 1/16 scan)
HUB08_Panel display(64, 32, 1);

void setup() {
    Serial.begin(115200);

    // Initialize hardware with dual data pins configuration
    // Parameters: R1, R2, CLK, LAT, OE, A, B, C, D, width, height, chain_count, scan_rows
    if (!display.begin(R1, R2, CLK, LAT, OE, ADDR_A, ADDR_B, ADDR_C, ADDR_D,
                       64, 32, 1, 16)) {
        Serial.println("ERROR: Failed to initialize HUB08 panel!");
        while (1) delay(100);
    }

    // Set brightness (0-255)
    display.setBrightness(255);

    // Select font using type-safe namespace
    display.setFont(HUB08Fonts::Roboto_Bold_15);
    display.setTextSize(1);
    display.setTextColor(1);

    // Clear display buffer
    display.clearScreen();

    // Display ready; Timer1 ISR handles refresh automatically
}

void loop() {
    // Main application logic here
    // No manual scan() calls needed - Timer1 ISR handles it at 625 Hz
}
```

---

## Font Management

### Available Fonts

The library includes 5 professional Roboto fonts stored in PROGMEM. Access them through the type-safe `HUB08Fonts` namespace for clean, IDE auto-complete friendly syntax:

| Font                         | Size | Usage                                             |
| ---------------------------- | ---- | ------------------------------------------------- |
| `HUB08Fonts::Roboto_6`       | 6pt  | Tiny text, compact display mode                   |
| `HUB08Fonts::Roboto_Bold_12` | 12pt | Small-medium text, good readability               |
| `HUB08Fonts::Roboto_Bold_13` | 13pt | Medium text, balanced appearance                  |
| `HUB08Fonts::Roboto_Bold_14` | 14pt | Medium-large text, emphasis                       |
| `HUB08Fonts::Roboto_Bold_15` | 15pt | Large text, professional appearance (recommended) |

### 🔄 **Adafruit_GFX Font Compatibility**

Since `HUB08_Panel` extends `Adafruit_GFX`, you can also use **any Adafruit_GFX compatible font**:

```cpp
#include "HUB08Panel.h"
#include <Fonts/FreeMonoBold9pt7b.h>       // External Adafruit font
#include <Fonts/FreeSansBold12pt7b.h>      // Another Adafruit font

HUB08_Panel display(64, 32, 1);

void setup() {
    display.begin(R1, R2, CLK, LAT, OE, A, B, C, D, 64, 32, 1, 16);

    // Use ANY Adafruit_GFX font directly
    display.setFont(&FreeMonoBold9pt7b);   // Built-in Adafruit font
    display.setTextSize(1);
    display.setTextColor(1);

    display.setCursor(0, 15);
    display.print("Hello!");
    display.swapBuffers(true);
}
```

**Available Adafruit fonts** (from Adafruit_GFX library):
- `FreeMono9pt7b`, `FreeMonoBold9pt7b`, `FreeMonoOblique9pt7b`, etc.
- `FreeSans9pt7b`, `FreeSansBold9pt7b`, `FreeSansOblique9pt7b`, etc.
- `FreeSerif9pt7b`, `FreeSerifBold9pt7b`, `FreeSerifItalic9pt7b`, etc.

All standard Adafruit_GFX fonts work seamlessly with this library.

### Using Fonts

```cpp
#include "HUB08Panel.h"
#include "fonts.h"  // Type-safe font namespace

HUB08_Panel display(64, 32, 1);

void setup() {
    display.begin(R1, R2, CLK, LAT, OE, A, B, C, D, 64, 32, 1, 16);

    // Method 1: Use built-in Roboto fonts
    display.setFont(HUB08Fonts::Roboto_Bold_15);
    display.setTextSize(1);
    display.setTextColor(1);

    // Method 2: Use Adafruit_GFX fonts
    // display.setFont(&FreeMonoBold9pt7b);  // Uncomment to use Adafruit font

    // Draw text
    display.setCursor(0, 15);
    display.print("Hello!");
    display.swapBuffers(true);
}
```

### Font Characteristics

**Built-in Roboto Fonts:**

- **Proportional**: Variable width per character for natural spacing
- **PROGMEM stored**: Saves RAM (entire font ~2-5 KB each in flash)
- **Adafruit_GFX format**: Compatible with standard Adafruit drawing commands
- **Gamma-corrected rendering**: Works seamlessly with brightness control

**Adafruit_GFX Fonts:**

- **Full compatibility**: All Adafruit fonts work out-of-the-box
- **No additional setup**: Just include the font header and call `setFont()`
- **Mix and match**: Can switch between Roboto and Adafruit fonts in your code

---

## API Reference

### Constructor

```cpp
HUB08_Panel(uint16_t width, uint16_t height, uint16_t chain = 1)
```

Creates a display instance. For standard P4.75 64×32 panels, use `HUB08_Panel display(64, 32, 1)`.

### Initialization

#### `bool begin(int8_t r1, int8_t r2, int8_t clk, int8_t lat, int8_t oe,

               int8_t A, int8_t B, int8_t C, int8_t D,
               uint16_t width = 64, uint16_t height = 32,
               uint16_t chain = 1, uint8_t scan = 16)`

Initializes hardware pins and starts Timer1 (10 kHz) and Timer2 PWM (31 kHz).

**Parameters:**

- `r1, r2`: Data pin numbers (use D8, D9)
- `clk, lat`: Clock and latch pin numbers (use D10, D11)
- `oe`: Output enable pin (use D3 - **REQUIRED**)
- `A, B, C, D`: Row address pins (can use A0-A3)
- `width, height`: Panel resolution (64×32 recommended)
- `chain`: Number of panels chained horizontally (default: 1)
- `scan`: Scan rate divisor (16 for 1/16 scan; 8 for 1/8 scan)

**Returns:** `true` if successful, `false` if memory allocation fails.

---

### Drawing Functions

#### `void drawPixel(int16_t x, int16_t y, uint16_t color)` [Override]

Sets or clears a pixel at (x, y). Writes to back buffer (non-blocking).

```cpp
display.drawPixel(10, 5, 1);  // Turn ON pixel at (10, 5)
display.drawPixel(10, 5, 0);  // Turn OFF pixel at (10, 5)
```

#### `void fillScreen(uint16_t color)` [Override]

Fills entire back buffer with white (1) or black (0).

```cpp
display.fillScreen(0);  // Clear screen to black
display.fillScreen(1);  // Fill screen to white
```

#### `void clearScreen()`

Clears back buffer to black (equivalent to `fillScreen(0)`).

```cpp
display.clearScreen();
```

---

### Buffer Management

#### `void swapBuffers(bool copyFrontToBack = false)`

Atomically swaps front and back buffers. Must be called after drawing to display changes.

**Parameters:**

- `copyFrontToBack`: If `true`, copies display buffer back to drawing buffer after swap (maintains consistency for next frame)

```cpp
display.clearScreen();           // Clear back buffer
display.setCursor(5, 10);
display.print("Hello World");    // Draw text to back buffer
display.swapBuffers(true);       // Display it, and keep a copy for next frame
```

---

### Text Drawing

#### `int16_t getTextWidth(const String &text)`

Returns the pixel width of text at current font size.

```cpp
int16_t w = display.getTextWidth("Status:");
```

#### `int16_t getTextHeight()`

Returns the pixel height of current font.

```cpp
int16_t h = display.getTextHeight();
```

#### `void drawTextMultilineCentered(const String &text)`

Draws multiline text centered on display. Supports `\n` for line breaks.

```cpp
display.drawTextMultilineCentered("Line 1\nLine 2\nLine 3");
display.swapBuffers(true);
```

---

### Brightness Control

#### `void setBrightness(uint8_t brightness)`

Sets display brightness via hardware PWM. Uses gamma-corrected curve for perceptually linear brightness.

**Parameters:**

- `brightness`: 0 (off) to 255 (max brightness)

```cpp
display.setBrightness(128);  // 50% brightness
display.setBrightness(255);  // Maximum brightness
```

---

## Internal Implementation Details

### Timer Configuration

| Timer      | Function            | Frequency | Interrupt                    | Supported MCU       |
| ---------- | ------------------- | --------- | ---------------------------- | ------------------- |
| **Timer1** | Row scanning (ISR)  | 10 kHz    | Every 100 µs, calls `scan()` | UNO, Nano, Mega     |
| **Timer2** | OE PWM (brightness) | 31 kHz    | Fast PWM on OC2B (D3)        | UNO, Nano, Pro Mini |
| **Timer3** | OE PWM (brightness) | 31 kHz    | Fast PWM on OC3C (D3)        | Mega 2560 only      |

**Note**: The library automatically selects the correct timer based on detected MCU using compile-time conditionals.

### Scan Timing

```
Timer1 @ 10kHz
   ↓ (every 100µs)
ISR: scan() executes
   ├─ Shift 8 bytes (64 pixels) via PORTB
   ├─ Set row address (A-D) via PORTC
   ├─ Latch data (LAT pulse)
   ├─ Restore OE PWM
   └─ Advance row (0→15→0)

Result: 16 rows × 625 Hz = 625 Hz refresh rate
```

### Double Buffer Architecture

```
Application             Hardware
─────────────────────────────────
drawPixel()  ────┐
fillScreen() ────┤───► bufferBack  (writing)
setText()    ────┤
              (Adafruit_GFX API)

              ┌─► bufferFront (ISR reads for display)
swapBuffers()─┤   └─ Timer1 ISR calls scan()
              │       Shifts data to LED panel
              └─ Display refreshes @ 625 Hz
```

---

## Complete Example Application

```cpp
#include "HUB08Panel.h"
#include "fonts.h"  // Type-safe font namespace

#define R1       8
#define R2       9
#define CLK      10
#define LAT      11
#define OE       3
#define ADDR_A   A0
#define ADDR_B   A1
#define ADDR_C   A2
#define ADDR_D   A3

HUB08_Panel display(64, 32, 1);
uint32_t lastUpdate = 0;
uint8_t brightness = 255;

void setup() {
    Serial.begin(115200);

    if (!display.begin(R1, R2, CLK, LAT, OE, ADDR_A, ADDR_B, ADDR_C, ADDR_D)) {
        Serial.println("Initialization failed!");
        while (1);
    }

    display.setBrightness(brightness);

    // Use type-safe font namespace (with IDE auto-complete)
    display.setFont(HUB08Fonts::Roboto_Bold_15);
    display.setTextSize(1);
    display.setTextColor(1);

    Serial.println("Display ready!");
}

void loop() {
    // Update every 500ms
    if (millis() - lastUpdate >= 500) {
        lastUpdate = millis();

        // Prepare new frame
        display.clearScreen();
        display.setCursor(0, 15);
        display.print("TIME:");
        display.print(millis() / 1000);

        // Display it
        display.swapBuffers(true);

        // Brightness control via Serial
        if (Serial.available()) {
            uint8_t cmd = Serial.read();
            if (cmd == '+') brightness = min(255, brightness + 10);
            if (cmd == '-') brightness = max(0, brightness - 10);
            display.setBrightness(brightness);
            Serial.print("Brightness: ");
            Serial.println(brightness);
        }
    }
}
```

---

## Troubleshooting

### Display Shows Nothing

- ✓ Verify OE pin is connected to **D3** (not any other pin)
- ✓ Check +5V and GND connections
- ✓ Measure voltage at OE pin: should be PWM signal (not constant HIGH or LOW)

### Display Shows Garbage / Glitchy Text

- ✓ Reduce cable length or add 1kΩ resistors on data lines
- ✓ Use separate 5V supply (not Arduino USB power)
- ✓ Verify address pins (A0-A3) are connected correctly

### Dim Display

- ✓ Call `display.setBrightness(255)` in setup
- ✓ Check power supply voltage (should be 5V ±0.5V)
- ✓ Verify current supply can deliver 2+ amps

### Text Appears Cut Off

- ✓ Adjust `setCursor()` position
- ✓ Call `setTextWrap(false)` to prevent overflow
- ✓ Use `getTextWidth()` to center text manually

---

## Performance Characteristics

| Metric             | Value                                  |
| ------------------ | -------------------------------------- |
| **Refresh Rate**   | 625 Hz (ISR-based)                     |
| **Row Scan Rate**  | 10 kHz (Timer1)                        |
| **Brightness PWM** | 31 kHz (Timer2)                        |
| **RAM Usage**      | ~1 KB (2× 512-byte buffers + overhead) |
| **Flash Usage**    | ~12 KB (with Adafruit_GFX)             |
| **CPU Overhead**   | ~5% (ISR-based, non-blocking)          |

---

## License & Credits

HUB08_Panel library - Arduino driver for HUB08 LED matrix panels  
Built on Adafruit_GFX framework  
Compatible with Arduino Uno, Mega, and other ATmega-based boards

---

## Support & Contributing

For issues, feature requests, or contributions, please refer to the project repository.
