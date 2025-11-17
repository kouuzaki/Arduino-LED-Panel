#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <api/ApiRouting.h>
#include <mqtt/MqttRouting.h>
#include <NetworkManager.h>
#include <MqttManager.h>
#include <DeviceConfig.h>

// --- Global Managers & Instances ---
DeviceConfig &deviceConfig = DeviceConfig::getInstance();
MqttManager *mqttManager = nullptr;
bool apiServerStarted = false;

// Timing variables
unsigned long lastNetCheck = 0;
unsigned long lastMqttTry = 0;
unsigned long lastHeartbeat = 0;

void printConfiguration()
{
    Serial.println("\n========================================");
    Serial.println("  Arduino Mega Configuration Status");
    Serial.println("========================================");
    Serial.println("Device ID    : " + deviceConfig.getDeviceID());
    Serial.println("Device IP    : " + deviceConfig.getDeviceIP());
    Serial.println("MQTT Server  : " + (deviceConfig.getMqttServer().length() > 0 ? deviceConfig.getMqttServer() : "(not configured)"));
    Serial.println("MQTT Port    : " + String(deviceConfig.getMqttPort()));
    Serial.println("========================================\n");

    if (deviceConfig.getMqttServer().length() == 0)
    {
        Serial.println("⚠️  MQTT not configured!");
        Serial.println("💡 Use API: POST /api/config to configure\n");
    }
}

bool startNetwork()
{
    NetworkManager &networkManager = NetworkManager::getInstance();
    Serial.println("🌐 Starting network (Ethernet W5100)...");

    if (networkManager.init() && networkManager.isConnected())
    {
        Serial.println("✅ Ethernet connected: " + networkManager.getLocalIP());
        return true;
    }
    else
    {
        Serial.println("❌ Ethernet connection failed");
        return false;
    }
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n");
    Serial.println("========================================");
    Serial.println("  Arduino Mega IoT - Ethernet + MQTT");
    Serial.println("========================================");

    Serial.println("\n🔧 Initializing Device Configuration...");
    
    // SELALU set defaults dulu SEBELUM init, supaya nilai ini digunakan
    deviceConfig.setCustomDefaults("arduino_mega_eth",
                                   "192.168.1.60",
                                   "255.255.255.0",
                                   "192.168.1.1",
                                   "8.8.8.8",
                                   "1.1.1.1",
                                   1884,
                                   "192.168.1.1",
                                   "edgeadmin",
                                   "edge123");
    
    // Init dengan forceDefaults=true agar selalu simpan defaults ke EEPROM
    bool configExists = deviceConfig.init(true);
    
    Serial.println("✅ Device Configuration initialized and saved to EEPROM");
    Serial.println("📝 IP: 192.168.1.60, MQTT Server: 192.168.1.1:1883");

    printConfiguration();

    Serial.println("🌐 Starting network...");
    bool netOk = startNetwork();
    if (!netOk)
    {
        Serial.println("⚠️  Network not available. Will keep trying in loop.");
    }

    Serial.println("📡 Initializing MQTT...");
    mqttManager = new MqttManager();
    mqttManager->init();

    String mqtt_server = deviceConfig.getMqttServer();
    if (mqtt_server.length() > 0)
    {
        mqttManager->setConfig(
            deviceConfig.getMqttUsername(),
            deviceConfig.getMqttPassword(),
            mqtt_server,
            deviceConfig.getMqttPort());
        Serial.println("✅ MQTT configured: " + mqtt_server + ":" + String(deviceConfig.getMqttPort()));
    }
    else
    {
        Serial.println("⚠️  MQTT not configured");
    }

    Serial.println("🌐 Setting up API routes...");
    ApiRouting &apiRouting = ApiRouting::getInstance();
    apiRouting.setupRoutes();
    Serial.println("✅ API routes configured");

    Serial.println("📋 Initializing MQTT Routing...");
    MqttRouting &mqttRouting = MqttRouting::getInstance();
    mqttRouting.init();
    Serial.println("✅ MQTT Routing initialized successfully");

    if (NetworkManager::getInstance().isConnected())
    {
        apiRouting.start();
        apiServerStarted = true;
        Serial.println("✅ API started at: http://" + NetworkManager::getInstance().getLocalIP());

        if (mqtt_server.length() > 0)
        {
            Serial.println("🔌 Connecting to MQTT broker...");
            if (mqttManager->connect())
            {
                Serial.println("✅ MQTT connected");
                Serial.println("📋 Subscribing to MQTT topics...");
                if (mqttRouting.subscribeAllRoutes())
                {
                    Serial.println("✅ MQTT subscriptions successful");
                }
            }
            else
            {
                Serial.println("⚠️  MQTT connect failed, will retry in loop");
            }
        }
    }

    Serial.println("\n========================================");
    Serial.println("  Setup Complete - System Ready");
    Serial.println("========================================\n");
}

void loop()
{
    if (apiServerStarted && NetworkManager::getInstance().isConnected())
    {
        ApiRouting::getInstance().handleClient();
    }

    if (millis() - lastNetCheck > 5000)
    {
        lastNetCheck = millis();
        if (!NetworkManager::getInstance().isConnected())
        {
            Serial.println("⚠️  Network down, attempting reconnect...");
            startNetwork();
        }
    }

    if (mqttManager)
    {
        mqttManager->loop();

        if (!mqttManager->isConnected() && deviceConfig.getMqttServer().length() > 0)
        {
            if (millis() - lastMqttTry > 10000)
            {
                lastMqttTry = millis();
                Serial.println("🔄 Retrying MQTT connection...");
                if (mqttManager->connect())
                {
                    MqttRouting::getInstance().subscribeAllRoutes();
                }
            }
        }
    }

    if (NetworkManager::getInstance().isConnected() && !apiServerStarted)
    {
        ApiRouting::getInstance().start();
        apiServerStarted = true;
        Serial.println("✅ API server started at: http://" + NetworkManager::getInstance().getLocalIP());
    }

    if (millis() - lastHeartbeat > 30000)
    {
        lastHeartbeat = millis();
        Serial.println("💓 Heartbeat - Device ID: " + deviceConfig.getDeviceID() +
                       " | Network: " + (NetworkManager::getInstance().isConnected() ? "✅" : "❌") +
                       " | MQTT: " + (mqttManager && mqttManager->isConnected() ? "✅" : "❌"));
    }

    delay(10);
}

