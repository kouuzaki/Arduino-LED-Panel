#include "HUB12Panel.h"

HUB12_Panel *HUB12_Panel::instance = nullptr;

const uint8_t dim_curve[] PROGMEM = {
    0,   1,   1,   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   3,
    3,   3,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   4,   4,
    4,   4,   4,   4,   4,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,
    6,   6,   6,   6,   6,   6,   6,   6,   7,   7,   7,   7,   7,   7,   7,
    8,   8,   8,   8,   8,   8,   9,   9,   9,   9,   9,   9,   10,  10,  10,
    10,  10,  11,  11,  11,  11,  11,  12,  12,  12,  12,  12,  13,  13,  13,
    13,  14,  14,  14,  14,  15,  15,  15,  16,  16,  16,  16,  17,  17,  17,
    18,  18,  18,  19,  19,  19,  20,  20,  20,  21,  21,  22,  22,  22,  23,
    23,  24,  24,  25,  25,  25,  26,  26,  27,  27,  28,  28,  29,  29,  30,
    30,  31,  32,  32,  33,  33,  34,  35,  35,  36,  36,  37,  38,  38,  39,
    40,  40,  41,  42,  43,  43,  44,  45,  46,  47,  48,  48,  49,  50,  51,
    52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,
    68,  69,  70,  71,  73,  74,  75,  76,  78,  79,  81,  82,  83,  85,  86,
    88,  90,  91,  93,  94,  96,  98,  99,  101, 103, 105, 107, 109, 110, 112,
    114, 116, 118, 121, 123, 125, 127, 129, 132, 134, 136, 139, 141, 144, 146,
    149, 151, 154, 157, 159, 162, 165, 168, 171, 174, 177, 180, 183, 186, 190,
    193, 196, 200, 203, 207, 211, 214, 218, 222, 226, 230, 234, 238, 242, 247,
    252};

ISR(TIMER1_COMPA_vect) {
  if (HUB12_Panel::instance)
    HUB12_Panel::instance->scan();
}

HUB12_Panel::HUB12_Panel(uint16_t w, uint16_t h, uint16_t chain)
    : Adafruit_GFX(w * chain, h) {
  config.width = w;
  config.height = h;
  config.chain = chain;
  bufferSize = (w * chain * h) / 8;
  bufferFront = nullptr;
  bufferBack = nullptr;
  instance = this;
}

bool HUB12_Panel::begin(int8_t r, int8_t clk, int8_t lat, int8_t oe, int8_t a,
                        int8_t b, uint16_t w, uint16_t h, uint16_t chain) {
  config = {r, clk, lat, oe, a, b, w, h, chain};
  // Allocate double buffer (front + back)
  uint8_t *raw = (uint8_t *)malloc(bufferSize * 2);
  if (!raw)
    return false;
  bufferFront = raw;
  bufferBack = raw + bufferSize;
  memset(bufferFront, 0, bufferSize); // Clear (Black)
  memset(bufferBack, 0, bufferSize);

  pinMode(r, OUTPUT);
  pinMode(clk, OUTPUT);
  pinMode(lat, OUTPUT);
  pinMode(oe, OUTPUT);
  pinMode(a, OUTPUT);
  pinMode(b, OUTPUT);

#if defined(__AVR_ATmega2560__)
  TCCR3A = (1 << COM3C1) | (1 << WGM30);
  TCCR3B = (1 << WGM32) | (1 << CS30);
#endif
  setBrightness(128);

  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 1600; // ~625Hz ISR for stable, proven refresh
  TCCR1B |= (1 << WGM12) | (1 << CS11);
  TIMSK1 |= (1 << OCIE1A);
  sei();

  initialized = true;
  return true;
}

void HUB12_Panel::scan() {
  if (!initialized)
    return;
  static uint8_t scanRow = 0;

// 1. Turn off Output (Blanking)
#if defined(__AVR_ATmega2560__)
  uint8_t currentPWM = OCR3C;
  OCR3C = OE_ACTIVE_LOW ? 255 : 0;
#endif

  // 2. Select Row (A & B) - for HUB12 1/4 scan
  // scanRow 0-3 selects which 4 rows to display
  uint8_t portF = PORTF;
  portF &= 0xFC; // Clear A0, A1 bits
  portF |= (scanRow & 0x03);
  PORTF = portF;

  // 3. Shift Data for 4 rows simultaneously
  // HUB12: width(32*chain) pixels per row = 4 bytes
  int bytesPerPanel = 4;
  int totalWidthBytes = config.chain * bytesPerPanel;

  for (int i = 0; i < totalWidthBytes; i++) {
    // For 1/4 scan: scanRow selects which set of 4 rows (0-3)
    // Each byte contains data for rows: scanRow, scanRow+4, scanRow+8,
    // scanRow+12 within the full 16-row height
    uint8_t row0 = scanRow;     // rows 0, 4, 8, 12
    uint8_t row1 = scanRow + 4; // corresponding to scan pattern
    uint8_t row2 = scanRow + 8;
    uint8_t row3 = scanRow + 12;

    // Calculate buffer indices for linear buffer layout
    uint8_t b0 = (row0 < config.height)
                     ? ~bufferFront[i + (row0 * totalWidthBytes)]
                     : 0xFF;
    uint8_t b1 = (row1 < config.height)
                     ? ~bufferFront[i + (row1 * totalWidthBytes)]
                     : 0xFF;
    uint8_t b2 = (row2 < config.height)
                     ? ~bufferFront[i + (row2 * totalWidthBytes)]
                     : 0xFF;
    uint8_t b3 = (row3 < config.height)
                     ? ~bufferFront[i + (row3 * totalWidthBytes)]
                     : 0xFF;

    // Helper ShiftOut (MSB First)
    auto shiftOutByte = [&](uint8_t val) {
      for (int k = 0; k < 8; k++) {
        // R (Pin 5 - PE3)
        if (val & 0x80)
          PORTE |= (1 << 3);
        else
          PORTE &= ~(1 << 3);
        // CLK (Pin 7 - PH4)
        PORTH |= (1 << 4);
        PORTH &= ~(1 << 4);
        val <<= 1;
      }
    };

    // Standard DMD order: 12 -> 8 -> 4 -> 0 (b3, b2, b1, b0)
    shiftOutByte(b3);
    shiftOutByte(b2);
    shiftOutByte(b1);
    shiftOutByte(b0);
  }

  // 4. Latch
  PORTH |= (1 << 5);
  PORTH &= ~(1 << 5);

// 5. Restore Brightness
#if defined(__AVR_ATmega2560__)
  OCR3C = currentPWM;
#endif

  // Advance to next scan row (0->1->2->3->0)
  scanRow = (scanRow + 1) & 0x03;
}

void HUB12_Panel::setBrightness(uint8_t b) {
  brightness = b;
  if (!initialized)
    return;
  uint8_t pwm = pgm_read_byte(&dim_curve[b]);

#if defined(__AVR_ATmega2560__)
  // Logika PWM
  if (OE_ACTIVE_LOW)
    OCR3C = 255 - pwm;
  else
    OCR3C = pwm;
#endif
}

// Grafis standar GFX
void HUB12_Panel::drawPixel(int16_t x, int16_t y, uint16_t c) {
  if (x < 0 || x >= width() || y < 0 || y >= height())
    return;
  // Buffer layout: linear row-by-row for full height
  // For HUB12 1/4 scan: height=16, but scan 4 rows at once
  uint16_t bytesPerRow = (width() / 8);
  int idx = (y * bytesPerRow) + (x / 8);
  // Write to BACK buffer only (CPU-safe, ISR reads FRONT)
  if (c)
    bufferBack[idx] |= (0x80 >> (x & 7));
  else
    bufferBack[idx] &= ~(0x80 >> (x & 7));
}

void HUB12_Panel::fillScreen(uint16_t c) {
  memset(bufferBack, c ? 0xFF : 0x00, bufferSize);
}

void HUB12_Panel::clearScreen() { memset(bufferBack, 0, bufferSize); }

void HUB12_Panel::drawTextCentered(const String &text) {
  clearScreen();
  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

  // Precise centering using actual text bounds
  // y1 = top of bbox relative to cursor (negative for ascenders)
  // Formula: cursorY + y1 = (height - h) / 2  →  cursorY = (height - h) / 2 -
  // y1
  int16_t centerX = (width() - w) / 2;
  int16_t centerY = (height() - h) / 2 - y1;

  setCursor(centerX, centerY);
  print(text);
  swapBuffers(true);
}

int16_t HUB12_Panel::getTextWidth(const String &text) {
  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  return w;
}

int16_t HUB12_Panel::getTextHeight() {
  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds("Ay", 0, 0, &x1, &y1, &w, &h);
  return h;
}

void HUB12_Panel::drawTextMultilineCentered(const String &text) {
  clearScreen();

  const uint8_t MAX_LINES = 8;
  String lines[MAX_LINES];
  uint8_t lineCount = 0;
  String current = "";

  // Parse text by newline
  for (uint16_t i = 0; i <= text.length(); i++) {
    char c = text[i];
    if (c == '\n' || c == '\0') {
      if (current.length() > 0 && lineCount < MAX_LINES) {
        lines[lineCount++] = current;
      }
      current = "";
    } else {
      current += c;
    }
  }

  if (lineCount == 0)
    return;

  // Calculate text metrics
  int16_t fontHeight = getTextHeight();
  if (fontHeight < 7)
    fontHeight = 7; // Minimum 5x7 font

  int16_t lineSpacing = fontHeight + 1;
  int16_t totalHeight = (lineCount - 1) * lineSpacing + fontHeight;

  // Center vertically
  int16_t verticalMargin = (height() - totalHeight) / 2;
  if (verticalMargin < 0)
    verticalMargin = 0;

  // Draw each line centered
  for (uint8_t i = 0; i < lineCount; i++) {
    int16_t lineWidth = getTextWidth(lines[i]);
    int16_t centerX = (width() - lineWidth) / 2;
    if (centerX < 0)
      centerX = 0;

    // Use actual text bounds for precise Y positioning per line
    int16_t x1, y1;
    uint16_t w, h;
    getTextBounds(lines[i], 0, 0, &x1, &y1, &w, &h);

    // Top of this line's bbox should be at: verticalMargin + i * lineSpacing
    // cursorY + y1 = verticalMargin + i * lineSpacing
    // cursorY = verticalMargin + i * lineSpacing - y1
    int16_t lineY = verticalMargin + i * lineSpacing - y1;
    setCursor(centerX, lineY);
    print(lines[i]);
  }

  // Swap buffers atomically to display rendered text
  swapBuffers(true);
}

void HUB12_Panel::swapBuffers(bool copyFrontToBack) {
  if (!initialized)
    return;

  // Swap pointers AND optional memcpy ATOMICALLY (disable interrupts to prevent
  // ISR contention)
  cli();
  uint8_t *tmp = bufferFront;
  bufferFront = bufferBack;
  bufferBack = tmp;

  // CRITICAL: memcpy harus atomic (inside cli/sei)
  // ISR tidak boleh interrupt saat data sedang di-copy
  if (copyFrontToBack) {
    memcpy(bufferBack, bufferFront, bufferSize);
  }

  sei(); // Enable Interrupt SETELAH memcpy selesai
}
// ==========================================================
// RUNNING TEXT / SCROLLING IMPLEMENTATION
// ==========================================================

void HUB12_Panel::startScrolling(const String &text, uint16_t speed) {
  scrollText = text;
  scrollSpeed = (speed > 0) ? speed : 1;
  scrollX = width();

  // *** PENTING: Matikan Text Wrap ***
  // Tanpa ini, teks akan terbelah saat sebagian di luar layar
  setTextWrap(false);

  isScrolling = true;
  lastScrollTime = millis();
}

void HUB12_Panel::stopScrolling() {
  isScrolling = false;
  scrollText = "";
  scrollX = 0;
}

void HUB12_Panel::updateScrolling() {
  if (!isScrolling || scrollText.length() == 0)
    return;

  unsigned long now = millis();
  unsigned long elapsed = now - lastScrollTime;

  if (elapsed >= 40) {
    scrollX -= scrollSpeed;
    lastScrollTime = now;

    int16_t textWidth = getTextWidth(scrollText);

    // Reset loop (+ buffer 5px biar bersih baru muncul lagi)
    if (scrollX < -(textWidth + 5)) {
      scrollX = width();
    }

    clearScreen();

    // Hitung Center Y yang Presisi
    int16_t x1, y1;
    uint16_t w, h;
    getTextBounds(scrollText, 0, 0, &x1, &y1, &w, &h);

    // Precise vertical centering using actual text bounds
    int16_t y = (height() - h) / 2 - y1;

    setCursor(scrollX, y);
    print(scrollText);

    // Swap dengan copy untuk prevent torn reads saat scrolling
    swapBuffers(true);
  }
}