#ifndef HUB08_PANEL_H
#define HUB08_PANEL_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <avr/pgmspace.h>

// Gamma / brightness curve (sama seperti HUB08SPI)
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

struct HUB08_Config
{
  int8_t data_pin_r1;
  int8_t data_pin_r2;
  int8_t clock_pin;
  int8_t latch_pin;
  int8_t enable_pin; // OE (active LOW) → D3

  int8_t addr_a;
  int8_t addr_b;
  int8_t addr_c;
  int8_t addr_d;

  uint16_t panel_width;
  uint16_t panel_height;
  uint16_t chain_length;
  uint8_t panel_scan; // 16 untuk 64x32 1/16 scan
};

enum
{
  HUB08_ALIGN_LEFT = 0,
  HUB08_ALIGN_CENTER = 1,
  HUB08_ALIGN_RIGHT = 2
};

class HUB08_Panel : public Adafruit_GFX
{
public:
  static HUB08_Panel *instance;

private:
  HUB08_Config config;

  uint8_t *bufferFront; // dipakai oleh ISR scan()
  uint8_t *bufferBack;  // dipakai untuk gambar (drawPixel)
  uint16_t bufferSize;

  uint8_t brightness;
  bool initialized;

public:
  HUB08_Panel(uint16_t width, uint16_t height, uint16_t chain = 1);

  bool begin(const HUB08_Config &cfg);
  bool begin(int8_t r1, int8_t r2, int8_t clk, int8_t lat, int8_t oe,
             int8_t A, int8_t B, int8_t C, int8_t D,
             uint16_t width = 64, uint16_t height = 32, uint16_t chain = 1,
             uint8_t scan = 16);

  int16_t getTextWidth(const String &text);
  int16_t getTextHeight();
  void drawTextMultilineCentered(const String &text);

  // Dipanggil dari ISR Timer1
  void scan();

  // Adafruit_GFX overrides → tulis ke bufferBack (double buffer)
  void drawPixel(int16_t x, int16_t y, uint16_t c) override;
  void fillScreen(uint16_t c) override;

  // Clear back-buffer
  void clearScreen();

  // Swap front/back buffer (opsional copy)
  void swapBuffers(bool copyFrontToBack = false);

  // Brightness 0..255 (via Timer2 PWM, OE aktif LOW)
  void setBrightness(uint8_t b);
};

#endif
