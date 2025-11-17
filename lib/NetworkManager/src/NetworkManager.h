#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <Ethernet.h>
#include <ArduinoJson.h>

enum NetworkType
{
    NETWORK_NONE,
    NETWORK_ETHERNET
};

enum NetworkStatus
{
    NETWORK_DISCONNECTED,
    NETWORK_CONNECTING,
    NETWORK_CONNECTED,
    NETWORK_ERROR
};

// W5100 Pin Configuration (SPI)
// CS (Chip Select) pin untuk W5100 - biasanya pin 10 untuk Arduino UNO
#define W5100_CS_PIN 10

// Ethernet Shield W5100 menggunakan SPI pins:
// MOSI: Pin 11
// MISO: Pin 12
// SCK: Pin 13
// CS: Pin 10 (dapat dikonfigurasi via Ethernet.init())

/**
 * @brief NetworkManager untuk Arduino UNO + Ethernet Shield W5100
 * Mengelola koneksi Ethernet dengan static IP configuration
 */
class NetworkManager
{
public:
    static NetworkManager &getInstance();

    // Initialize network (Ethernet only)
    bool init();

    // Initialize Ethernet connection with static IP
    bool initEthernet();

    // Get network status
    NetworkStatus getStatus();
    NetworkType getType();

    // Get connection info
    String getLocalIP();
    String getMacAddress();
    String getGatewayIP();
    String getSubnetMask();

    // Check connection status
    bool isConnected();
    bool isEthernetAvailable();

    // Get network info as JSON document
    JsonDocument getNetworkInfoJson();

    // Uncomment for debugging (uses extra RAM)
    // void printNetworkDiagnostics();

private:
    NetworkManager() = default;
    NetworkType currentType;
    NetworkStatus currentStatus;
    unsigned long lastCheckTime;
    static const unsigned long CHECK_INTERVAL = 5000; // Check every 5 seconds

    void checkConnection();
    bool checkEthernetConnection();
    bool shouldUpdateStatus();

    static NetworkManager instance;

    // Helper function to print MAC address
    friend void printMacAddress(byte *mac);
};

void printMacAddress(byte *mac);

#endif
