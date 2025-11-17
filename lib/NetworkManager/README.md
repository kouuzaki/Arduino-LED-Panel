# NetworkManager Library - Complete Documentation

NetworkManager adalah library untuk mengelola koneksi Ethernet dan WiFi pada ESP32 LilyGO T-Internet-POE board dengan sistem prioritas dan failover otomatis.

## 🚀 Fitur Utama

- **Dual Network Support**: Ethernet (prioritas) + WiFi (fallback)
- **Auto-Failover**: Otomatis beralih ke WiFi jika Ethernet gagal  
- **Static IP & DHCP**: Mendukung konfigurasi static IP dan DHCP
- **Network Diagnostics**: Tools untuk debugging network issues
- **Event Handling**: Real-time network event monitoring
- **Singleton Pattern**: Thread-safe instance management

## 📋 Hardware Requirements

- **Board**: LilyGO T-Internet-POE ESP32
- **Ethernet PHY**: LAN8720 (built-in)
- **Power**: PoE atau external 5V
- **Pins**: Pre-configured untuk T-Internet-POE

## 🔧 Pin Configuration (LilyGO T-Internet-POE)

```cpp
// Ethernet Pins (Hardware defined - TIDAK DAPAT DIUBAH)
#define ETH_CLK_MODE     ETH_CLOCK_GPIO17_OUT
#define ETH_PHY_ADDR     0
#define ETH_PHY_TYPE     ETH_PHY_LAN8720
#define ETH_PHY_POWER    5     // Power pin
#define ETH_PHY_MDC      23    // MDC pin  
#define ETH_PHY_MDIO     18    // MDIO pin
#define ETH_RESET_PIN    5     // Reset pin
```

⚠️ **PENTING**: Pin Ethernet sudah fix di hardware, tidak dapat diubah!

## 📦 Setup & Installation

### 1. Include Library dalam main.cpp

```cpp
#include <NetworkManager.h>

bool apiServerStarted = false;
bool lastConnectionStatus = false;
```

### 2. Basic Setup dalam setup()

```cpp
void setup() {
    Serial.begin(115200);
    
    // Inisialisasi Network Manager (tries Ethernet first, then WiFi)
    Serial.println("🌐 Initializing Network Manager (Ethernet + WiFi)...");
    NetworkManager& networkManager = NetworkManager::getInstance();
    
    if (networkManager.init()) {
        Serial.println("✅ Network initialization successful");
        NetworkType netType = networkManager.getType();
        if (netType == NETWORK_ETHERNET) {
            Serial.println("📡 Connected via Ethernet (LAN Cable)");
        } else if (netType == NETWORK_WIFI) {
            Serial.println("📶 Connected via WiFi");
        }
    } 
    else {
        Serial.println("⚠️ Network initialization failed, trying WiFi Manager...");
        
        // If automatic network fails, try WiFi Manager for manual setup
        wifiManager = new WiFiManager();
        
        if (wifiManager->autoConnect()) {
            Serial.println("✅ WiFi Manager auto-connect successful");
        } else {
            Serial.println("⚠️ WiFi Manager auto-connect failed");
            Serial.println("💡 Use 'WIFI' command in serial console for manual WiFi setup");
        }
    }
}
```

### 3. Loop Monitoring

```cpp
void loop() {
    // Check network connection status (Ethernet has priority over WiFi)
    NetworkManager& networkManager = NetworkManager::getInstance();
    bool currentConnectionStatus = networkManager.isConnected();
    
    if(currentConnectionStatus) {
        // Network connection is available, handle API server
        if (!apiServerStarted) {
            apiServerStarted = true;
            Serial.println("🚀 Starting API server...");
            
            // Display connection information
            NetworkType netType = networkManager.getType();
            if (netType == NETWORK_ETHERNET) {
                Serial.println("📡 Connection type: Ethernet (LAN Cable)");
                Serial.println("🔌 Ethernet MAC: " + networkManager.getMacAddress());
                Serial.println("🌐 Ethernet IP: " + networkManager.getLocalIP());
            } else if (netType == NETWORK_WIFI) {
                Serial.println("📶 Connection type: WiFi");
                Serial.println("📶 WiFi MAC: " + networkManager.getMacAddress());
                Serial.println("🌐 WiFi IP: " + networkManager.getLocalIP());
            }
            
            // Start your API server, MQTT, etc. here
            ApiRouting::ApiRoutingRoutes();
        }
    } else {
        // No network connection available
        if (apiServerStarted && lastConnectionStatus) {
            Serial.println("⚠ Network connection lost, API server may not function properly");
            Serial.println("🔄 Checking Ethernet cable or WiFi connection...");
        }
    }
    
    // Update last connection status
    lastConnectionStatus = currentConnectionStatus;
    delay(100);
}
```

## 🛠️ Konfigurasi Static IP

### Method 1: Ubah di NetworkManager.cpp (DHCP ke Static)

Buka file: `lib/NetworkManager/src/NetworkManager.cpp`

Cari fungsi `initEthernet()` sekitar line 248, ubah bagian ini:

**BEFORE (DHCP):**
```cpp
// Wait for IP address (up to 10 more seconds)
attempts = 0;
while (ETH2.localIP() == IPAddress(0, 0, 0, 0) && attempts < 100) {
    delay(100);
    attempts++;
    if (attempts % 10 == 0) {
        Serial.printf("⏳ Waiting for IP address... %d/10s\n", attempts/10);
    }
}

if (ETH2.localIP() != IPAddress(0, 0, 0, 0)) {
    Serial.print("🌐 Ethernet IP: ");
    Serial.println(ETH2.localIP());
    return true;
} else {
    Serial.println("❌ Failed to get IP address via DHCP");
    return false;
}
```

**AFTER (Static IP):**
```cpp
// Configure static IP immediately instead of waiting for DHCP
Serial.println("🔧 Configuring static IP (skipping DHCP)...");

IPAddress staticIP(192, 168, 18, 99);   // ESP32 IP - UBAH SESUAI NETWORK ANDA
IPAddress gateway(192, 168, 18, 1);     // Gateway router - UBAH SESUAI NETWORK ANDA  
IPAddress subnet(255, 255, 255, 0);     // Subnet mask
IPAddress dns1(8, 8, 8, 8);             // Google DNS
IPAddress dns2(192, 168, 18, 1);        // Local gateway as secondary DNS

if (ETH2.config(staticIP, gateway, subnet, dns1, dns2)) {
    Serial.println("🔧 Static IP configuration applied");
    delay(2000); // Give time for configuration to apply
    
    if (ETH2.localIP() == staticIP) {
        Serial.print("✅ Static IP assigned: ");
        Serial.println(ETH2.localIP());
        Serial.print("📡 Gateway: ");
        Serial.println(ETH2.gatewayIP());
        Serial.print("📡 Subnet: ");
        Serial.println(ETH2.subnetMask());
        Serial.print("📡 DNS: ");
        Serial.println(ETH2.dnsIP());
        return true;
    } else {
        Serial.println("❌ Static IP configuration failed");
        return false;
    }
} else {
    Serial.println("❌ Failed to configure static IP");
    return false;
}
```

### Method 2: Ganti dari Static ke DHCP

Jika ingin kembali ke DHCP, replace code static IP di atas dengan:

```cpp
// Wait for DHCP IP address (up to 10 seconds)
int attempts = 0;
while (ETH2.localIP() == IPAddress(0, 0, 0, 0) && attempts < 100) {
    delay(100);
    attempts++;
    if (attempts % 10 == 0) {
        Serial.printf("⏳ Waiting for DHCP IP... %d/10s\n", attempts/10);
    }
}

if (ETH2.localIP() != IPAddress(0, 0, 0, 0)) {
    Serial.print("🌐 Ethernet DHCP IP: ");
    Serial.println(ETH2.localIP());
    return true;
} else {
    Serial.println("❌ Failed to get IP address via DHCP");
    Serial.println("🔧 Consider using static IP configuration");
    return false;
}
```

### Method 3: Konfigurasi yang Mudah Diubah

Tambahkan di bagian atas main.cpp:

```cpp
// ============================
// NETWORK CONFIGURATION
// ============================
struct NetworkConfig {
    bool useStaticIP = true;  // true = static IP, false = DHCP
    IPAddress staticIP = IPAddress(192, 168, 18, 99);   // IP ESP32
    IPAddress gateway = IPAddress(192, 168, 18, 1);     // Gateway router
    IPAddress subnet = IPAddress(255, 255, 255, 0);     // Subnet mask
    IPAddress dns1 = IPAddress(8, 8, 8, 8);             // Primary DNS
    IPAddress dns2 = IPAddress(192, 168, 18, 1);        // Secondary DNS
};

// Instance configuration untuk easy modification
NetworkConfig networkConfig;

// Function to apply network config
void applyNetworkConfig() {
    if (networkConfig.useStaticIP) {
        Serial.printf("🔧 Network Config: Static IP %s\n", networkConfig.staticIP.toString().c_str());
    } else {
        Serial.println("🔧 Network Config: DHCP");
    }
}
```

Lalu dalam setup(), sebelum `networkManager.init()`:

```cpp
void setup() {
    // Customize network config sebelum init
    networkConfig.useStaticIP = true;  // Enable static IP
    networkConfig.staticIP = IPAddress(192, 168, 1, 150);  // Custom IP
    applyNetworkConfig();
    
    // Initialize network
    NetworkManager& networkManager = NetworkManager::getInstance();
    networkManager.init();
}
```

## 📚 API Reference (Sesuai Header File)

### Core Methods

```cpp
class NetworkManager {
public:
    // Singleton instance
    static NetworkManager& getInstance();
    
    // Initialization
    bool init();                    // Initialize dengan auto-detection
    bool initEthernet();           // Initialize Ethernet saja
    bool initWiFi();               // Initialize WiFi saja
    void setupEthernetEvents();    // Setup Ethernet event handlers
    void setupWiFiEvents();        // Setup WiFi event handlers
    
    // Status & Info
    bool isConnected();            // Check apakah connected
    NetworkType getType();         // Get current network type
    NetworkStatus getStatus();     // Get detailed status
    String getLocalIP();           // Get IP address
    String getMacAddress();        // Get MAC address
    String getNetworkInfo();       // Get JSON info lengkap
    bool isEthernetAvailable();    // Check Ethernet hardware
    bool isWiFiAvailable();        // Check WiFi hardware
    
    // WiFi Methods
    bool connectToWiFi(const String& ssid, const String& password);
    void scanWiFiNetworks();       // Scan available networks
    bool autoConnectWiFi();        // Auto-connect to saved
    
    // Diagnostics (Yang benar-benar ada)
    void printNetworkDiagnostics(); // Print detailed diagnostics
    bool testNetworkConnectivity(); // Test connection
};
```

### Network Types & Status

```cpp
enum NetworkType {
    NETWORK_NONE,        // No connection
    NETWORK_WIFI,        // Connected via WiFi
    NETWORK_ETHERNET     // Connected via Ethernet
};

enum NetworkStatus {
    NETWORK_DISCONNECTED,  // Disconnected
    NETWORK_CONNECTING,    // Attempting connection
    NETWORK_CONNECTED,     // Connected successfully
    NETWORK_ERROR          // Error state
};
```

## 🧪 Testing & Diagnostics

### Serial Commands untuk Testing

Tambahkan di serial command handler dalam loop():

```cpp
// Serial command processing for debugging
if (Serial.available()) {
    String command = Serial.readString();
    command.trim();
    command.toUpperCase();
    
    if (command == "HELP") {
        Serial.println("📋 Available Commands:");
        Serial.println("   HELP    - Show this help");
        Serial.println("   DIAG    - Network diagnostics");
        Serial.println("   PING    - Test network connectivity");
        Serial.println("   STATUS  - Show current status");
        Serial.println("   REBOOT  - Restart ESP32");
    }
    else if (command == "DIAG") {
        networkManager.printNetworkDiagnostics();
    }
    else if (command == "PING") {
        networkManager.testNetworkConnectivity();
    }
    else if (command == "STATUS") {
        Serial.println("📊 System Status:");
        Serial.printf("   Network: %s\n", networkManager.isConnected() ? "Connected" : "Disconnected");
        Serial.printf("   Type: %s\n", (networkManager.getType() == NETWORK_ETHERNET) ? "Ethernet" : "WiFi");
        Serial.printf("   IP: %s\n", networkManager.getLocalIP().c_str());
        Serial.printf("   MAC: %s\n", networkManager.getMacAddress().c_str());
    }
    else if (command == "REBOOT") {
        Serial.println("🔄 Rebooting ESP32...");
        delay(1000);
        ESP.restart();
    }
}
```

### Expected Output

```
🔍 Network Diagnostics:
================================
📶 Current Connection: Ethernet
📊 Status: Connected

🔌 Ethernet Status:
   Link Up: Yes
   IP Address: 192.168.18.99
   Gateway: 192.168.18.1
   Subnet: 255.255.255.0
   DNS: 8.8.8.8
   MAC: F0:24:F9:E5:64:7F

📶 WiFi Status:
   Connected: No
================================
```

## 🔧 Troubleshooting

### 1. Ethernet tidak dapat IP (DHCP)

**Symptoms:**
```
⏳ Waiting for DHCP IP... 10/10s
❌ Failed to get IP address via DHCP
```

**Solutions:**
- Check kabel LAN terpasang dengan benar
- Check router/switch DHCP settings enabled
- Gunakan static IP sebagai fallback
- Check LED Ethernet pada board menyala

### 2. Static IP tidak terapply

**Symptoms:**
```
❌ Static IP configuration failed
```

**Solutions:**
- Pastikan IP tidak bentrok dengan device lain
- Check subnet mask sesuai network
- Check gateway IP benar
- Pastikan network range sesuai router

### 3. WiFi tidak auto-connect

**Symptoms:**
```
❌ No suitable WiFi networks found for auto-connection
```

**Solutions:**
```cpp
// Use WiFiManager untuk manual setup
else if (command == "WIFI") {
    Serial.println("🔧 Starting WiFi Manager for manual setup...");
    if (wifiManager) {
        wifiManager->autoConnect();
    } else {
        Serial.println("⚠️ WiFi Manager not initialized");
    }
}
```

### 4. Network disconnect/reconnect berulang

**Symptoms:**
```
📡 Ethernet Connected
📡 Ethernet Got IP: 192.168.18.99
📡 Ethernet Disconnected
```

**Solutions:**
- Check kualitas kabel ethernet
- Check power supply stability (PoE)
- Check pin interference dengan RS485/sensor lain
- Disable sensor reading sementara untuk isolasi masalah

### 5. RS485 mengganggu Network Stability

**Solution:**
Disable RS485 auto-read dalam setup():

```cpp
// Disable RS485 sementara untuk testing network
// if (initRS485WithConfig()) {
//     Serial.println("✅ RS485 ModBus initialized successfully");
// } else {
//     Serial.println("⚠️ RS485 ModBus initialization failed");
// }
```

Dan dalam loop():

```cpp
// Disable RS485 sensor reading
// if (rs485ConfigSensorLDRR16.enableAutoRead && millis() - lastSensorRead >= rs485ConfigSensorLDRR16.readInterval) {
//     // ... RS485 reading code
// }
```

## 📊 Performance Comparison

| Feature   | Ethernet (PoE)   | WiFi                    |
| --------- | ---------------- | ----------------------- |
| Speed     | 🟢 100 Mbps      | 🟡 50-100 Mbps          |
| Latency   | 🟢 <1ms          | 🟡 5-20ms               |
| Stability | 🟢 Excellent     | 🟡 Good                 |
| Power     | 🟢 PoE Available | 🔴 Device Power Only    |
| Setup     | 🟢 Plug & Play   | 🟡 Requires Credentials |
| Range     | 🟢 100m cable    | 🟡 Limited by WiFi      |

## 📄 Complete Example

Lihat `src/main.cpp` untuk contoh implementasi lengkap dengan:
- Network initialization dengan priority
- Failover handling  
- API server integration
- Serial commands
- Status monitoring
- Error handling

## 🔗 Dependencies

```cpp
#include <WiFi.h>           // ESP32 WiFi library
#include <ETHClass2.h>      // Custom Ethernet library untuk T-Internet-POE
#include <WifiManager.h>    // WiFi credential management
```

## 📅 Version History

- v1.0.0 - Initial release dengan dual network support
- v1.1.0 - Tambah static IP configuration
- v1.2.0 - Tambah network diagnostics & testing
- v1.3.0 - Improved error handling & documentation
