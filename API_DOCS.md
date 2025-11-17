# API Documentation

## HTTP Server

- **Port:** 8080
- **Protocol:** HTTP/1.1
- **Base URL:** `http://192.168.1.60:8080`

## Endpoints

### GET /api/device/info

Retrieve device system information including uptime, free memory, etc.

**Request:**

```
GET /api/device/info HTTP/1.1
Host: 192.168.1.60:8080
```

**Response (200 OK):**

```json
{
  "device": "iot_led_panel",
  "uptime": 45230,
  "memory": {
    "free": 853,
    "used": 1195
  },
  "network": {
    "ip": "192.168.1.60",
    "mac": "DE:AD:BE:EF:FE:ED"
  }
}
```

**cURL Example:**

```bash
curl http://192.168.1.60:8080/api/device/info
```

---

## MQTT Topics

### Publish (Device → Broker)

- `device/iot_led_panel/status` - Device status (online/offline)
- `device/iot_led_panel/info` - Device info (JSON, every 30s)

### Subscribe (Broker → Device)

- `device/iot_led_panel/cmd` - Command topic (for future use)

**MQTT Config:**

- **Broker:** 192.168.1.1:1884
- **Username:** edgeadmin
- **Password:** edge123
- **Heartbeat Interval:** 30 seconds

---

## Implementation Notes

### Modular Architecture

- `handlers/api_handler.h` - HTTP API server on port 8080
- `handlers/mqtt_handler.h` - MQTT client with auto-reconnect
- `src/main.cpp` - Main controller

### Memory Efficient

- **RAM Used:** 1195 bytes (58.3% of 2048)
- **Flash Used:** 27214 bytes (84.4% of 32256)
- Minimal serial debug output
- No string literals in SRAM (using PROGMEM where needed)

### Features

- ✅ HTTP API on port 8080
- ✅ Auto-reconnect MQTT
- ✅ Device info endpoint
- ✅ 30s heartbeat
- ✅ LED panel control (HUB08 P4.75)
