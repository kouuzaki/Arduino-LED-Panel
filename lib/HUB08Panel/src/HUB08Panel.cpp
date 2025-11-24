#include "HUB08Panel.h"

/// Static instance pointer (set in constructor)
HUB08_Panel *HUB08_Panel::instance = nullptr;

/**
 * @brief Timer1 Compare Match A interrupt service routine @ 10 kHz
 * Calls scan() to display the next row. Fired every 100 microseconds.
 * This provides a 10 kHz row scan rate = 625 Hz full refresh (16 rows)
 */
ISR(TIMER1_COMPA_vect)
{
    if (HUB08_Panel::instance)
        HUB08_Panel::instance->scan();
}

HUB08_Panel::HUB08_Panel(uint16_t w, uint16_t h, uint16_t chain)
    : Adafruit_GFX(w * chain, h)
{
    /// Calculate buffer size: 1 bit per pixel
    bufferSize = (w * chain * h) / 8;
    bufferFront = nullptr;
    bufferBack = nullptr;
    brightness = 255;
    initialized = false;
    instance = this; // Set static instance for ISR access
}

/**
 * @brief Initialize hardware configuration and timers
 * @details Sets up:
 *   - GPIO pins for data, clock, latch, address, and OE
 *   - Timer2 @ 31 kHz PWM on OE pin (D3) for brightness control
 *   - Timer1 @ 10 kHz CTC mode for row scanning ISR
 * @return true if initialization succeeded, false if memory allocation failed
 */
bool HUB08_Panel::begin(const HUB08_Config &cfg)
{
    config = cfg;
    uint8_t *raw = (uint8_t *)malloc(bufferSize * 2);
    if (!raw)
        return false;
    bufferFront = raw;
    bufferBack = raw + bufferSize;
    memset(bufferFront, 0, bufferSize);
    memset(bufferBack, 0, bufferSize);

    // Setup Pins
    pinMode(cfg.data_pin_r1, OUTPUT);
    pinMode(cfg.data_pin_r2, OUTPUT);
    pinMode(cfg.clock_pin, OUTPUT);
    pinMode(cfg.latch_pin, OUTPUT);
    pinMode(cfg.enable_pin, OUTPUT);
    pinMode(cfg.addr_a, OUTPUT);
    pinMode(cfg.addr_b, OUTPUT);
    pinMode(cfg.addr_c, OUTPUT);
    pinMode(cfg.addr_d, OUTPUT);

#if defined(__AVR_ATmega2560__)
    // MEGA 2560 - OE = D3 = OC3C (Timer3)
    TCCR3A = (1 << COM3C1) | (1 << WGM30);
    TCCR3B = (1 << WGM32) | (1 << CS30);
    OCR3C = 255 - pgm_read_byte(&dim_curve[brightness]);
#elif defined(__AVR_ATmega328P__)
    // UNO - OE = D3 = OC2B (Timer2)
    analogWrite(cfg.enable_pin, 128);
    TCCR2B = (TCCR2B & 0b11111000) | 0x01;
    OCR2B = 255 - pgm_read_byte(&dim_curve[brightness]);
#endif
    initialized = true;

    // Timer 1 Setup for Row Scanning
    cli();
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;

    // --- PERBAIKAN DISINI ---
    // Ganti 1599 (10kHz) menjadi 7999 (2kHz)
    // Rumus: (16.000.000 / Frekuensi) - 1
    // 10kHz = 1599 (Terlalu berat untuk 2 panel)
    // 2kHz  = 7999 (Ringan, CPU Load hanya ~40%)
    OCR1A = 7999;

    TCCR1B |= (1 << WGM12) | (1 << CS10); // CTC Mode, Prescaler 1
    TIMSK1 |= (1 << OCIE1A);
    sei();

    return true;
}

/**
 * @brief Parameterized initialization (convenience wrapper)
 * Creates HUB08_Config from individual pin parameters and calls begin(cfg)
 */
bool HUB08_Panel::begin(int8_t r1, int8_t r2, int8_t clk, int8_t lat, int8_t oe,
                        int8_t A, int8_t B, int8_t C, int8_t D,
                        uint16_t w, uint16_t h, uint16_t chain, uint8_t scan)
{
    HUB08_Config cfg;
    cfg.data_pin_r1 = r1;
    cfg.data_pin_r2 = r2;
    cfg.clock_pin = clk;
    cfg.latch_pin = lat;
    cfg.enable_pin = oe;
    cfg.addr_a = A;
    cfg.addr_b = B;
    cfg.addr_c = C;
    cfg.addr_d = D;
    cfg.panel_width = w;
    cfg.panel_height = h;
    cfg.chain_length = chain;
    cfg.panel_scan = scan;
    return begin(cfg);
}

/**
 * @brief Scan and display a single row
 * Called by Timer1 ISR every 100 µs (10 kHz). Advances through all 16 rows
 * in sequence, creating a complete refresh cycle.
 *
 * Procedure:
 *   1. Disable output temporarily (OE HIGH to prevent ghosting during shift)
 *   2. Shift 8 bytes (64 pixels) via R1/R2 data pins using direct port access
 *   3. Set row address (A-D bits)
 *   4. Pulse latch to load data into row latches
 *   5. Restore OE PWM brightness control
 *   6. Advance to next row (wraps 0→15→0)
 *
 * Performance: ~90 µs per row scan (leaves ~10 µs margin within 100 µs ISR period)
 */
void HUB08_Panel::scan()
{
    if (!initialized)
        return;
    static uint8_t row = 0;
    uint8_t bytesPerRow = (config.panel_width * config.chain_length) / 8;
    uint8_t *upper = bufferFront + row * bytesPerRow;
    uint8_t *lower = nullptr;
    if (config.panel_height >= (config.panel_scan * 2))
        lower = bufferFront + (row + config.panel_scan) * bytesPerRow;

    // DISABLE OE (HIGH)
#if defined(__AVR_ATmega2560__)
    uint8_t savedOCR = OCR3C;
    OCR3C = 255;
#else
    uint8_t savedOCR = OCR2B;
    OCR2B = 255;
#endif

#if defined(__AVR_ATmega2560__)
    // ===== MEGA 2560 - Custom mapping (D5..D8) =====
    // Pins may be on different ports (PORTE / PORTH), write each port separately

    for (uint8_t i = 0; i < bytesPerRow; i++)
    {
        uint8_t ur = upper[i];
        uint8_t lr = lower ? lower[i] : 0x00;

        for (uint8_t b = 0; b < 8; b++)
        {
            // Set R1
            if (ur & 0x80)
                HUB_R1_PORT |= (1 << HUB_R1_BIT);
            else
                HUB_R1_PORT &= ~(1 << HUB_R1_BIT);

            // Set R2
            if (lr & 0x80)
                HUB_R2_PORT |= (1 << HUB_R2_BIT);
            else
                HUB_R2_PORT &= ~(1 << HUB_R2_BIT);

            // Clock Pulse
            HUB_CLK_PORT |= (1 << HUB_CLK_BIT);  // CLK HIGH
            HUB_CLK_PORT &= ~(1 << HUB_CLK_BIT); // CLK LOW

            ur <<= 1;
            lr <<= 1;
        }
    }

    // Latch Pulse
    HUB_LAT_PORT |= (1 << HUB_LAT_BIT);  // LAT HIGH
    HUB_LAT_PORT &= ~(1 << HUB_LAT_BIT); // LAT LOW

    // Set Address (PORTF 0-3) - A0-A3
    uint8_t pf = HUB_ADDR_PORT & 0xF0;
    pf |= (row & 0x0F);
    HUB_ADDR_PORT = pf;

#else
    // ===== UNO / 328P - Custom mapping (D5..D8) =====
    for (uint8_t i = 0; i < bytesPerRow; i++)
    {
        uint8_t ur = upper[i];
        uint8_t lr = lower ? lower[i] : 0x00;
        for (uint8_t b = 0; b < 8; b++)
        {
            // R1 (PD5)
            if (ur & 0x80)
                HUB_R1_PORT |= (1 << HUB_R1_BIT);
            else
                HUB_R1_PORT &= ~(1 << HUB_R1_BIT);

            // R2 (PD6)
            if (lr & 0x80)
                HUB_R2_PORT |= (1 << HUB_R2_BIT);
            else
                HUB_R2_PORT &= ~(1 << HUB_R2_BIT);

            // Clock pulse (PD7)
            HUB_CLK_PORT |= (1 << HUB_CLK_BIT);
            HUB_CLK_PORT &= ~(1 << HUB_CLK_BIT);

            ur <<= 1;
            lr <<= 1;
        }
    }

    // Latch pulse
    HUB_LAT_PORT |= (1 << HUB_LAT_BIT);
    HUB_LAT_PORT &= ~(1 << HUB_LAT_BIT);

    // Address (PORTC 0-3)
    uint8_t pc = HUB_ADDR_PORT & 0xF0;
    pc |= (row & 0x0F);
    HUB_ADDR_PORT = pc;
#endif

    // RESTORE OE
#if defined(__AVR_ATmega2560__)
    OCR3C = savedOCR;
#else
    OCR2B = savedOCR;
#endif

    row++;
    if (row >= config.panel_scan)
        row = 0;
}

/**
 * @brief Draw or clear a pixel in back buffer
 * Adafruit_GFX override - coordinates are checked against bounds
 */
void HUB08_Panel::drawPixel(int16_t x, int16_t y, uint16_t c)
{
    if (!initialized)
        return;
    if (x < 0 || x >= width() || y < 0 || y >= height())
        return;

    uint16_t bytesPerRow = (config.panel_width * config.chain_length) / 8;
    uint8_t *ptr = bufferBack + y * bytesPerRow + (x >> 3);

    if (c)
        *ptr |= (0x80 >> (x & 7)); // Set bit (pixel ON)
    else
        *ptr &= ~(0x80 >> (x & 7)); // Clear bit (pixel OFF)
}

/**
 * @brief Fill entire back buffer with solid color
 * Adafruit_GFX override
 */
void HUB08_Panel::fillScreen(uint16_t c)
{
    if (!initialized)
        return;
    memset(bufferBack, c ? 0xFF : 0x00, bufferSize);
}

/**
 * @brief Clear back buffer to black (convenience function)
 */
void HUB08_Panel::clearScreen()
{
    if (!initialized)
        return;
    memset(bufferBack, 0x00, bufferSize);
}

/**
 * @brief Atomically swap front and back buffers
 * Front buffer (displayed) and back buffer (drawing) are swapped safely
 * to prevent ISR from reading partial/incomplete frame data.
 *
 * @param copyFrontToBack If true, copies display buffer back to drawing buffer
 *                        after swap to maintain frame consistency for next draw
 */
void HUB08_Panel::swapBuffers(bool copyFrontToBack)
{
    if (!initialized)
        return;

    /// Swap pointer atomically (disable interrupts to prevent ISR contention)
    cli();
    uint8_t *tmp = bufferFront;
    bufferFront = bufferBack;
    bufferBack = tmp;
    sei();

    /// Optional: synchronize both buffers for consistent multi-frame drawing
    if (copyFrontToBack)
    {
        memcpy(bufferBack, bufferFront, bufferSize);
    }
}

/**
 * @brief Set display brightness via hardware PWM
 * Controls brightness using appropriate timer for the MCU:
 * - UNO: Timer2 OCR2B register (OE pin duty cycle)
 * - MEGA: Timer3 OCR3C register (OE pin duty cycle)
 * Uses gamma-corrected dim_curve for perceptually linear brightness.
 *
 * @param b Brightness level (0-255):
 *          0   = display off
 *          128 = ~50% perceived brightness
 *          255 = maximum brightness
 */
void HUB08_Panel::setBrightness(uint8_t b)
{
    brightness = b;
    if (!initialized)
        return;

    /// Look up gamma-corrected PWM value from PROGMEM table
    uint8_t dim = pgm_read_byte(&dim_curve[brightness]);

    /// OE is active LOW: small OCR value = short OFF time = bright output
#if defined(__AVR_ATmega2560__)
    OCR3C = 255 - dim; // MEGA Timer3 OC3C
#else
    OCR2B = 255 - dim; // UNO Timer2 OC2B
#endif
}

/**
 * @brief Get the pixel width of text in current font
 */
int16_t HUB08_Panel::getTextWidth(const String &text)
{
    int16_t x1, y1;
    uint16_t w, h;

    getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    return w;
}

/**
 * @brief Get the pixel height of current font
 */
int16_t HUB08_Panel::getTextHeight()
{
    int16_t x1, y1;
    uint16_t w, h;

    getTextBounds("Ay", 0, 0, &x1, &y1, &w, &h);
    return h;
}

/**
 * @brief Draw multiline centered text with newline support
 *
 * @param text Text string with optional \\n for line breaks
 * @details
 *   - Clears back buffer
 *   - Parses \\n characters to split lines
 *   - Centers each line horizontally and vertically on display
 *   - Swaps buffers to display
 *   - Max 8 lines supported
 *   - Works on all supported resolutions (64×32, 128×32, etc)
 */
void HUB08_Panel::drawTextMultilineCentered(const String &text)
{
    clearScreen();

    /// ---- 1) Split text by newline characters ----
    const uint8_t MAX_LINES = 8;
    String lines[MAX_LINES];
    uint8_t lineCount = 0;

    String current = "";
    for (uint16_t i = 0; i <= text.length(); i++)
    {
        char c = text[i];

        if (c == '\n' || c == '\0')
        {
            if (current.length() > 0)
                lines[lineCount++] = current;
            current = "";
        }
        else
        {
            current += c;
        }
    }

    if (lineCount == 0)
        return;

    /// ---- 2) Get text metrics ----
    int16_t fontHeight = getTextHeight();
    if (fontHeight < 8)
        fontHeight = 8; // Safety minimum

    /// Tighter line spacing for multiline text
    int16_t lineSpacing = fontHeight - 2;
    int16_t totalHeight = (lineCount - 1) * lineSpacing + fontHeight;

    /// ---- 3) Calculate vertical center ----
    /// True center: (displayHeight - textTotalHeight) / 2
    int16_t verticalMargin = (height() - totalHeight) / 2;

    /// For Adafruit fonts, setCursor uses the baseline + descent
    /// We need to account for this to truly center
    int16_t baselineY = verticalMargin + fontHeight;

    /// ---- 4) Draw and center-align each line ----
    for (uint8_t i = 0; i < lineCount; i++)
    {
        int16_t lineWidth = getTextWidth(lines[i]);
        int16_t centerX = (width() - lineWidth) / 2;
        int16_t lineY = baselineY + i * lineSpacing;

        setCursor(centerX, lineY);
        print(lines[i]);
    }

    /// ---- 5) Swap buffers to display ----
    swapBuffers(true);
}
