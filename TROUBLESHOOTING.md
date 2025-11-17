# Troubleshooting Guide

## ❌ Masalah: Compilation Error

### Error: "expected unqualified-id before '{' token"

**Penyebab**: Syntax error di file (orphan brace)
**Solusi**:

```bash
pio run -e mega2560 --verbose  # Lihat error detail
# Fix file dan compile ulang
```

### Error: "undefined reference to `printMacAddress'"

**Penyebab**: Function declaration missing
**Solusi**: Pastikan di NetworkManager.h ada:

```cpp
// Helper function declaration
void printMacAddress(byte *mac);
```

---

## ❌ Masalah: Upload Gagal

### "device not found" atau "port busy"

**Solusi**:

1. Check port mana Arduino connected:

```bash
ls /dev/tty* | grep -i usb
# atau
pio device list
```

2. Specify port saat upload:

```bash
pio run -e mega2560 -t upload --upload-port /dev/ttyACM0
```

3. Jika tetap error:

```bash
# Kill any process using the port
sudo lsof /dev/ttyACM0
# Then unplug-replug USB cable
```

### "avrdude: stk500_recv(): programmer is not responding"

**Penyebab**: Boot loader issue atau hardware problem
**Solusi**:

1. Pastikan Mega 2560 asli (bukan clone)
2. Coba upload dengan PlatformIO verbose mode
3. Coba double-click reset button sebelum upload

---

## ❌ Masalah: Ethernet Tidak Terhubung

### Serial Output:

```
❌ Ethernet link is down - checking if cable connected...
```

**Checklist:**

1. ✅ Kabel ethernet terpasang ke router
2. ✅ Ethernet Shield terpasang ke pin SPI (11, 12, 13) + CS pin 10
3. ✅ LED pada shield menyala (biasanya ada LED indicator)
4. ✅ IP configuration benar (default: 192.168.1.100)
5. ✅ Router sudah on

**Jika tetap tidak terhubung:**

- Coba di komputer lain/router lain
- Check router DHCP setting (jika pakai DHCP)
- Coba static IP manually via serial command (future)

---

## ❌ Masalah: MQTT Tidak Connect

### Serial Output:

```
⚠️  MQTT not configured
```

**Penyebab**: EEPROM kosong atau config tidak tersimpan
**Solusi**:

1. First boot akan auto-initialize → cek serial output
2. Jika tetap kosong, check EEPROM:

```cpp
// Uncomment di main.cpp untuk force reset config
deviceConfig.setCustomDefaults(...);
deviceConfig.saveConfig();
```

### Serial Output:

```
🔌 Connecting to MQTT broker...
// (tunggu lama, tidak ada pesan sukses)
```

**Penyebab**: MQTT broker tidak accessible
**Solusi**:

1. Verifikasi server address benar: `192.168.1.1`
2. Verifikasi port benar: `1883`
3. Check username/password: `edgeadmin` / `edge123`
4. Pastikan MQTT broker running:

```bash
# Dari laptop
mosquitto_pub -h 192.168.1.1 -u edgeadmin -P edge123 -t "test" -m "hello"
```

5. Jika tetap tidak connect, check firewall

---

## ❌ Masalah: Memory Penuh

### Error: "section '.text' overflows available space"

**Status**: Tidak seharusnya terjadi pada Mega 2560
**Jika terjadi**:

1. Compile dengan `-Os` optimization (sudah default)
2. Hapus unused features (API, HTTP server)
3. Upgrade hardware

---

## ❌ Masalah: Serial Output Tidak Jelas / Garbled

### Output terlihat:

```
▒▒▒▒▒▒▒ ╫╣╝╔╚╕
```

**Penyebab**: Baud rate salah
**Solusi**:

```bash
# Default adalah 9600, pastikan monitor set ke 9600
pio device monitor -b 9600

# Jika masih garbled, coba baud rate lain:
pio device monitor -b 115200
```

---

## ❌ Masalah: Device Restart Terus-Menerus

### Serial Output:

```
(boot messages muncul berulang)
```

**Penyebab**: Watchdog timeout atau memory corruption
**Solusi**:

1. Check apakah ada infinite loop di code
2. Check apakah memory overrun
3. Coba upload ulang clean build:

```bash
pio run -e mega2560 --target clean
pio run -e mega2560 -t upload
```

---

## ❌ Masalah: API Endpoint Tidak Respond

### Command:

```bash
curl http://192.168.1.100/api/status
# Timeout atau connection refused
```

**Penyebab**:

1. Device belum online (check serial)
2. IP address berbeda
3. API server tidak started

**Solusi**:

1. Pastikan Ethernet connected (serial output: "✅ Ethernet linked is up")
2. Ping device dulu:

```bash
ping 192.168.1.100
```

3. Cek konfigurasi IP di serial output

---

## 🔍 Debug Tips

### Enable Verbose Output

```bash
pio run -e mega2560 --verbose
```

### Check Memory Usage

```bash
pio run -e mega2560
# Lihat output: RAM: XX%, Flash: XX%
```

### View Full Serial Output (tanpa tail)

```bash
pio device monitor -b 9600 --raw
```

### Save Serial Output ke File

```bash
pio device monitor -b 9600 > /tmp/serial_log.txt &
```

### List Connected Devices

```bash
pio device list
```

---

## 📞 Kontrol MQTT untuk Debug

### Subscribe ke semua device status:

```bash
mosquitto_sub -h 192.168.1.1 -u edgeadmin -P edge123 -t "#" -v
```

### Send system command:

```bash
mosquitto_pub -h 192.168.1.1 -u edgeadmin -P edge123 \
  -t "system/arduino_mega_eth/cmd" \
  -m '{"action":"status"}'
```

### Send network command:

```bash
mosquitto_pub -h 192.168.1.1 -u edgeadmin -P edge123 \
  -t "network/arduino_mega_eth/cmd" \
  -m '{"action":"status"}'
```

---

## ✅ Verification Checklist

Untuk memastikan semua working:

- [ ] Compile tanpa error
- [ ] Upload successful ke device
- [ ] Serial output menunjukkan initialization messages
- [ ] "✅ Ethernet connected" message muncul
- [ ] "✅ MQTT connected" message muncul
- [ ] Ping device berhasil
- [ ] `curl /api/status` memberikan response
- [ ] MQTT topics tercapai dari laptop

---

**Last Updated**: November 17, 2025
**Platform**: Arduino Mega 2560 + W5100 Shield
