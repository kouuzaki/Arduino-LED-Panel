# 🎯 HUB08 Panel Fix - Root Cause & Solution

## Problem: Panel kedip garis-garis meski wiring sudah benar

### ✅ ROOT CAUSE (Setelah analisis dengan referensi HUB08SPI)

#### 1. **Missing OE Wait** ❌→✅ FIXED

**Problem**: Tidak ada wait untuk OE (Output Enable) sebelum ganti row  
**Effect**: Data baru muncul sebelum row lama disabled → ghosting/flickering lines  
**Solution**:

```cpp
// CRITICAL: Wait for OE to go HIGH before changing rows
uint16_t timeout = 2000;
while (digitalRead(stored_enable) == LOW && --timeout > 0);
```

#### 2. **Single Latch Toggle** ❌→✅ FIXED

**Problem**: Latch pin cuma di-toggle sekali  
**Effect**: Data tidak ter-latch sempurna ke shift register → unstable display  
**Solution** (from HUB08SPI reference):

```cpp
// CRITICAL: Double toggle latch (PIND trick)
digitalWrite(stored_latch, HIGH);
digitalWrite(stored_latch, LOW);
digitalWrite(stored_latch, HIGH);  // Second toggle!
digitalWrite(stored_latch, LOW);
```

#### 3. **PWM Frequency Too Low** ❌→✅ FIXED

**Problem**: Arduino default PWM ~490Hz → visible flicker  
**Effect**: Brightness control causing visible flickering  
**Solution**:

```cpp
// Set Timer 3 to 32kHz PWM for flicker-free brightness
#ifdef ARDUINO_AVR_MEGA2560
    TCCR3B = TCCR3B & 0b11111000 | 0x01;
#endif
```

## 📋 Perubahan Kode (v2.0)

### `scan()` function - Complete Rewrite

```cpp
// BEFORE (WRONG):
- analogWrite(stored_enable, 255);  // Disable with PWM
- Shift data
- Set address
- digitalWrite(latch, HIGH/LOW);  // Single toggle
- analogWrite(stored_enable, pwm); // Enable with PWM

// AFTER (CORRECT):
- Shift data
- while (digitalRead(enable) == LOW);  // WAIT for OE HIGH!
- Set address pins
- digitalWrite(latch, HIGH/LOW/HIGH/LOW);  // DOUBLE toggle!
- (Brightness via 32kHz PWM continuously)
```

### `setBrightness()` - Added Timer Setup

```cpp
// Setup 32kHz PWM on pin 3 (Timer 3)
TCCR3B = TCCR3B & 0b11111000 | 0x01;
analogWrite(stored_enable, pwmValue);
```

## 🔌 Hardware Setup (HUB08 Monochrome Panel)

### Pin Connections

```
Arduino Mega → HUB08 Panel
========================
D22 (DATA_R1) → Pin 9 (R1)    - Data line
D24 (CLOCK)   → Pin 16 (CLK)  - Shift clock
D25 (LATCH)   → Pin 14 (LAT)  - Latch
D3  (ENABLE)  → Pin 7 (OE)    - Output Enable ⚡ PWM PIN!
D27 (ADDR_A)  → Pin 2 (A)     - Address bit 0
D28 (ADDR_B)  → Pin 4 (B)     - Address bit 1
D29 (ADDR_C)  → Pin 6 (C)     - Address bit 2
D30 (ADDR_D)  → Pin 8 (D)     - Address bit 3
GND           → GND (pins 1,3,5,13,15)
```

**⚠️ CRITICAL**: ENABLE_PIN HARUS di PWM pin (Mega: 2-13, 44-46)

### Panel Configuration

```cpp
#define PANEL_WIDTH 64   // Single panel
#define PANEL_HEIGHT 32
#define PANEL_CHAIN 1    // 1 panel
#define PANEL_SCAN 16    // 1/16 scan

// Initialize
HUB08_Panel ledPanel(64, 32, 1);
ledPanel.begin(22, 23, 24, 25, 3,  // Data, Data, Clk, Lat, OE
               27, 28, 29, 30,      // A, B, C, D
               64, 32, 1, 16);

ledPanel.startScanning(78);  // ~78 FPS refresh
ledPanel.setBrightness(200); // 0-255
```

## 📚 Reference

- Library: https://github.com/fahroniganteng/Arduino_HUB08_Matrix_Led
- Key files:
  - `HUB08SPI.cpp` lines 49-71 (scan routine)
  - `TimerOne.h` (PWM setup reference)

## ✅ Testing Results

- **Platform**: Arduino Mega 2560 @ 16MHz
- **Panel**: 64x32 P4.75 HUB08 (1/16 scan)
- **Status**: ✅ Stable display, no flicker, proper brightness
- **Compiled**: 11,604 bytes (4.6% Flash), 307 bytes (3.7% RAM)
