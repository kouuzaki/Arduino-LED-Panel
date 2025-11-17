# Quick Start Guide

## ⚡ Super Cepat

### 1. Siapkan Hardware

```
Arduino Mega 2560 ← USB ← Laptop
Ethernet Shield   ← terpasang di SPI pins
Ethernet Cable    ← ke router
```

### 2. Compile & Upload

```bash
cd /home/kouuzaki/Documents/DumaiWorkspace/Arduino-LED-Panel
pio run -e mega2560 -t upload
```

### 3. Monitor Serial

```bash
pio device monitor -p /dev/ttyACM0 -b 9600
```

## ✅ Expected Output (Pertama Kali)

```
========================================
  Arduino UNO IoT Base - Starter Kit
========================================

🔧 Initializing Device Configuration...
📝 No configuration found - applying factory defaults...
✅ Factory defaults applied and saved to EEPROM

Device ID    : arduino_mega_eth
Device IP    : 192.168.1.100
MQTT Server  : 192.168.1.1
MQTT Port    : 1883

🌐 Starting network...
🔌 Initializing Ethernet (W5100 Shield)...
✅ Ethernet link is up
📍 IP Address: 192.168.1.100
✅ Ethernet connected: 192.168.1.100

📡 Initializing MQTT...
✅ MQTT configured: 192.168.1.1:1883
🔌 Connecting to MQTT broker...
✅ MQTT connected
📋 Subscribing to MQTT topics...
✅ MQTT subscriptions successful

========================================
  Setup Complete - System Ready
========================================
```

## 📋 Default Configuration

| Setting       | Value              |
| ------------- | ------------------ |
| Device ID     | `arduino_mega_eth` |
| IP Address    | `192.168.1.100`    |
| Gateway       | `192.168.1.1`      |
| DNS           | `8.8.8.8`          |
| MQTT Server   | `192.168.1.1`      |
| MQTT Port     | `1883`             |
| MQTT User     | `edgeadmin`        |
| MQTT Password | `edge123`          |

## 🔧 Jika Ethernet Tidak Terhubung

Output akan seperti ini:

```
⏳ Waiting for Ethernet link............
❌ Ethernet link is down - checking if cable connected...
⚠️ Proceeding anyway with configured static IP
```

**Solusi:**

1. ✅ Check kabel ethernet connected ke router
2. ✅ Pastikan Ethernet Shield terpasang dengan benar
3. ✅ Coba unplug-replug
4. ✅ Check LED pada shield

## 🌐 Test API Endpoint

Setelah device online dengan IP `192.168.1.100`:

```bash
# Get Status
curl http://192.168.1.100/api/status

# Get Device Info
curl http://192.168.1.100/api/device-info

# Get Config
curl http://192.168.1.100/api/config
```

## 📡 Test MQTT Connection

Dari laptop dengan MQTT client (mosquitto):

```bash
# Subscribe to all device topics
mosquitto_sub -h 192.168.1.1 -u edgeadmin -P edge123 -t "system/+/status" -t "network/+/status"

# Send system status command
mosquitto_pub -h 192.168.1.1 -u edgeadmin -P edge123 -t "system/arduino_mega_eth/cmd" -m '{"action":"status"}'

# Check network info
mosquitto_pub -h 192.168.1.1 -u edgeadmin -P edge123 -t "network/arduino_mega_eth/cmd" -m '{"action":"status"}'
```

## 📊 Memory Status

```
RAM:   74.9% (6139 / 8192 bytes)
Flash: 22.0% (55938 / 253952 bytes)
```

**Status**: ✅ Comfortable - banyak ruang untuk tambah fitur

## 🚀 Next Steps

1. ✅ Boot device dan verify semua working
2. 📝 Customize MQTT topics sesuai kebutuhan
3. 📝 Add LED control logic
4. 📝 Add sensor integration
5. 📝 Deploy ke production

---

**Questions?** Check `SETUP_MEGA_2560.md` untuk detail lengkap
