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
  buffer = nullptr;
  instance = this;
}

bool HUB12_Panel::begin(int8_t r, int8_t clk, int8_t lat, int8_t oe, int8_t a,
                        int8_t b, uint16_t w, uint16_t h, uint16_t chain) {
  config = {r, clk, lat, oe, a, b, w, h, chain};
  if (buffer)
    free(buffer);
  buffer = (uint8_t *)malloc(bufferSize);
  if (!buffer)
    return false;
  memset(buffer, 0, bufferSize); // Clear buffer (Black)

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
  OCR1A = 1600;  // ~625Hz ISR for stable, proven refresh
  TCCR1B |= (1 << WGM12) | (1 << CS11);
  TIMSK1 |= (1 << OCIE1A);
  sei();

  initialized = true;
  return true;
}

void HUB12_Panel::scan() {
  if (!initialized)
    return;
  static uint8_t row = 0;

// 1. Turn off Output (Blanking)
#if defined(__AVR_ATmega2560__)
  uint8_t currentPWM = OCR3C;
  // If Active Low, 255 = Off. If Active High, 0 = Off.
  OCR3C = OE_ACTIVE_LOW ? 255 : 0;
#endif

  // 2. Select Row (A & B)
  uint8_t portF = PORTF;
  portF &= 0xFC;
  portF |= (row & 0x03);
  PORTF = portF;

  // 3. Shift Data (FIX: Color Inversion & Row Order)
  int bytesPerPanel = 4;
  int totalWidthBytes = config.chain * bytesPerPanel;

  // Loop setiap kolom byte
  for (int i = 0; i < totalWidthBytes; i++) {

    // Get data for 4 scan rows
    // Fix Color Inversion & Row Order
    // This will invert 0 to 1 (On) and 1 to 0 (Off)
    // Fix "Background Bright, Font Dark" issue

    uint8_t b0 = ~buffer[i + ((row + 0) * totalWidthBytes)];
    uint8_t b1 = ~buffer[i + ((row + 4) * totalWidthBytes)];
    uint8_t b2 = ~buffer[i + ((row + 8) * totalWidthBytes)];
    uint8_t b3 = ~buffer[i + ((row + 12) * totalWidthBytes)];

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

    // Fix Broken Font
    // Standard DMD order is 12 -> 8 -> 4 -> 0 (b3, b2, b1, b0)
    // If still broken, try changing to (b0, b1, b2, b3)
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

  row = (row + 1) & 0x03;
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
  int idx = (y * (width() / 8)) + (x / 8);
  if (c)
    buffer[idx] |= (0x80 >> (x & 7));
  else
    buffer[idx] &= ~(0x80 >> (x & 7));
}

void HUB12_Panel::fillScreen(uint16_t c) {
  memset(buffer, c ? 0xFF : 0x00, bufferSize);
}

void HUB12_Panel::clearScreen() { memset(buffer, 0, bufferSize); }

void HUB12_Panel::drawTextCentered(const String &text) {
  clearScreen();
  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int16_t centerX = (width() - w) / 2;
  int16_t centerY = (height() - h) / 2 - y1 + 2;  // Extra +2 offset to prevent pixel overflow
  if (centerY < 0) centerY = 0;  // Clamp to prevent negative Y
  setCursor(centerX, centerY);
  print(text);
  // Single-buffer ISR @ 625Hz: immediate display update
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

  // Tight line spacing for responsive display
  int16_t lineSpacing = fontHeight + 1; // Small gap between lines
  int16_t totalHeight = (lineCount - 1) * lineSpacing + fontHeight;

  // Center vertically with some margin
  int16_t verticalMargin = (height() - totalHeight) / 2;
  if (verticalMargin < 0)
    verticalMargin = 0; // Prevent negative margin

  // Draw each line centered
  for (uint8_t i = 0; i < lineCount; i++) {
    int16_t lineWidth = getTextWidth(lines[i]);
    int16_t centerX = (width() - lineWidth) / 2;
    if (centerX < 0)
      centerX = 0; // Safety for long text

    int16_t x1, y1;
    uint16_t w, h;
    getTextBounds(lines[i], 0, 0, &x1, &y1, &w, &h);
    int16_t lineY = verticalMargin + i * lineSpacing - y1 + 2;  // Extra +2 to prevent pixel overflow
    if (lineY < 0) lineY = 0;  // Clamp to prevent negative Y
    setCursor(centerX, lineY);
    print(lines[i]);
  }
  
  // No explicit buffer swap needed - ISR updates continuously at 625Hz
}

void HUB12_Panel::swapBuffers(bool copyFrontToBack) {
  (void)copyFrontToBack;
  // Single-buffer ISR driver: continuous 625Hz scan updates display immediately.
  // No buffer swap needed—changes visible within ~2ms of any drawPixel/print call.
}