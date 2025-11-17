# Hardware Wiring Guide

## Arduino Mega 2560 + Ethernet Shield W5100

### Pin Configuration

#### SPI Interface (Ethernet Shield → Arduino Mega)

```
W5100 Shield Pin    Arduino Mega Pin    Function
MOSI                11                  SPI Data Out
MISO                12                  SPI Data In
SCK                 13                  SPI Clock
CS (Chip Select)    10                  Ethernet Select
GND                 GND                 Ground
+5V                 5V                  Power
```

#### Serial Communication

```
Serial0 (USB)
TX → Pin 1 (auto via USB)
RX → Pin 0 (auto via USB)
Baud: 9600

Serial1
TX → Pin 18
RX → Pin 19
```

#### Power & Ground

```
Arduino Mega → Ethernet Shield → Power Supply
5V          → +5V (Shield)     → +5V
GND         → GND (Shield)     → GND
```

### Full Pin Assignment

```cpp
// Ethernet Shield W5100 (uses SPI)
#define W5100_CS_PIN  10    // Chip Select
#define MOSI_PIN      11    // SPI Data Out
#define MISO_PIN      12    // SPI Data In
#define SCK_PIN       13    // SPI Clock

// These are used by Ethernet.h internally
// No need to configure manually
```

### Connector Diagram

```
                    ┌─────────────────┐
                    │  Ethernet Shield│
                    │     W5100       │
                    │                 │
                    │   RJ-45 Jack    │
                    │  (to Router)    │
                    └─────────────────┘
                            ↓
                    (Ethernet Cable)
                            ↓
                      Router/Network

                    ┌─────────────────┐
                    │  Arduino Mega   │
                    │     2560        │
         ┌──────────┼──────┬──────────┼──────────┐
         │          │      │          │          │
      USB to        GND   5V    Shield MOSI(11) MISO(12)
      Laptop              Connector     SCK(13)  CS(10)
         │          │      │          │          │
         └──────────┴──────┴──────────┴──────────┘
                       ↑
                 (Via Programming Port)
```

### Connection Steps

1. **Assembly**

   ```
   1. Stack Ethernet Shield on top of Arduino Mega
   2. Align all pin headers correctly
   3. Shield should sit flush with Arduino
   ```

2. **Cabling**

   ```
   1. Connect Ethernet cable to W5100 RJ-45 jack
   2. Other end → Router/Network Switch
   3. Connect USB cable to Arduino Mega
   4. USB → Laptop/Computer
   ```

3. **Verification**
   ```
   1. Arduino LED should light up (power indicator)
   2. Shield LED should light up if connected to network
   3. Check for any loose connections
   ```

## Default Network Configuration

### Static IP Setup

```
Device IP:    192.168.1.100
Subnet Mask:  255.255.255.0
Gateway:      192.168.1.1
DNS Primary:  8.8.8.8
DNS Secondary:1.1.1.1
MAC Address:  DE:AD:BE:EF:FE:ED (hardcoded)
```

### MQTT Configuration

```
Server: 192.168.1.1
Port:   1883
User:   edgeadmin
Pass:   edge123
```

## Hardware Troubleshooting

### Ethernet Shield not recognized

**Symptoms:**

- Serial output shows: "❌ Ethernet initialization failed"
- No LED on shield

**Check:**

1. ✅ Shield properly stacked on Arduino
2. ✅ All pins aligned correctly
3. ✅ Power connections good (5V, GND)
4. ✅ SPI pins (11, 12, 13) not blocked
5. ✅ CS pin (10) not used elsewhere

**Fix:**

- Reseat the shield (remove and re-install)
- Check for bent pins
- Try different shield (if available)

### Network cable connection issue

**Symptoms:**

- Serial output shows: "❌ Ethernet link is down"
- Shield LED not blinking

**Check:**

1. ✅ Ethernet cable fully inserted (both ends)
2. ✅ Router/Switch powered on
3. ✅ Try different network port
4. ✅ Try different cable

**Fix:**

- Unplug/replug both ends
- Try different port on router
- Swap with known-good cable

### USB Connection issue

**Symptoms:**

- Device not showing in `pio device list`
- Cannot upload code

**Check:**

1. ✅ USB cable connected
2. ✅ USB cable not damaged
3. ✅ Computer USB port working
4. ✅ Device shows in system (lsusb)

**Fix:**

```bash
# Check if device appears
lsusb | grep Arduino

# List available ports
pio device list

# If port not recognized, try:
sudo apt install libftdi-dev  # Linux
```

## Performance Notes

### Power Consumption

```
Idle State:       ~100 mA
With Ethernet:    ~150 mA
Full Load:        ~200 mA

Recommended Power Supply: 1A @ 5V (at minimum)
Better: 2A @ 5V (with headroom)
```

### Thermal Management

```
Operating Temp:   0°C - 50°C
Storage Temp:     -20°C - 60°C

MCU typically runs at 30-35°C in normal operation
Shield typically runs at 25-30°C

If running hot (>45°C):
- Check for short circuit
- Verify cooling airflow
- Reduce load/data rate
```

### Signal Integrity

```
SPI Clock:  13 MHz maximum
Ethernet:   10/100 Mbps automatic
USB:        Full-speed (12 Mbps)

Keep SPI cables short (<10cm)
Avoid long wire runs near power lines
```

## Upgrade Considerations

If you need to upgrade components:

### Alternative Ethernet Shields

- ✅ Ethernet Shield v1 (cheaper, slower)
- ✅ Ethernet Shield v2 (better, same pins)
- ✅ WiFi Shield (different pins, different setup)
- ❌ W5500 Shield (different SPI, needs rewrite)

### MCU Upgrades

- ✅ Arduino Due (more RAM/Flash, 3.3V logic)
- ✅ Arduino Mega ADK (variant with ADK)
- ❌ Arduino UNO (not enough memory - we tested this!)

### Network Upgrades

- ✅ WiFi Shield instead of Ethernet
- ✅ LoRaWAN Shield for long-range
- ✅ Cellular Shield for GSM/LTE

---

**Last Updated**: November 17, 2025
**Compatible With**: Arduino Mega 2560 + Ethernet Shield W5100
