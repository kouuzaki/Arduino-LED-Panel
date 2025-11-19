# Pin Mapping - Arduino Mega 2560 untuk HUB08 Panel LED

## ⚡ **PENTING: ENABLE PIN HARUS PWM!**

### Masalah Umum:

- ❌ **Brightness tidak bekerja** → ENABLE pin bukan PWM pin
- ❌ **Panel kedip/flicker** → Pin salah atau wiring longgar
- ❌ **Garis-garis random** → Power supply tidak cukup

---

## ✅ Pin Configuration yang Benar

### Arduino Mega 2560 → HUB08 Panel

| HUB08 Pin     | Function      | Arduino Mega | Pin Type   | Notes         |
| ------------- | ------------- | ------------ | ---------- | ------------- |
| **R1**        | Data Upper    | **D22**      | Digital    | Rows 0-15     |
| **R2**        | Data Lower    | **D23**      | Digital    | Rows 16-31    |
| **CLK/S**     | Shift Clock   | **D24**      | Digital    | SPI Clock     |
| **LAT/L/STB** | Latch         | **D25**      | Digital    | Latch pulse   |
| **OE/EN**     | Output Enable | **D3**       | **⚡ PWM** | **CRITICAL!** |
| **A/LA**      | Address A     | **D27**      | Digital    | LSB           |
| **B/LB**      | Address B     | **D28**      | Digital    | Bit 1         |
| **C/LC**      | Address C     | **D29**      | Digital    | Bit 2         |
| **D/LD**      | Address D     | **D30**      | Digital    | MSB           |
| **GND**       | Ground        | **GND**      | Power      | Common GND!   |
| **VCC**       | Power 5V      | **Ext PSU**  | Power      | Min 2A/panel  |

### Kode Implementation:

```cpp
// main.cpp
#define DATA_PIN_R1 22  // Upper half
#define DATA_PIN_R2 23  // Lower half
#define CLOCK_PIN 24
#define LATCH_PIN 25
#define ENABLE_PIN 3    // ⚡ PWM PIN - MANDATORY!
#define ADDR_A 27
#define ADDR_B 28
#define ADDR_C 29
#define ADDR_D 30
```

---

## 🔌 Wiring Diagram

```
Arduino Mega 2560          HUB08 Panel (Input)
┌──────────────┐          ┌─────────────────┐
│              │          │                 │
│   D22 ───────┼─────────▶│ R1 (Data Upper) │
│   D23 ───────┼─────────▶│ R2 (Data Lower) │
│   D24 ───────┼─────────▶│ CLK (Clock)     │
│   D25 ───────┼─────────▶│ LAT (Latch)     │
│   D3  ───────┼─────────▶│ OE  (Enable)    │◀─ PWM!
│   D27 ───────┼─────────▶│ A   (Addr LSB)  │
│   D28 ───────┼─────────▶│ B               │
│   D29 ───────┼─────────▶│ C               │
│   D30 ───────┼─────────▶│ D   (Addr MSB)  │
│   GND ───────┼─────────▶│ GND             │
│              │          │                 │
└──────────────┘          └─────────────────┘
                                 ▲
                                 │
                          5V PSU (2A min)
```

---

## 💡 PWM Pins di Arduino Mega

### Available PWM Pins:

- **2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13** (Timer1, Timer2, Timer3, Timer4)
- **44, 45, 46** (Timer5)

### Pin 3 dipilih karena:

1. ✅ PWM-capable (Timer 3 - OC3B)
2. ✅ Sesuai dengan referensi library HUB08SPI
3. ✅ Tidak bentrok dengan SPI hardware pins
4. ✅ Accessible di header Arduino Mega

---

## 🔋 Power Requirements

### LED Panel P4.75 64x32:

- **Voltage**: 5V DC
- **Current per panel**:
  - Idle: ~200mA
  - Max (all LEDs ON): ~2A
- **2 Panels chained**: **4A total** (worst case)

### ⚠️ JANGAN gunakan 5V dari Arduino!

```
WRONG ❌:
Panel VCC → Arduino 5V pin  // Arduino akan rusak!

CORRECT ✅:
Panel VCC → External 5V PSU (min 4A)
Panel GND → PSU GND + Arduino GND (common ground)
```

---

## 🧪 Quick Test

### Test 1: Pin OE PWM

```cpp
void setup() {
    pinMode(3, OUTPUT);

    // Test PWM sweep
    for(int i = 0; i < 256; i++) {
        analogWrite(3, i);
        delay(10);
    }
}
```

**Expected**: Panel brightness berubah smooth dari gelap → terang

### Test 2: Basic Display

```cpp
ledPanel.begin(...);
ledPanel.startScanning(78);
ledPanel.fillScreen(1);  // All LEDs ON
ledPanel.setBrightness(100);
```

**Expected**: Panel menyala merah penuh dengan brightness 100/255

---

## 🐛 Troubleshooting

### Panel tidak menyala sama sekali:

- [ ] Check power supply (min 5V 2A)
- [ ] Verify common ground (Arduino GND ↔ PSU GND)
- [ ] Test ENABLE pin: `digitalWrite(ENABLE_PIN, LOW);` → panel harus ON

### Brightness tidak bisa diatur:

- [ ] **ENABLE_PIN menggunakan PWM pin?** (Pin 3, bukan 6!)
- [ ] Call `setBrightness()` SETELAH `startScanning()`
- [ ] Test manual: `analogWrite(ENABLE_PIN, 128);`

### Garis-garis kedip:

- [ ] Power supply voltage drop? (check dengan multimeter)
- [ ] Address pins wiring benar? (A=27, B=28, C=29, D=30)
- [ ] Scan frequency terlalu tinggi? Coba: `startScanning(50);`

### Data corrupt/salah:

- [ ] R1/R2 pins tertukar?
- [ ] Clock/Latch pins wiring benar?
- [ ] Kabel terlalu panjang? (max 30cm recommended)

---

## 📚 References

- Arduino Mega PWM: https://www.arduino.cc/reference/en/language/functions/analog-io/analogwrite/
- HUB08 Protocol: 1/16 scan multiplexing
- Library Reference: https://github.com/fahroniganteng/Arduino_HUB08_Matrix_Led
- Spec Panel: P4.75 SMD Indoor 64x32 HUB08

---

## ✅ Checklist Before Upload

- [ ] ENABLE_PIN = 3 (PWM pin)
- [ ] All pins defined correctly (R1=22, R2=23, etc.)
- [ ] External 5V PSU connected dan ON
- [ ] Common GND: Arduino ↔ PSU ↔ Panel
- [ ] Panel power LED menyala (jika ada)
- [ ] Kabel data rapi dan tidak longgar
