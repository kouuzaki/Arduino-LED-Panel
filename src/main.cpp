#include <Arduino.h>
#include "HUB08Panel.h"
#include <fonts/Roboto_Bold_15.h>

// Mapping hardware (disesuaikan sama wiring kamu)
#define R1 8
#define R2 9
#define CLK 10
#define LAT 11
#define OE 3 // OE → WAJIB ke D3 (OC2B / Timer2 PWM)

#define A A0
#define B A1
#define C A2
#define D A3

HUB08_Panel display(64, 32, 1);

void setup()
{
    Serial.begin(115200);

    // Inisialisasi panel 64x32, 1 panel, 1/16 scan
    display.begin(R1, R2, CLK, LAT, OE, A, B, C, D, 64, 32, 1, 16);

    // Brightness max
    display.setBrightness(255);

    // Gambar ke BACK buffer
    display.clearScreen();
    display.setFont(&Roboto_Bold_15);
    display.setTextSize(1); // kecil
    display.drawTextMultilineCentered("MAJU\nLAGI...");

    // Setelah selesai gambar → swap ke FRONT
    display.swapBuffers(true); // true = copy front → back (biar back sync)
}

void loop()
{
    // Tidak perlu apa-apa, refresh di ISR Timer1
}
