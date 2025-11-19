# Troubleshooting Guide - Panel LED Kedip Garis-Garis

## Masalah: Panel menampilkan garis-garis yang kedip

### Penyebab & Solusi

#### 1. **Output Enable (OE) Timing Issue** ✅ FIXED

- **Masalah**: OE pin tidak di-sync dengan refresh cycle
- **Solusi**: Disable output sebelum shift data, enable setelah latch
- **Di kode**: `scan()` function sekarang mengikuti urutan:
  ```
  1. Disable output (OE = HIGH)
  2. Shift data ke shift register
  3. Set address pins
  4. Latch data
  5. Enable output dengan brightness PWM
  ```

#### 2. **Address Pin Timing** ✅ FIXED

- **Masalah**: Address pins di-set SETELAH latch, menyebabkan row selection tidak stabil
- **Solusi**: Address pins sekarang di-set SEBELUM latch
- **Urutan yang benar**: Data shift → Address set → Latch → Enable

#### 3. **PWM Brightness** ✅ FIXED

- **Masalah**: `analogWrite()` dipanggil saat setup, menyebabkan inconsistent brightness
- **Solusi**: Brightness PWM sekarang di-apply setiap scan cycle setelah latch
- **Benefit**: Brightness stabil dan tidak flicker

### Pengaturan Panel yang Benar

Untuk Arduino Mega (ATmega2560):

```cpp
HUB08_Panel ledPanel(128, 32, 2);  // 128x32, 2 panels

ledPanel.begin(DATA_PIN_R1, DATA_PIN_R2, CLOCK_PIN, LATCH_PIN, ENABLE_PIN,
               ADDR_A, ADDR_B, ADDR_C, ADDR_D,
               128, 32, 2, 16);  // 1/16 scan

ledPanel.startScanning(100);      // 100 Hz base frequency
ledPanel.setBrightness(200);      // 0-255
```

### Verifikasi Wiring

Pastikan koneksi sesuai dengan pin configuration di kode:

- **Data Pins**: R1 (upper half), R2 (lower half) harus separate
- **Control Pins**: Clock, Latch, Enable harus di pin digital yang support PWM (untuk ENABLE)
- **Address Pins**: A, B, C, D untuk 1/16 scan (4 pins untuk 16 rows)

### Jika Masih Flicker

1. **Reduce frequency**: `startScanning(50)` - turunkan dari 100Hz
2. **Increase brightness**: Ensure `setBrightness()` dipanggil dengan nilai > 150
3. **Check power**: LED panel memerlukan power supply yang stabil
4. **Check wiring**: Verifikasi semua koneksi, terutama GND dari panel ke Arduino

## Changes Made (v1.1)

- Fixed output enable timing dalam `scan()` function
- Moved address pin setup sebelum latch pulse
- Implemented proper PWM brightness control per scan cycle
- Removed premature `analogWrite()` call dari `begin()` function
