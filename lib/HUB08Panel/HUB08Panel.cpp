#include "HUB08Panel.h"
#include <Fonts/FreeSans9pt7b.h>

// Brightness dimming curve
static const uint8_t dim_curve[256] PROGMEM = {
    0,
    1,
    1,
    2,
    2,
    2,
    2,
    2,
    2,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    8,
    8,
    8,
    8,
    8,
    8,
    9,
    9,
    9,
    9,
    9,
    9,
    10,
    10,
    10,
    10,
    10,
    11,
    11,
    11,
    11,
    11,
    12,
    12,
    12,
    12,
    12,
    13,
    13,
    13,
    13,
    14,
    14,
    14,
    14,
    15,
    15,
    15,
    16,
    16,
    16,
    16,
    17,
    17,
    17,
    18,
    18,
    18,
    19,
    19,
    19,
    20,
    20,
    20,
    21,
    21,
    22,
    22,
    22,
    23,
    23,
    24,
    24,
    24,
    25,
    25,
    26,
    26,
    27,
    27,
    28,
    28,
    29,
    29,
    30,
    30,
    31,
    32,
    32,
    33,
    33,
    34,
    35,
    35,
    36,
    36,
    37,
    38,
    38,
    39,
    40,
    40,
    41,
    42,
    43,
    43,
    44,
    45,
    46,
    47,
    47,
    48,
    49,
    50,
    50,
    51,
    52,
    53,
    54,
    55,
    56,
    56,
    57,
    58,
    59,
    60,
    61,
    62,
    63,
    64,
    65,
    66,
    67,
    68,
    69,
    70,
    71,
    72,
    73,
    74,
    75,
    76,
    77,
    78,
    79,
    80,
    81,
    82,
    84,
    85,
    86,
    87,
    88,
    89,
    91,
    92,
    93,
    94,
    95,
    97,
    98,
    99,
    100,
    102,
    103,
    104,
    106,
    107,
    109,
    110,
    111,
    113,
    114,
    116,
    117,
    119,
    120,
    121,
    123,
    125,
    126,
    128,
    129,
    131,
    132,
    134,
    136,
    137,
    139,
    141,
    142,
    144,
    146,
    147,
    149,
    151,
    153,
    154,
    156,
    158,
    160,
    162,
    163,
    165,
    167,
    169,
};

// Static instance untuk interrupt service routine
HUB08_Panel *HUB08_Panel::instance = nullptr;

// Constructor
HUB08_Panel::HUB08_Panel(uint16_t width, uint16_t height, uint16_t chain)
    : Adafruit_GFX(width * chain, height)
{
    config.panel_width = width;
    config.panel_height = height;
    config.chain_length = chain;
    config.panel_scan = 16; // Default to 1/16 scan

    bufferSize = (width * chain * height) / 8; // 1 bit per pixel untuk single color
    frameBuffer = nullptr;
    brightness = 128;
    initialized = false;
    currentRow = 0;
    scanEnabled = false;
    addressBits = 4; // Default for 1/16 scan (2^4 = 16)
    maxRows = 16;

    // Set static instance untuk ISR
    instance = this;
}

// Helper function untuk set multiple pins HIGH
inline void setMultiplePinsHigh(int8_t p1, int8_t p2, int8_t p3 = -1, int8_t p4 = -1)
{
    digitalWrite(p1, HIGH);
    digitalWrite(p2, HIGH);
    if (p3 >= 0)
        digitalWrite(p3, HIGH);
    if (p4 >= 0)
        digitalWrite(p4, HIGH);
}

// Helper function untuk set multiple pins LOW
inline void setMultiplePinsLow(int8_t p1, int8_t p2, int8_t p3 = -1, int8_t p4 = -1)
{
    digitalWrite(p1, LOW);
    digitalWrite(p2, LOW);
    if (p3 >= 0)
        digitalWrite(p3, LOW);
    if (p4 >= 0)
        digitalWrite(p4, LOW);
}

// Initialize dengan dual data pins configuration (R1/R2) untuk 64x32 panel
bool HUB08_Panel::begin(const HUB08_Config &cfg)
{
    config = cfg;

    // Calculate address bits and max rows based on panel_scan
    addressBits = 0;
    uint8_t temp = config.panel_scan;
    while (temp > 1)
    {
        temp >>= 1;
        addressBits++;
    }
    maxRows = config.panel_scan;

    // Validate scan rate (must be power of 2)
    if ((1 << addressBits) != config.panel_scan)
    {
        return false;
    }

    // Allocate frame buffer
    frameBuffer = (uint8_t *)malloc(bufferSize);
    if (!frameBuffer)
    {
        return false;
    }

    // Setup pins untuk dual data configuration
    pinMode(config.data_pin_r1, OUTPUT); // R1 - Upper half
    pinMode(config.data_pin_r2, OUTPUT); // R2 - Lower half
    pinMode(config.clock_pin, OUTPUT);   // Clock
    pinMode(config.latch_pin, OUTPUT);
    pinMode(config.enable_pin, OUTPUT);
    pinMode(config.addr_a, OUTPUT);
    pinMode(config.addr_b, OUTPUT);
    pinMode(config.addr_c, OUTPUT);
    pinMode(config.addr_d, OUTPUT);

    // Store pins for later use
    stored_data_r1 = config.data_pin_r1;
    stored_data_r2 = config.data_pin_r2;
    stored_clock = config.clock_pin;
    stored_latch = config.latch_pin;
    stored_enable = config.enable_pin;
    stored_addr_a = config.addr_a;
    stored_addr_b = config.addr_b;
    stored_addr_c = config.addr_c;
    stored_addr_d = config.addr_d;

    // Initialize pin states - set data, clock, latch LOW; enable HIGH
    setMultiplePinsLow(config.data_pin_r1, config.data_pin_r2, config.clock_pin, config.latch_pin);
    digitalWrite(config.enable_pin, HIGH); // OE is active LOW, so HIGH = disabled initially

    // Clear buffer
    memset(frameBuffer, 0, bufferSize);
    clearDisplay();

    initialized = true;
    return true;
}

// Simplified begin method untuk dual data pins
bool HUB08_Panel::begin(int8_t data_r1, int8_t data_r2, int8_t clk, int8_t lat, int8_t oe,
                        int8_t a, int8_t b, int8_t c, int8_t d,
                        uint16_t width, uint16_t height, uint16_t chain,
                        uint8_t panelScan)
{

    HUB08_Config cfg;
    cfg.data_pin_r1 = data_r1;
    cfg.data_pin_r2 = data_r2;
    cfg.clock_pin = clk;
    cfg.latch_pin = lat;
    cfg.enable_pin = oe;
    cfg.addr_a = a;
    cfg.addr_b = b;
    cfg.addr_c = c;
    cfg.addr_d = d;
    cfg.panel_width = width;
    cfg.panel_height = height;
    cfg.chain_length = chain;
    cfg.panel_scan = panelScan; // Set custom scan rate

    return begin(cfg);
}

// Timer interrupt service routine untuk Arduino Uno
void HUB08_Panel::scanISR()
{
    if (instance && instance->scanEnabled)
    {
        instance->scan();
    }
}

// Start scanning dengan timer interrupt - Arduino Uno compatible
void HUB08_Panel::startScanning(uint32_t frequency)
{
    if (!initialized)
        return;

    // Enable scanning flag
    scanEnabled = true;

    // Use custom scan rate instead of hardcoded 16
    uint32_t adjustedFreq = frequency * config.panel_scan;

    // Setup Timer1 interrupt untuk Arduino Uno
    // Timer1 runs at 16MHz / prescaler
    // For ~adjustedFreq Hz scanning frequency
    // We want interrupts at adjustedFreq * config.panel_scan Hz

    cli(); // Disable interrupts during setup

    // Set Timer1 ke CTC mode
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;

    // CTC mode (mode 4)
    TCCR1B |= (1 << WGM12);

    // Set prescaler ke 64
    TCCR1B |= (1 << CS10) | (1 << CS11);

    // OCR1A untuk frequency calculation
    // freq = 16MHz / (prescaler * (OCR1A + 1))
    // OCR1A = (16MHz / (prescaler * freq)) - 1
    uint16_t ocr = (16000000UL / (64UL * adjustedFreq)) - 1;
    OCR1A = ocr;

    // Enable Timer1 Compare A interrupt
    TIMSK1 |= (1 << OCIE1A);

    sei(); // Enable interrupts
}

// Stop scanning timer
void HUB08_Panel::stopScanning()
{
    scanEnabled = false;

    // Disable Timer1 Compare A interrupt
    TIMSK1 &= ~(1 << OCIE1A);
}

// Select row menggunakan digitalWrite (Arduino Uno compatible)
void HUB08_Panel::selectRowFast(uint16_t row)
{
    // Clear all address bits terlebih dahulu
    digitalWrite(stored_addr_a, LOW);
    digitalWrite(stored_addr_b, LOW);
    digitalWrite(stored_addr_c, LOW);
    digitalWrite(stored_addr_d, LOW);

    // Set address bits berdasarkan row value
    if (row & 0x01)
        digitalWrite(stored_addr_a, HIGH);
    if ((row & 0x02) && addressBits >= 2)
        digitalWrite(stored_addr_b, HIGH);
    if ((row & 0x04) && addressBits >= 3)
        digitalWrite(stored_addr_c, HIGH);
    if ((row & 0x08) && addressBits >= 4)
        digitalWrite(stored_addr_d, HIGH);
}

// Shift out data untuk satu baris - Arduino Uno compatible
void HUB08_Panel::shiftRow(uint16_t row)
{
    uint16_t totalWidth = config.panel_width * config.chain_length;
    uint8_t *upperHead = frameBuffer + row * (totalWidth / 8);
    uint8_t *lowerHead = nullptr;

    // For dual data pin panels (seperti 64x32): lower half adalah row + scan
    if (config.panel_height > config.panel_scan)
    {
        lowerHead = frameBuffer + (row + config.panel_scan) * (totalWidth / 8);
    }

    // Transfer menggunakan dual data pins
    for (uint8_t byte = 0; byte < (totalWidth / 8); byte++)
    {
        uint8_t upperPixels = *upperHead;
        uint8_t lowerPixels = lowerHead ? *lowerHead : 0x00;

        upperHead++;
        if (lowerHead)
            lowerHead++;

        // Shift out 8 bits dengan dual data pins, MSB first
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            bool upperBit = upperPixels & (0x80 >> bit);
            bool lowerBit = lowerPixels & (0x80 >> bit);

            // Set data pins
            digitalWrite(stored_data_r1, upperBit ? HIGH : LOW);
            digitalWrite(stored_data_r2, lowerBit ? HIGH : LOW);

            // Clock pulse
            digitalWrite(stored_clock, HIGH);
            digitalWrite(stored_clock, LOW);
        }
    }
}

// Latch data menggunakan digitalWrite (Arduino Uno compatible)
void HUB08_Panel::latchDataFast()
{
    // Toggle latch pin
    digitalWrite(stored_latch, HIGH);
    digitalWrite(stored_latch, LOW);
}

// Enable/disable output menggunakan digitalWrite
void HUB08_Panel::enableOutputFast(bool enable)
{
    if (enable)
    {
        digitalWrite(stored_enable, LOW); // Set LOW (OE is active LOW)
    }
    else
    {
        digitalWrite(stored_enable, HIGH); // Set HIGH (disable output)
    }
}

// Clear physical display - Arduino Uno compatible
void HUB08_Panel::clearDisplay()
{
    // Disable output using PWM (255 = HIGH = disabled)
    analogWrite(stored_enable, 255); // OE disabled
    delayMicroseconds(5);

    for (uint16_t row = 0; row < maxRows; row++)
    {
        // Select row
        selectRowFast(row);

        // Shift out zeros menggunakan dual data pins
        uint16_t totalWidth = config.panel_width * config.chain_length;

        // Clear data pins
        digitalWrite(stored_data_r1, LOW);
        digitalWrite(stored_data_r2, LOW);

        // Shift out zeros dengan manual clock
        for (uint16_t bit = 0; bit < totalWidth; bit++)
        {
            // Clock pulse untuk shift data 0x00 (data pins sudah LOW)
            digitalWrite(stored_clock, HIGH);
            digitalWrite(stored_clock, LOW);
        }

        // Latch data
        latchDataFast();
        delayMicroseconds(1);
    }

    // Keep output disabled after clearing (PWM = 255)
    analogWrite(stored_enable, 255); // OE disabled
}

// Scan single row - Dual data pins (R1/R2) for 64x32 panel
void HUB08_Panel::scan()
{
    if (!initialized)
        return;

    static uint8_t row = 0; // from 0 to 15 (panel_scan)
    uint16_t totalWidth = config.panel_width * config.chain_length;

    // Calculate pointers for upper half (R1) and lower half (R2)
    uint8_t *upperHead = frameBuffer + row * (totalWidth / 8);
    uint8_t *lowerHead = nullptr;

    // For 64x32 panel: lower half is row + 16
    if (config.panel_height > config.panel_scan)
    {
        lowerHead = frameBuffer + (row + config.panel_scan) * (totalWidth / 8);
    }

    // Shift out data with DUAL data pins (R1 for upper, R2 for lower)
    for (uint8_t byte = 0; byte < (totalWidth / 8); byte++)
    {
        uint8_t upperPixels = *upperHead++;
        uint8_t lowerPixels = lowerHead ? *lowerHead++ : 0x00;

        // Shift out 8 bits, MSB first
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            bool upperBit = upperPixels & (0x80 >> bit);
            bool lowerBit = lowerPixels & (0x80 >> bit);

            // Set BOTH data pins (R1 and R2)
            digitalWrite(stored_data_r1, upperBit ? HIGH : LOW);
            digitalWrite(stored_data_r2, lowerBit ? HIGH : LOW);

            // Clock pulse
            digitalWrite(stored_clock, HIGH);
            digitalWrite(stored_clock, LOW);
        }
    }

    // CRITICAL: Wait for OE to go HIGH before changing rows
    uint16_t timeout = 2000;
    while (digitalRead(stored_enable) == LOW && --timeout > 0)
        ;

    // Set address pins for row selection
    digitalWrite(stored_addr_a, (row & 0x01) ? HIGH : LOW);
    if (addressBits >= 2)
        digitalWrite(stored_addr_b, (row & 0x02) ? HIGH : LOW);
    if (addressBits >= 3)
        digitalWrite(stored_addr_c, (row & 0x04) ? HIGH : LOW);
    if (addressBits >= 4)
        digitalWrite(stored_addr_d, (row & 0x08) ? HIGH : LOW);

    // CRITICAL: Double toggle latch
    digitalWrite(stored_latch, HIGH);
    digitalWrite(stored_latch, LOW);
    digitalWrite(stored_latch, HIGH);
    digitalWrite(stored_latch, LOW);

    // Next row (wraps from 15 back to 0)
    row = (row + 1) & (config.panel_scan - 1);
}

// Draw pixel (override from Adafruit_GFX)
void HUB08_Panel::drawPixel(int16_t x, int16_t y, uint16_t color)
{
    if (!initialized)
        return;

    // Bounds checking using runtime dimensions from Adafruit_GFX
    if (x < 0 || x >= this->width() || y < 0 || y >= this->height())
        return;

    // Calculate position PERSIS seperti HUB08SPI.cpp drawPoint()
    uint16_t totalWidth = config.panel_width * config.chain_length;
    uint8_t *byte = frameBuffer + x / 8 + y * (totalWidth / 8);
    uint8_t bit = x % 8;

    // SAMA seperti HUB08SPI.cpp: MSB first bit ordering (0x80 >> bit)
    if (color)
    {
        *byte |= (0x80 >> bit);
    }
    else
    {
        *byte &= ~(0x80 >> bit);
    }
}

// Fill screen (override from Adafruit_GFX)
void HUB08_Panel::fillScreen(uint16_t color)
{
    if (!initialized)
        return;

    // Fill buffer dengan pattern berdasarkan color
    uint8_t fillValue = color ? 0xFF : 0x00;
    memset(frameBuffer, fillValue, bufferSize);
}

// Clear screen (shortcut) - SAMA seperti HUB08SPI.cpp clear()
void HUB08_Panel::clearScreen()
{
    if (!initialized)
        return;

    // SAMA seperti HUB08SPI.cpp clear() method
    uint8_t *ptr = frameBuffer;
    for (uint16_t i = 0; i < (config.panel_width * config.chain_length * config.panel_height / 8); i++)
    {
        *ptr = 0x00;
        ptr++;
    }
}

// Set brightness - Arduino Mega dengan 32kHz PWM (as per HUB08SPI reference)
void HUB08_Panel::setBrightness(uint8_t brt)
{
    brightness = brt;

// Setup 32kHz PWM on pin 3 (Timer 3 on Mega) untuk flicker-free
// Reference: HUB08SPI.cpp line: TCCR2B = TCCR2B & 0b11111000 | 0x01;
#ifdef ARDUINO_AVR_MEGA2560
    TCCR3B = (TCCR3B & 0b11111000) | 0x01; // 32kHz PWM on pin 3
#endif

    // Gunakan dim_curve untuk brightness control
    uint8_t pwmValue = 255 - pgm_read_byte(&dim_curve[brt]);
    // Apply brightness langsung dengan PWM
    analogWrite(stored_enable, pwmValue);
}

// HUB08SPI compatibility methods
void HUB08_Panel::drawPoint(uint16_t x, uint16_t y, uint8_t color)
{
    drawPixel(x, y, color);
}

void HUB08_Panel::drawRect(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h, uint8_t color)
{
    // SAMA seperti HUB08SPI.cpp drawRect()
    for (uint16_t x = x1; x < x1 + w; x++)
    {
        for (uint16_t y = y1; y < y1 + h; y++)
        {
            drawPoint(x, y, color);
        }
    }
}

// Enhanced thick pixel drawing dengan bounds checking dan optimization
void HUB08_Panel::drawThickPixel(int16_t x, int16_t y, uint8_t thickness, uint16_t color)
{
    if (thickness == 0)
        return;

    // Optimization: single pixel case
    if (thickness == 1)
    {
        drawPixel(x, y, color);
        return;
    }

    // Bounds checking untuk thick pixel area
    int16_t maxX = x + thickness - 1;
    int16_t maxY = y + thickness - 1;

    // Early exit if completely outside bounds
    if (x >= this->width() || y >= this->height() ||
        maxX < 0 || maxY < 0)
    {
        return;
    }

    // Draw thick pixel as filled rectangle dengan optimized loops
    for (uint8_t dy = 0; dy < thickness; dy++)
    {
        int16_t pixelY = y + dy;
        if (pixelY >= 0 && pixelY < this->height())
        {
            for (uint8_t dx = 0; dx < thickness; dx++)
            {
                int16_t pixelX = x + dx;
                if (pixelX >= 0 && pixelX < this->width())
                {
                    drawPixel(pixelX, pixelY, color);
                }
            }
        }
    }
}

// Draw thick line dengan anti-aliasing effect
void HUB08_Panel::drawThickLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t thickness, uint16_t color)
{
    if (thickness == 0)
        return;

    // Use Bresenham's line algorithm dengan thickness
    int16_t dx = abs(x1 - x0);
    int16_t dy = abs(y1 - y0);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;

    int16_t x = x0, y = y0;

    while (true)
    {
        // Draw thick pixel at current position
        drawThickPixel(x - thickness / 2, y - thickness / 2, thickness, color);

        if (x == x1 && y == y1)
            break;

        int16_t e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y += sy;
        }
    }
}

// Draw thick rectangle dengan outline control
void HUB08_Panel::drawThickRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t thickness, uint16_t color, bool filled)
{
    if (thickness == 0 || w <= 0 || h <= 0)
        return;

    if (filled)
    {
        // Filled rectangle
        for (int16_t row = 0; row < h; row++)
        {
            for (int16_t col = 0; col < w; col++)
            {
                drawThickPixel(x + col, y + row, thickness, color);
            }
        }
    }
    else
    {
        // Outline only
        // Top and bottom lines
        for (int16_t col = 0; col < w; col++)
        {
            drawThickPixel(x + col, y, thickness, color);                 // Top
            drawThickPixel(x + col, y + h - thickness, thickness, color); // Bottom
        }

        // Left and right lines
        for (int16_t row = thickness; row < h - thickness; row++)
        {
            drawThickPixel(x, y + row, thickness, color);                 // Left
            drawThickPixel(x + w - thickness, y + row, thickness, color); // Right
        }
    }
}

// ===== ADAFRUIT GFX FONT METHODS =====

// Set font Adafruit (gunakan font dari library Adafruit GFX)
void HUB08_Panel::setAdafruitFont(const GFXfont *font)
{
    setFont(font); // Menggunakan method setFont dari Adafruit_GFX
}

// Print text menggunakan font Adafruit GFX dengan padding (simplified)
void HUB08_Panel::printAdafruitText(const char *text, int16_t x, int16_t y, uint16_t color, int16_t paddingX, int16_t paddingY)
{
    if (!text)
        return;

    // Apply padding to position
    setCursor(x + paddingX, y + paddingY);
    setTextColor(color);
    print(text);
    // Note: Text wrapping akan otomatis jika setTextWrap(true) dipanggil sebelumnya
}

// Print text terpusat menggunakan font Adafruit GFX dengan padding (simplified)
void HUB08_Panel::printAdafruitTextCentered(const char *text, int16_t y, uint16_t color, int16_t paddingX, int16_t paddingY)
{
    if (!text)
        return;

    int16_t textWidth = getAdafruitTextWidth(text);
    int16_t availableWidth = this->width() - (2 * paddingX);
    int16_t centerX = paddingX + (availableWidth - textWidth) / 2;

    setCursor(centerX, y + paddingY);
    setTextColor(color);
    print(text);
}

// Get text width menggunakan font Adafruit GFX
int16_t HUB08_Panel::getAdafruitTextWidth(const char *text)
{
    if (!text)
        return 0;

    int16_t x1, y1;
    uint16_t w, h;
    getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    return w;
}

// Get text height menggunakan font Adafruit GFX
int16_t HUB08_Panel::getAdafruitTextHeight(const char *text)
{
    if (!text)
        return 0;

    int16_t x1, y1;
    uint16_t w, h;
    getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    return h;
}

// (Scrolling animation removed) Horizontal scrolling code removed per
// user request. For text rendering use Adafruit_GFX calls directly.

// Color conversion (untuk compatibility)
uint16_t HUB08_Panel::color565(uint8_t r, uint8_t g, uint8_t b)
{
    return (r > 0 || g > 0 || b > 0) ? 1 : 0;
}

// Timer1 Compare A ISR untuk Arduino Uno & Mega
#if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_MEGA2560)
ISR(TIMER1_COMPA_vect)
{
    if (HUB08_Panel::instance)
    {
        HUB08_Panel::instance->scan();
    }
}
#endif