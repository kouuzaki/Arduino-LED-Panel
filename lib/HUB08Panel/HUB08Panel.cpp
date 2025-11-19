#include "HUB08Panel.h"

HUB08_Panel *HUB08_Panel::instance = nullptr;

// ISR Timer1 → hanya scan row
ISR(TIMER1_COMPA_vect)
{
    if (HUB08_Panel::instance)
        HUB08_Panel::instance->scan();
}

HUB08_Panel::HUB08_Panel(uint16_t w, uint16_t h, uint16_t chain)
    : Adafruit_GFX(w * chain, h)
{
    bufferSize = (w * chain * h) / 8;
    bufferFront = nullptr;
    bufferBack = nullptr;
    brightness = 255;
    initialized = false;
    instance = this;
}

bool HUB08_Panel::begin(const HUB08_Config &cfg)
{
    config = cfg;

    // Double buffer: 2x framebuffer
    uint8_t *raw = (uint8_t *)malloc(bufferSize * 2);
    if (!raw)
        return false;

    bufferFront = raw;
    bufferBack = raw + bufferSize;

    memset(bufferFront, 0, bufferSize);
    memset(bufferBack, 0, bufferSize);

    // Konfigurasi pin
    pinMode(cfg.data_pin_r1, OUTPUT); // D8  PB0
    pinMode(cfg.data_pin_r2, OUTPUT); // D9  PB1
    pinMode(cfg.clock_pin, OUTPUT);   // D10 PB2
    pinMode(cfg.latch_pin, OUTPUT);   // D11 PB3
    pinMode(cfg.enable_pin, OUTPUT);  // D3  PD3 (OC2B)

    pinMode(cfg.addr_a, OUTPUT); // A0 PC0
    pinMode(cfg.addr_b, OUTPUT); // A1 PC1
    pinMode(cfg.addr_c, OUTPUT); // A2 PC2
    pinMode(cfg.addr_d, OUTPUT); // A3 PC3

    // ========== Timer2: PWM untuk OE (pin 3 / OC2B) ==========
    // Pakai analogWrite sekali untuk set mode Fast PWM + COM2B
    analogWrite(cfg.enable_pin, 128); // nilai sembarang dulu

    // Ubah prescaler Timer2 ke 1 → ~31kHz PWM (lebih halus)
    TCCR2B = (TCCR2B & 0b11111000) | 0x01;

    // Set brightness awal via dim_curve (langsung tulis OCR2B)
    uint8_t dim = pgm_read_byte(&dim_curve[brightness]);
    // OE aktif LOW → nilai kecil = terang
    OCR2B = 255 - dim;

    initialized = true;

    // ========== Timer1: row refresh (scan) ==========
    // 16MHz / (1 * (1599+1)) = 10kHz row rate
    cli();
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;

    OCR1A = 1599;           // 10kHz
    TCCR1B |= (1 << WGM12); // CTC mode
    TCCR1B |= (1 << CS10);  // prescaler = 1
    TIMSK1 |= (1 << OCIE1A);
    sei();

    return true;
}

bool HUB08_Panel::begin(
    int8_t r1, int8_t r2, int8_t clk, int8_t lat, int8_t oe,
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
    cfg.panel_scan = scan; // 16 untuk 64x32 1/16 scan

    return begin(cfg);
}

// =======================
//        SCAN (ISR)
// =======================
// Catatan:
//   * OE dikendalikan PWM Timer2 di pin 3 (OCR2B).
//   * Di sini kita MATIKAN OE sementara shifting (OCR2B = 255),
//     lalu restore nilai sebelumnya.
//   * Tidak ada delay brightness di sini.
void HUB08_Panel::scan()
{
    if (!initialized)
        return;

    static uint8_t row = 0;

    uint8_t bytesPerRow = (config.panel_width * config.chain_length) / 8;

    uint8_t *upper = bufferFront + row * bytesPerRow;
    uint8_t *lower = nullptr;

    // Untuk 64x32 1/16 scan: lower row = row + 16
    if (config.panel_height >= (config.panel_scan * 2))
        lower = bufferFront + (row + config.panel_scan) * bytesPerRow;

    // ==== Matikan OE sementara shifting (supaya tidak nyala saat clock) ====
    uint8_t savedOCR2B = OCR2B;
    OCR2B = 255; // OE HIGH = OFF

    // ==== SHIFT DATA: R1/R2 di PB0/PB1, CLK di PB2 ====
    for (uint8_t i = 0; i < bytesPerRow; i++)
    {
        uint8_t ur = upper[i];
        uint8_t lr = lower ? lower[i] : 0x00;

        // Shift 8 bit, MSB dulu, pakai shift-left supaya cepat
        for (uint8_t b = 0; b < 8; b++)
        {
            uint8_t out = PORTB & 0b11111000; // clear PB0..PB2

            if (ur & 0x80)
                out |= (1 << 0); // PB0 → R1
            if (lr & 0x80)
                out |= (1 << 1); // PB1 → R2

            PORTB = out;

            // CLK HIGH–LOW pada PB2
            PORTB |= (1 << 2);
            PORTB &= ~(1 << 2);

            ur <<= 1;
            lr <<= 1;
        }
    }

    // Pastikan CLK LOW sebelum latch
    PORTB &= ~(1 << 2);

    // ==== SET ROW ADDRESS di PC0..PC3 (A, B, C, D) ====
    uint8_t pc = PORTC & 0xF0;
    pc |= (row & 0x0F);
    PORTC = pc;

    // beri sedikit waktu agar address settle
    asm volatile("nop\nnop\nnop\nnop");

    // ==== LATCH (PB3) ====
    PORTB |= (1 << 3);
    PORTB &= ~(1 << 3);

    // ==== Restore OE PWM ====
    OCR2B = savedOCR2B;

    // Next row (0..panel_scan-1)
    row++;
    if (row >= config.panel_scan)
        row = 0;
}

// =======================
//   GFX drawing (back)
// =======================

void HUB08_Panel::drawPixel(int16_t x, int16_t y, uint16_t c)
{
    if (!initialized)
        return;
    if (x < 0 || x >= width() || y < 0 || y >= height())
        return;

    uint16_t bytesPerRow = (config.panel_width * config.chain_length) / 8;
    uint8_t *ptr = bufferBack + y * bytesPerRow + (x >> 3);

    if (c)
        *ptr |= (0x80 >> (x & 7));
    else
        *ptr &= ~(0x80 >> (x & 7));
}

void HUB08_Panel::fillScreen(uint16_t c)
{
    if (!initialized)
        return;
    memset(bufferBack, c ? 0xFF : 0x00, bufferSize);
}

void HUB08_Panel::clearScreen()
{
    if (!initialized)
        return;
    memset(bufferBack, 0x00, bufferSize);
}

// =======================
//    Double-buffer swap
// =======================
void HUB08_Panel::swapBuffers(bool copyFrontToBack)
{
    if (!initialized)
        return;

    // Tukar pointer front/back secara atomik (hindari bentrok ISR)
    cli();
    uint8_t *tmp = bufferFront;
    bufferFront = bufferBack;
    bufferBack = tmp;
    sei();

    if (copyFrontToBack)
    {
        // opsional: jaga bufferBack tetap sama dengan front
        memcpy(bufferBack, bufferFront, bufferSize);
    }
}

// =======================
//       Brightness
// =======================
void HUB08_Panel::setBrightness(uint8_t b)
{
    brightness = b;
    if (!initialized)
        return;

    uint8_t dim = pgm_read_byte(&dim_curve[brightness]);
    // OE active LOW, jadi nilai PWM kecil = terang
    OCR2B = 255 - dim;
}

int16_t HUB08_Panel::getTextWidth(const String &text)
{
    int16_t x1, y1;
    uint16_t w, h;

    getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    return w;
}

int16_t HUB08_Panel::getTextHeight()
{
    int16_t x1, y1;
    uint16_t w, h;

    getTextBounds("Ay", 0, 0, &x1, &y1, &w, &h);
    return h;
}

void HUB08_Panel::drawTextMultilineCentered(const String &text)
{
    clearScreen();

    // ---- 1) Pisahkan baris berdasarkan '\n' ----
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

    // ---- 2) Hitung tinggi semua baris ----
    int16_t lineHeight = getTextHeight();
    if (lineHeight < 8)
        lineHeight = 8; // fallback

    int16_t totalHeight = lineCount * lineHeight;

    // ---- 3) Cari posisi Y supaya vertikal center ----
    int16_t startY = (height() - totalHeight) / 2 + lineHeight;

    // ---- 4) Render setiap baris dan center horizontal ----
    for (uint8_t i = 0; i < lineCount; i++)
    {
        int16_t w = getTextWidth(lines[i]);
        int16_t x = (width() - w) / 2;
        int16_t y = startY + i * lineHeight;

        setCursor(x, y);
        print(lines[i]);
    }

    // ---- 5) Swap buffer ----
    swapBuffers(true);
}
