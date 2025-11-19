# MQTT Text Rendering Integration

## Hardware Configuration (ATmega2560)

### Pin Mapping untuk HUB08 Panel

| Function | Pin | Port | Description |
|----------|-----|------|-------------|
| **Data R1** | 22 | D22 | Upper half data (rows 0-15) |
| **Data R2** | 23 | D23 | Lower half data (rows 16-31) |
| **Clock** | 24 | D24 | Shift register clock |
| **Latch** | 25 | D25 | Latch enable (LAT) |
| **Enable** | 26 | D26 | Output enable (OE) - active LOW |
| **Addr A** | 27 | D27 | Address line A (LSB) |
| **Addr B** | 28 | D28 | Address line B |
| **Addr C** | 29 | D29 | Address line C |
| **Addr D** | 30 | D30 | Address line D (MSB) |

### Panel Specifications
- **Resolution**: 64×32 pixels per panel
- **Chain**: 2 panels = 128×32 total display
- **Scan Type**: 1/16 multiplexing
- **Color**: Monochrome (white/black)

---

## MQTT Topics

### Command Topics (Device subscribes)

#### 1. Text Rendering
**Topic**: `device/iot_led_panel/command/text`

**Payload**:
```json
{
  "text": "Hello World",
  "x": 0,
  "y": 16,
  "color": 1
}
```

**Parameters**:
- `text` (string): Text to display (max 127 chars)
- `x` (int): X coordinate (0-127)
- `y` (int): Y coordinate (0-31)
- `color` (int): 0 = off, 1 = on

**Example**:
```bash
mosquitto_pub -h 192.168.1.1 -p 1884 -u edgeadmin -P edge123 \
  -t "device/iot_led_panel/command/text" \
  -m '{"text":"IoT LED","x":10,"y":16,"color":1}'
```

---

#### 2. Clear Display
**Topic**: `device/iot_led_panel/command/clear`

**Payload**: Any value (payload is ignored)

**Example**:
```bash
mosquitto_pub -h 192.168.1.1 -p 1884 -u edgeadmin -P edge123 \
  -t "device/iot_led_panel/command/clear" \
  -m "1"
```

---

#### 3. Brightness Control
**Topic**: `device/iot_led_panel/command/brightness`

**Payload**:
```json
{
  "brightness": 200
}
```

**Parameters**:
- `brightness` (int): 0-255 (0 = off, 255 = max)

**Example**:
```bash
mosquitto_pub -h 192.168.1.1 -p 1884 -u edgeadmin -P edge123 \
  -t "device/iot_led_panel/command/brightness" \
  -m '{"brightness":150}'
```

---

### Status Topics (Device publishes)

#### 1. Display Status
**Topic**: `device/iot_led_panel/status/display`

**Payload**:
```json
{
  "text": "Hello World",
  "x": 0,
  "y": 16,
  "brightness": 200
}
```

**Published when**: Text rendered, cleared, or brightness changed

---

#### 2. Device Info
**Topic**: `device/iot_led_panel/info`

**Payload**:
```json
{
  "device_id": "iot_led_panel",
  "ip": "192.168.1.60",
  "uptime_ms": 125000,
  "free_memory": 1500
}
```

**Published**: Every 30 seconds (heartbeat)

---

#### 3. Status (Online/Offline)
**Topic**: `device/iot_led_panel/status` (retained)

**Payload**: `"online"` or `"offline"`

**Published**: On connection (retained), on disconnect (broker sets to offline)

---

## Network Configuration

| Setting | Value |
|---------|-------|
| **MQTT Broker** | 192.168.1.1 |
| **MQTT Port** | 1884 |
| **Username** | edgeadmin |
| **Password** | edge123 |
| **Device IP** | 192.168.1.60 |
| **API Port** | 8080 |

---

## Testing

### 1. Subscribe to Status (in another terminal)
```bash
mosquitto_sub -h 192.168.1.1 -p 1884 -u edgeadmin -P edge123 \
  -t "device/iot_led_panel/status/display"
```

### 2. Send Text Command
```bash
mosquitto_pub -h 192.168.1.1 -p 1884 -u edgeadmin -P edge123 \
  -t "device/iot_led_panel/command/text" \
  -m '{"text":"Test123","x":5,"y":20,"color":1}'
```

### 3. Clear Display
```bash
mosquitto_pub -h 192.168.1.1 -p 1884 -u edgeadmin -P edge123 \
  -t "device/iot_led_panel/command/clear" \
  -m ""
```

### 4. Adjust Brightness
```bash
mosquitto_pub -h 192.168.1.1 -p 1884 -u edgeadmin -P edge123 \
  -t "device/iot_led_panel/command/brightness" \
  -m '{"brightness":100}'
```

---

## Serial Monitor Output

When connected, you'll see:
```
Start
Panel OK
Eth:192.168.1.60
API:8080
MQTT:Setup
Eth:OK
MQTT:OK
MQTT: Text rendered: Hello World
MQTT: Display cleared
MQTT: Brightness set to 100
```

---

## API Integration

### GET /api/device/info

Returns full device information including:
- Device ID
- IP Address
- Uptime
- Free memory
- Network configuration
- Service status

**Example**:
```bash
curl http://192.168.1.60:8080/api/device/info | jq .
```

---

## Advanced: Custom Font Selection

The device uses **FreeSans9pt7b** Adafruit font by default. To change:

Edit `src/main.cpp`:
```cpp
// Change this line in setup():
ledPanel.setAdafruitFont(&FreeSans9pt7b);

// To use different fonts from Adafruit GFX:
// Available fonts: FreeMono9pt7b, FreeSansBold9pt7b, etc.
```

---

## Troubleshooting

### Device not responding to MQTT commands
1. Check MQTT connection: Monitor serial output for "MQTT:OK"
2. Verify broker connectivity: `ping 192.168.1.1`
3. Check topic subscription: Use `mosquitto_sub` to verify topics

### Text not appearing on panel
1. Verify panel is powered and scanning
2. Check brightness > 0 (default: 200)
3. Ensure x, y coordinates are within display bounds (0-127 x, 0-31 y)

### Brightness not changing
1. Verify `brightness` command format is valid JSON
2. Check value is 0-255 range
3. Monitor serial output for "Brightness set to X"

---

## Memory Usage

- **ATmega2560 RAM**: 27.4% (2245 bytes / 8192 bytes)
- **ATmega2560 Flash**: 12.7% (32246 bytes / 253952 bytes)

With 87.3% Flash headroom, you can add more features:
- Additional text rendering effects
- Animation support
- Custom API endpoints
- Extended MQTT commands

