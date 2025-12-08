# HUB08 LED Panel - API Documentation

Complete guide for controlling the LED matrix via **REST API** and **MQTT**.

## Quick Start

### Device Details

- **IP Address**: `192.168.1.60` (configurable via API)
- **REST API Port**: `8080`
- **MQTT Broker**: `192.168.1.1:1884` (configurable via API)
- **Device ID**: `led_lilygo` (default, configurable)

---

## REST API (HTTP)

### Base URL

```
http://192.168.1.60:8080
```

---

### 1. GET /api/device/info

**Retrieve device information and current system status**

#### Request

```bash
curl -X GET http://192.168.1.60:8080/api/device/info
```

#### Response (200 OK)

```json
{
  "ok": true,
  "message": "Device information retrieved successfully",
  "timestamp": 123456789,
  "data": {
    "device_id": "led_lilygo",
    "mac_address": "02:00:00:01:02:03",
    "uptime": "0000-00-00 00:15:30",
    "free_memory": "6 KB",
    "memory": {
      "total_bytes": 8192,
      "free_bytes": 5632,
      "used_bytes": 2560,
      "used_percent": 31.2,
      "used": "2.5 KB"
    },
    "network": {
      "type": "ethernet",
      "status": "connected",
      "connected": true,
      "ethernet_available": true,
      "ip": "192.168.1.60",
      "mac": "02:00:00:01:02:03",
      "subnet_mask": "255.255.255.0",
      "gateway": "192.168.1.1",
      "dns_primary": "8.8.8.8",
      "dns_secondary": "1.1.1.1"
    },
    "mqtt_server_ip": "192.168.1.1",
    "services": {
      "api": "running",
      "mqtt": "connected"
    }
  }
}
```

---

### 2. POST /api/display/text

**Display text on the LED matrix**

#### Request

```bash
curl -X POST http://192.168.1.60:8080/api/display/text \
  -H "Content-Type: application/json" \
  -d '{
    "text": "HELLO\nWORLD",
    "brightness": 200
  }'
```

#### Request Body

```json
{
  "text": "HELLO\nWORLD",
  "brightness": 200
}
```

| Field        | Type    | Required | Description                                |
| ------------ | ------- | -------- | ------------------------------------------ |
| `text`       | string  | ✅ Yes   | Text to display; use `\n` for newlines     |
| `brightness` | integer | ❌ No    | LED brightness (0-255); default: 255 (max) |

#### Response (200 OK)

```json
{
  "ok": true,
  "message": "Text displayed",
  "action": "text"
}
```

#### Error Responses

```json
// 400 - Invalid JSON
{"error": "invalid json"}

// 422 - Missing required field
{"error": "missing required field: text"}

// 503 - Display service unavailable
{"error": "display service not available"}
```

#### Example Usage

```bash
# Display simple text
curl -X POST http://192.168.1.60:8080/api/display/text \
  -H "Content-Type: application/json" \
  -d '{"text": "IoT Device"}'

# Display multiline text with custom brightness
curl -X POST http://192.168.1.60:8080/api/display/text \
  -H "Content-Type: application/json" \
  -d '{"text": "TEMP\n25C\nHUM\n60%", "brightness": 150}'
```

---

### 3. POST /api/display/clear

**Clear the LED display (black screen)**

#### Request

```bash
curl -X POST http://192.168.1.60:8080/api/display/clear
```

#### Response (200 OK)

```json
{
  "ok": true,
  "message": "Display cleared",
  "action": "clear"
}
```

#### Error Response

```json
// 503 - Display service unavailable
{ "error": "display service not available" }
```

---

### 4. POST /api/display/brightness

**Set the LED display brightness level**

#### Request

```bash
curl -X POST http://192.168.1.60:8080/api/display/brightness \
  -H "Content-Type: application/json" \
  -d '{"brightness": 150}'
```

#### Request Body

```json
{
  "brightness": 150
}
```

| Field        | Type    | Required | Description                              |
| ------------ | ------- | -------- | ---------------------------------------- |
| `brightness` | integer | ✅ Yes   | Brightness level (0-255); 0=off, 255=max |

#### Response (200 OK)

```json
{
  "ok": true,
  "message": "Brightness set to 150",
  "action": "brightness",
  "value": 150
}
```

#### Error Responses

```json
// 400 - Invalid JSON
{"error": "invalid json"}

// 422 - Invalid brightness value or missing field
{"error": "brightness must be 0-255"}
{"error": "missing required field: brightness"}

// 503 - Display service unavailable
{"error": "display service not available"}
```

#### Example Usage

```bash
# Turn off display
curl -X POST http://192.168.1.60:8080/api/display/brightness \
  -H "Content-Type: application/json" \
  -d '{"brightness": 0}'

# Maximum brightness
curl -X POST http://192.168.1.60:8080/api/display/brightness \
  -H "Content-Type: application/json" \
  -d '{"brightness": 255}'

# Half brightness
curl -X POST http://192.168.1.60:8080/api/display/brightness \
  -H "Content-Type: application/json" \
  -d '{"brightness": 128}'
```

---

### 5. POST /api/device/config

**Update device configuration and restart**

#### Request

```bash
curl -X POST http://192.168.1.60:8080/api/device/config \
  -H "Content-Type: application/json" \
  -d '{
    "device_id": "led_lilygo",
    "mqtt_server": "192.168.1.1",
    "mqtt_port": 1884,
    "mqtt_username": "edgeadmin",
    "mqtt_password": "edge123",
    "mqtt_heartbeat_interval": 10000,
    "ip": "192.168.1.60",
    "gateway": "192.168.1.1",
    "subnet_mask": "255.255.255.0",
    "dns_primary": "8.8.8.8"
  }'
```

#### Request Body (All fields)

```json
{
  "device_id": "led_lilygo",
  "mqtt_server": "192.168.1.1",
  "mqtt_port": 1884,
  "mqtt_username": "edgeadmin",
  "mqtt_password": "edge123",
  "mqtt_heartbeat_interval": 10000,
  "ip": "192.168.1.60",
  "gateway": "192.168.1.1",
  "subnet_mask": "255.255.255.0",
  "dns_primary": "8.8.8.8",
  "dns_secondary": "1.1.1.1"
}
```

| Field                     | Type    | Required | Description                                                   |
| ------------------------- | ------- | -------- | ------------------------------------------------------------- |
| `device_id`               | string  | ✅ Yes   | Unique device identifier                                      |
| `mqtt_server`             | string  | ✅ Yes   | MQTT broker IP address                                        |
| `mqtt_port`               | integer | ❌ No    | MQTT broker port (default: 1884)                              |
| `mqtt_username`           | string  | ❌ No    | MQTT authentication username                                  |
| `mqtt_password`           | string  | ❌ No    | MQTT authentication password                                  |
| `mqtt_heartbeat_interval` | integer | ❌ No    | MQTT heartbeat interval in ms (1-86,400,000; default: 10,000) |
| `ip`                      | string  | ❌ No    | Device static IP address                                      |
| `gateway`                 | string  | ❌ No    | Network gateway IP                                            |
| `subnet_mask`             | string  | ❌ No    | Network subnet mask                                           |
| `dns_primary`             | string  | ❌ No    | Primary DNS server                                            |
| `dns_secondary`           | string  | ❌ No    | Secondary DNS server                                          |

#### Response (200 OK)

```json
{
  "ok": true,
  "message": "Config saved. Device restarting..."
}
```

**Note**: Device will restart in ~2 seconds after this response.

#### Error Responses

```json
// 400 - Invalid JSON
{"error": "invalid json"}

// 422 - Missing required fields or invalid values
{"error": "missing required fields"}
{"error": "invalid mqtt_heartbeat_interval (must be 1..86400000)"}

// 500 - Storage error
{"error": "failed to save"}
```

#### Example Usage

```bash
# Update MQTT server only
curl -X POST http://192.168.1.60:8080/api/device/config \
  -H "Content-Type: application/json" \
  -d '{
    "device_id": "led_lilygo",
    "mqtt_server": "192.168.1.100"
  }'

# Change heartbeat interval to 30 seconds (30,000 ms)
curl -X POST http://192.168.1.60:8080/api/device/config \
  -H "Content-Type: application/json" \
  -d '{
    "device_id": "led_lilygo",
    "mqtt_server": "192.168.1.1",
    "mqtt_heartbeat_interval": 30000
  }'

# Heartbeat max 1 day (86,400,000 ms)
curl -X POST http://192.168.1.60:8080/api/device/config \
  -H "Content-Type: application/json" \
  -d '{
    "device_id": "led_lilygo",
    "mqtt_server": "192.168.1.1",
    "mqtt_heartbeat_interval": 86400000
  }'
```

---

## MQTT API

### Broker Connection

- **Host**: `192.168.1.1` (configurable via API)
- **Port**: `1884` (configurable via API)
- **Username**: `edgeadmin` (configurable via API)
- **Password**: `edge123` (configurable via API)
- **QoS**: 1 (at-least-once delivery)

### Topic Naming Convention

- **Command Topic**: `device/{device_id}/cmd/display`
- **Response Topic**: `{device_id}/response`
- **Telemetry Topic**: `system/{device_id}/info`

**Default Device ID**: `led_lilygo`

---

### 1. Display Text

**Display text on the LED matrix via MQTT**

#### Publish To

```
device/led_lilygo/cmd/display
```

#### Payload

```json
{
  "action": "text",
  "text": "HELLO\nWORLD",
  "brightness": 200
}
```

| Field        | Type    | Required | Description                            |
| ------------ | ------- | -------- | -------------------------------------- |
| `action`     | string  | ✅ Yes   | Must be `"text"`                       |
| `text`       | string  | ✅ Yes   | Text to display; use `\n` for newlines |
| `brightness` | integer | ❌ No    | LED brightness (0-255); default: 255   |

#### Response (Published To `{device_id}/response`)

```json
{
  "action": "text",
  "status": "success",
  "message": "Text displayed successfully",
  "timestamp": 123456789
}
```

#### Example (MQTT CLI)

```bash
mosquitto_pub -h 192.168.1.1 -p 1884 \
  -u edgeadmin -P edge123 \
  -t device/led_lilygo/cmd/display \
  -m '{"action":"text","text":"HELLO\nWORLD","brightness":200}'
```

---

### 2. Clear Display

**Clear the LED display via MQTT**

#### Publish To

```
device/led_lilygo/cmd/display
```

#### Payload

```json
{
  "action": "clear"
}
```

#### Response (Published To `{device_id}/response`)

```json
{
  "action": "clear",
  "status": "success",
  "message": "Display cleared successfully",
  "timestamp": 123456789
}
```

#### Example

```bash
mosquitto_pub -h 192.168.1.1 -p 1884 \
  -u edgeadmin -P edge123 \
  -t device/led_lilygo/cmd/display \
  -m '{"action":"clear"}'
```

---

### 3. Restart Device

**Trigger a soft reset of the device via MQTT**

#### Publish To

```
device/led_lilygo/cmd/display
```

#### Payload

```json
{
  "action": "restart"
}
```

#### Response (Published To `{device_id}/response`)

```json
{
  "action": "restart",
  "status": "success",
  "message": "Device will restart in 2 seconds",
  "timestamp": 123456789
}
```

**Note**: Device will restart in ~2 seconds after publishing the response.

#### Example

```bash
mosquitto_pub -h 192.168.1.1 -p 1884 \
  -u edgeadmin -P edge123 \
  -t device/led_lilygo/cmd/display \
  -m '{"action":"restart"}'
```

---

### 4. Device Heartbeat (Telemetry)

**Subscribe to receive device status every heartbeat interval (default: 10 seconds)**

#### Subscribe To

```
system/led_lilygo/info
```

#### Payload (Published Every 10 Seconds)

```json
{
  "message": "Device information",
  "timestamp": 123456789,
  "encrypted": false,
  "data": {
    "device_id": "led_lilygo",
    "mac_address": "02:00:00:01:02:03",
    "uptime": "0000-00-00 00:15:30",
    "free_memory": "6 KB",
    "memory": {
      "total_bytes": 8192,
      "free_bytes": 5632,
      "used_bytes": 2560,
      "used_percent": 31.2,
      "used": "2.5 KB"
    },
    "network": {
      "type": "ethernet",
      "status": "connected",
      "connected": true,
      "ethernet_available": true,
      "ip": "192.168.1.60",
      "mac": "02:00:00:01:02:03",
      "subnet_mask": "255.255.255.0",
      "gateway": "192.168.1.1",
      "dns_primary": "8.8.8.8",
      "dns_secondary": "1.1.1.1"
    },
    "mqtt_server_ip": "192.168.1.1",
    "services": {
      "api": "running",
      "mqtt": "connected"
    }
  }
}
```

#### Example (MQTT CLI - Subscribe)

```bash
mosquitto_sub -h 192.168.1.1 -p 1884 \
  -u edgeadmin -P edge123 \
  -t 'system/led_lilygo/info'
```

#### Configuring Heartbeat Interval

The heartbeat interval can be configured via the REST API POST `/api/device/config`:

```bash
# Set heartbeat to 30 seconds (30,000 ms)
curl -X POST http://192.168.1.60:8080/api/device/config \
  -H "Content-Type: application/json" \
  -d '{
    "device_id": "led_lilygo",
    "mqtt_server": "192.168.1.1",
    "mqtt_heartbeat_interval": 30000
  }'
```

**Heartbeat Interval Limits**:

- **Minimum**: 1 ms
- **Maximum**: 86,400,000 ms (24 hours)
- **Default**: 10,000 ms (10 seconds)

---

## Error Handling

### HTTP Status Codes

| Code | Meaning               | Example                                         |
| ---- | --------------------- | ----------------------------------------------- |
| 200  | OK                    | Command executed successfully                   |
| 400  | Bad Request           | Invalid JSON or malformed request               |
| 422  | Unprocessable Entity  | Invalid field values or missing required fields |
| 503  | Service Unavailable   | Display or MQTT service not initialized         |
| 404  | Not Found             | Unknown endpoint                                |
| 500  | Internal Server Error | Storage failure or memory error                 |

### Error Response Format

```json
{
  "error": "Description of the error"
}
```

### Common Errors

#### Missing Required Field

```json
{ "error": "missing required field: text" }
```

#### Invalid JSON

```json
{ "error": "invalid json" }
```

#### Invalid Brightness Value

```json
{ "error": "brightness must be 0-255" }
```

#### Invalid Heartbeat Interval

```json
{ "error": "invalid mqtt_heartbeat_interval (must be 1..86400000)" }
```

---

## Integration Examples

### Python - Control Display via REST API

```python
import requests
import json

device_url = "http://192.168.1.60:8080"

# Display text
def display_text(text, brightness=255):
    response = requests.post(
        f"{device_url}/api/display/text",
        json={"text": text, "brightness": brightness}
    )
    return response.json()

# Clear display
def clear_display():
    response = requests.post(f"{device_url}/api/display/clear")
    return response.json()

# Set brightness
def set_brightness(brightness):
    response = requests.post(
        f"{device_url}/api/display/brightness",
        json={"brightness": brightness}
    )
    return response.json()

# Get device info
def get_device_info():
    response = requests.get(f"{device_url}/api/device/info")
    return response.json()

# Usage
print(display_text("HELLO\nWORLD", 200))
print(get_device_info())
print(clear_display())
```

### Python - Control Display via MQTT

```python
import paho.mqtt.client as mqtt
import json

BROKER = "192.168.1.1"
PORT = 1884
USERNAME = "edgeadmin"
PASSWORD = "edge123"
DEVICE_ID = "led_lilygo"

client = mqtt.Client()
client.username_pw_set(USERNAME, PASSWORD)
client.connect(BROKER, PORT, 60)

def display_text(text, brightness=255):
    payload = {
        "action": "text",
        "text": text,
        "brightness": brightness
    }
    client.publish(
        f"device/{DEVICE_ID}/cmd/display",
        json.dumps(payload),
        qos=1
    )

def clear_display():
    payload = {"action": "clear"}
    client.publish(
        f"device/{DEVICE_ID}/cmd/display",
        json.dumps(payload),
        qos=1
    )

# Usage
client.loop_start()
display_text("HELLO\nWORLD", 200)
clear_display()
client.loop_stop()
```

---

## Troubleshooting

### Device Not Responding

1. Check device is powered on: LED display shows "SYSTEM READY"
2. Verify network connectivity: `ping 192.168.1.60`
3. Check device configuration: `curl http://192.168.1.60:8080/api/device/info`
4. Review device serial logs for errors

### MQTT Not Connected

1. Verify MQTT broker is running at configured address
2. Check credentials in device configuration
3. Confirm firewall allows port 1884
4. Check device logs for connection errors

### Display Not Showing Text

1. Verify display service is available: Check device info response
2. Ensure brightness is not 0 (off): Use `/api/display/brightness` to set > 0
3. Check text contains valid UTF-8 characters
4. Monitor device serial output for rendering errors

---

## Response Topic Subscription

Both REST API and MQTT commands support response feedback:

- **REST API**: Synchronous - response returned in HTTP response
- **MQTT**: Asynchronous - subscribe to `{device_id}/response` for command responses

### Standard Response Format

```json
{
  "action": "text|clear|restart|brightness",
  "status": "success|error",
  "message": "Human-readable status message",
  "timestamp": 123456789,
  "data": {
    // Optional additional data
  }
}
```
