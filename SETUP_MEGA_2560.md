# Arduino Mega 2560 - Ethernet + MQTT Setup

## Status: ✅ WORKING

Kode sudah dioptimalkan untuk **Arduino Mega 2560** dengan memory:

- **RAM**: 74.9% (6139 bytes dari 8192 bytes)
- **Flash**: 22.0% (55938 bytes dari 253952 bytes)

## Hardware Requirements

1. **Arduino Mega 2560**
2. **Ethernet Shield W5100** (connected to SPI pins)
3. **Ethernet Cable** (connected to router)

## Fitur Yang Active

✅ **Ethernet Connection**

- Static IP configuration
- W5100 Shield support
- Default: 192.168.1.100

✅ **MQTT Broker Support**

- PubSubClient library
- Auto-reconnect
- Default server: 192.168.1.1:1883

✅ **Simple API** (HTTP)

- Status endpoint
- Config endpoint
- Network info endpoint

✅ **Configuration Storage**

- EEPROM-based persistence
- Auto-load on boot
- Default values applied on first boot

## First Boot Configuration

Saat pertama kali di-upload, device akan:

1. Cek EEPROM apakah ada konfigurasi
2. Jika kosong, set defaults dan save ke EEPROM:
   - Device ID: `arduino_mega_eth`
   - IP: `192.168.1.100`
   - MQTT Server: `192.168.1.1:1883`
   - MQTT User: `edgeadmin` / `edge123`

Serial output akan menunjukkan:

```
🔧 Initializing Device Configuration...
📝 No configuration found - applying factory defaults...
✅ Factory defaults applied and saved to EEPROM
✅ Device Configuration loaded from EEPROM
```

## Network Connection Status

Setelah boot, akan melihat:

### Jika Ethernet Connected ✅

```
🌐 Starting network (Ethernet W5100)...
🔌 Initializing Ethernet (W5100 Shield)...
🔧 Configuring Ethernet with static IP...
⏳ Waiting for Ethernet link...
✅ Ethernet link is up
📍 IP Address: 192.168.1.100
✅ Ethernet connected: 192.168.1.100
```

### Jika Ethernet Disconnect ❌

```
⏳ Waiting for Ethernet link............
❌ Ethernet link is down - checking if cable connected...
⚠️ Proceeding anyway with configured static IP
```

## MQTT Connection

Setelah Ethernet terhubung:

```
📡 Initializing MQTT...
✅ MQTT configured: 192.168.1.1:1883
🔌 Connecting to MQTT broker...
✅ MQTT connected
📋 Subscribing to MQTT topics...
✅ MQTT subscriptions successful
```

## MQTT Topics

Sistem akan subscribe ke:

- `system/{device_id}/cmd` - System commands
- `network/{device_id}/cmd` - Network commands

Publish to:

- `system/{device_id}/status` - System status
- `network/{device_id}/status` - Network status

## Compile & Upload

### Compile saja (untuk test):

```bash
pio run -e mega2560
```

### Compile + Upload:

```bash
pio run -e mega2560 -t upload
```

### Monitor serial output:

```bash
pio device monitor -p /dev/ttyACM0 -b 9600
```

## Configuration via API

### Get Device Status

```bash
curl http://192.168.1.100/api/status
```

### Get Device Info

```bash
curl http://192.168.1.100/api/device-info
```

### Get Configuration

```bash
curl http://192.168.1.100/api/config
```

## Troubleshooting

### EEPROM Kosong / Tidak Tersimpan

- First boot akan auto-initialize dengan defaults
- Jika tidak berubah, check EEPROM library support

### Ethernet Tidak Terhubung

- Pastikan Ethernet shield terpasang dengan benar di pin SPI (11, 12, 13)
- Pin CS: 10 (default)
- Check sambungan kabel ethernet ke router

### MQTT Tidak Connect

- Verifikasi server address dan port di konfigurasi
- Pastikan broker MQTT running di server
- Check username/password

## Next Steps

1. ✅ Compile & upload ke Mega 2560
2. ✅ Verifikasi Ethernet terhubung
3. ✅ Verifikasi MQTT connect
4. 📝 Setup LED control logic (sesuai kebutuhan)
5. 📝 Add sensor integration

---

**Last Updated**: November 17, 2025
**Platform**: Arduino Mega 2560
**Memory**: 74.9% RAM, 22.0% Flash
