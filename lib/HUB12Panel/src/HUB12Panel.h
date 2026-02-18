#ifndef HUB12_PANEL_H
#define HUB12_PANEL_H

#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <avr/pgmspace.h>

#define OE_ACTIVE_LOW true

struct HUB12_Config {
  int8_t r, clk, lat, oe, a, b;
  uint16_t width, height, chain;
};

class HUB12_Panel : public Adafruit_GFX {
public:
  static HUB12_Panel *instance;

private:
  HUB12_Config config;
  uint8_t *bufferFront;  // ISR reads this (displayed buffer)
  uint8_t *bufferBack;   // CPU writes to this (drawing buffer)
  uint16_t bufferSize;
  volatile bool initialized;
  uint8_t brightness;
  
  // Untuk running text
  String scrollText;
  int16_t scrollX;
  uint16_t scrollSpeed;  // pixel per frame (default: 1)
  unsigned long lastScrollTime;
  bool isScrolling;

public:
  HUB12_Panel(uint16_t w, uint16_t h, uint16_t chain = 1);
  bool begin(int8_t r, int8_t clk, int8_t lat, int8_t oe, int8_t a, int8_t b,
             uint16_t w = 32, uint16_t h = 16, uint16_t chain = 1);
  void scan();
  void setBrightness(uint8_t b);
  void drawPixel(int16_t x, int16_t y, uint16_t c) override;
  void fillScreen(uint16_t c) override;
  void clearScreen();

  // Helper
  void drawTextCentered(const String &text);
  int16_t getTextWidth(const String &text);
  int16_t getTextHeight();
  void drawTextMultilineCentered(const String &text);
  
  // Running text
  void startScrolling(const String &text, uint16_t speed = 1);
  void stopScrolling();
  void updateScrolling();  // call this di loop utama
  bool getScrollingStatus() const { return isScrolling; }  // getter untuk isScrolling
  
  void swapBuffers(bool copyFrontToBack = false);
};

#endif