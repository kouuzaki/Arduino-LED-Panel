#include "NetworkManager.h"
#include <SPI.h>
#include <DeviceConfig.h>

NetworkManager NetworkManager::instance;

String ipToString(IPAddress ip)
{
    char buf[20];
    snprintf(buf, sizeof(buf), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    return String(buf);
}

NetworkManager &NetworkManager::getInstance()
{
    return instance;
}

bool NetworkManager::init()
{
    currentType = NETWORK_NONE;
    currentStatus = NETWORK_DISCONNECTED;
    lastCheckTime = 0;

    Serial.println("🌐 Initializing Network Manager (Ethernet W5100)...");

    // Try Ethernet initialization
    if (initEthernet())
    {
        Serial.println("✅ Ethernet initialization successful");
        currentType = NETWORK_ETHERNET;
        currentStatus = NETWORK_CONNECTED;
        Serial.println("✓ Network Manager initialized with Ethernet");
        return true;
    }
    else
    {
        Serial.println("❌ Ethernet initialization failed");
        currentType = NETWORK_NONE;
        currentStatus = NETWORK_ERROR;
        return false;
    }
}

bool NetworkManager::initEthernet()
{
    Serial.println("🔌 Initializing Ethernet (W5100 Shield)...");

    // Initialize SPI for Ethernet
    SPI.begin();

    // Initialize Ethernet with W5100_CS_PIN (default pin 10)
    Ethernet.init(W5100_CS_PIN);

    // Get configuration from DeviceConfig
    DeviceConfig &deviceConfig = DeviceConfig::getInstance();
    String staticIPStr = deviceConfig.getDeviceIP();
    String subnetStr = deviceConfig.getSubnetMask();
    String gatewayStr = deviceConfig.getGateway();
    String dnsPrimaryStr = deviceConfig.getDnsPrimary();

    // Parse IP addresses
    IPAddress staticIP;
    IPAddress subnet;
    IPAddress gateway;
    IPAddress dnsPrimary;

    if (!staticIP.fromString(staticIPStr.c_str()))
    {
        Serial.println("⚠️ Invalid static IP, using default: 192.168.1.100");
        staticIP = IPAddress(192, 168, 1, 100);
    }

    if (!subnet.fromString(subnetStr.c_str()))
    {
        Serial.println("⚠️ Invalid subnet mask, using default: 255.255.255.0");
        subnet = IPAddress(255, 255, 255, 0);
    }

    if (!gateway.fromString(gatewayStr.c_str()))
    {
        Serial.println("⚠️ Invalid gateway, using default: 192.168.1.1");
        gateway = IPAddress(192, 168, 1, 1);
    }

    if (!dnsPrimary.fromString(dnsPrimaryStr.c_str()))
    {
        Serial.println("⚠️ Invalid DNS primary, using default: 8.8.8.8");
        dnsPrimary = IPAddress(8, 8, 8, 8);
    }

    // Configure static IP
    Serial.println("🔧 Configuring Ethernet with static IP...");

    // Ethernet.begin with static IP: begin(mac, ip, gateway, subnet, dns)
    byte mac[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
    Ethernet.begin(mac, staticIP, dnsPrimary, gateway, subnet);

    // Wait for Ethernet to be ready
    Serial.print("⏳ Waiting for Ethernet link...");
    unsigned long start = millis();
    while ((Ethernet.linkStatus() != LinkON) && (millis() - start) < 10000)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    // Check if link is up
    if (Ethernet.linkStatus() == LinkON)
    {
        Serial.println("✅ Ethernet link is up");
        Serial.print("📍 Ethernet MAC: ");
        byte mac[6];
        Ethernet.MACAddress(mac);
        printMacAddress(mac);
        Serial.print("📍 IP Address: ");
        Serial.println(ipToString(Ethernet.localIP()));
        Serial.print("📍 Gateway: ");
        Serial.println(ipToString(Ethernet.gatewayIP()));
        Serial.print("📍 Subnet Mask: ");
        Serial.println(ipToString(Ethernet.subnetMask()));
        Serial.print("📍 DNS: ");
        Serial.println(ipToString(Ethernet.dnsServerIP()));

        return true;
    }
    else
    {
        Serial.println("❌ Ethernet link is down - checking if cable connected...");
        Serial.println("⚠️ Proceeding anyway with configured static IP");
        return true; // Allow to continue even without link
    }
}

void NetworkManager::checkConnection()
{
    lastCheckTime = millis();
    checkEthernetConnection();
}

bool NetworkManager::checkEthernetConnection()
{
    if (Ethernet.linkStatus() == LinkON && Ethernet.localIP() != IPAddress(0, 0, 0, 0))
    {
        if (currentType != NETWORK_ETHERNET)
        {
            currentType = NETWORK_ETHERNET;
            currentStatus = NETWORK_CONNECTED;
            Serial.println("📡 Ethernet connection detected");
        }
        return true;
    }
    else
    {
        if (currentType != NETWORK_NONE)
        {
            currentType = NETWORK_NONE;
            currentStatus = NETWORK_DISCONNECTED;
            Serial.println("❌ Ethernet connection lost");
        }
        return false;
    }
}

bool NetworkManager::shouldUpdateStatus()
{
    return (millis() - lastCheckTime) > CHECK_INTERVAL;
}

NetworkStatus NetworkManager::getStatus()
{
    if (shouldUpdateStatus())
    {
        checkConnection();
    }
    return currentStatus;
}

NetworkType NetworkManager::getType()
{
    return currentType;
}

String NetworkManager::getLocalIP()
{
    if (currentType == NETWORK_ETHERNET)
    {
        return ipToString(Ethernet.localIP());
    }
    return "0.0.0.0";
}

String NetworkManager::getMacAddress()
{
    byte mac[6];
    Ethernet.MACAddress(mac);
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

String NetworkManager::getGatewayIP()
{
    if (currentType == NETWORK_ETHERNET)
    {
        return ipToString(Ethernet.gatewayIP());
    }
    return "0.0.0.0";
}

String NetworkManager::getSubnetMask()
{
    if (currentType == NETWORK_ETHERNET)
    {
        return ipToString(Ethernet.subnetMask());
    }
    return "0.0.0.0";
}

bool NetworkManager::isConnected()
{
    if (shouldUpdateStatus())
    {
        checkConnection();
    }
    return (currentStatus == NETWORK_CONNECTED);
}

bool NetworkManager::isEthernetAvailable()
{
    return checkEthernetConnection();
}

JsonDocument NetworkManager::getNetworkInfoJson()
{
    JsonDocument doc;

    doc["type"] = (currentType == NETWORK_ETHERNET) ? "ethernet" : "none";
    doc["connected"] = isConnected();
    doc["status"] = (currentStatus == NETWORK_CONNECTED) ? "connected" : "disconnected";
    doc["ip"] = getLocalIP();
    doc["mac"] = getMacAddress();
    doc["gateway"] = getGatewayIP();
    doc["subnet"] = getSubnetMask();
    doc["link_status"] = (Ethernet.linkStatus() == LinkON) ? "up" : "down";

    return doc;
}

// Helper function untuk print MAC address
void printMacAddress(byte *mac)
{
    for (int i = 0; i < 6; i++)
    {
        if (mac[i] < 16)
            Serial.print("0");
        Serial.print(mac[i], HEX);
        if (i < 5)
            Serial.print(":");
    }
    Serial.println();
}
