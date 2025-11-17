#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoJson.h>
#include "ApiRouting.h"
#include <NetworkManager.h>
#include <MqttManager.h>
#include <DeviceConfig.h>
#include "interface/secure-json-response.h"
#include "interface/device-system-info.h"
#include <time.h>

// External references
extern MqttManager *mqttManager;

// Initialize static instance pointer
ApiRouting *ApiRouting::instance = nullptr;

// Singleton instance getter
ApiRouting &ApiRouting::getInstance()
{
    if (instance == nullptr)
    {
        instance = new ApiRouting();
    }
    return *instance;
}

// Private constructor
ApiRouting::ApiRouting() : serverStarted(false)
{
    // Initialize EthernetServer on port 80
    server = new EthernetServer(80);
    Serial.println("🔧 ApiRouting instance created (EthernetServer)");
}

// Destructor
ApiRouting::~ApiRouting()
{
    stop();
    if (server)
    {
        delete server;
        server = nullptr;
    }
}

// Start the server
void ApiRouting::start()
{
    if (server && !serverStarted)
    {
        serverStarted = true;
        Serial.println("🚀 API Server started on port 80");
    }
}

// Stop the server
void ApiRouting::stop()
{
    if (server && serverStarted)
    {
        serverStarted = false;
        Serial.println("⏹️ API Server stopped");
    }
}

// Handle client requests - Called from main loop
void ApiRouting::handleClient()
{
    if (!server || !serverStarted)
        return;

    // Listen for incoming clients
    EthernetClient client = server->available();
    if (client)
    {
        handleHttpRequest(client);
    }
}

// Parse and handle HTTP requests
void ApiRouting::handleHttpRequest(EthernetClient &client)
{
    String httpRequest = "";
    String method = "";
    String path = "";
    String body = "";

    // Read HTTP request
    while (client.connected())
    {
        if (client.available())
        {
            char c = client.read();
            httpRequest += c;

            // Check if we've received end of HTTP request (blank line)
            if (httpRequest.endsWith("\n\n") || httpRequest.endsWith("\r\n\r\n"))
            {
                break;
            }
        }
    }

    if (httpRequest.length() == 0)
    {
        client.stop();
        return;
    }

    // Parse HTTP request line
    int firstNewline = httpRequest.indexOf('\n');
    if (firstNewline > 0)
    {
        String firstLine = httpRequest.substring(0, firstNewline);
        int firstSpace = firstLine.indexOf(' ');
        int secondSpace = firstLine.indexOf(' ', firstSpace + 1);

        if (firstSpace > 0 && secondSpace > firstSpace)
        {
            method = firstLine.substring(0, firstSpace);
            path = firstLine.substring(firstSpace + 1, secondSpace);
        }
    }

    // Extract request body for POST requests
    int bodyStart = httpRequest.indexOf("\r\n\r\n");
    if (bodyStart < 0)
        bodyStart = httpRequest.indexOf("\n\n");

    if (bodyStart >= 0 && method == "POST")
    {
        bodyStart += (httpRequest.indexOf("\r\n\r\n") >= 0) ? 4 : 2;
        body = httpRequest.substring(bodyStart);
    }

    // Route requests
    JsonDocument responseDoc;
    bool notFound = false;

    if (path == "/" || path == "/index.html")
    {
        // Root endpoint
        JsonArray data = responseDoc.createNestedArray("data");
        JsonObject rootInfo = data.createNestedObject();

        DeviceConfig &deviceConfig = DeviceConfig::getInstance();
        String localIP = NetworkManager::getInstance().getLocalIP();

        rootInfo["welcome"] = "Arduino UNO IoT Device API";
        rootInfo["status"] = "running";
        rootInfo["version"] = "1.0.0";
        rootInfo["device_id"] = deviceConfig.getDeviceID();
        rootInfo["device_ip"] = localIP;
        rootInfo["api_port"] = 80;

        // Available endpoints
        JsonArray endpoints = rootInfo.createNestedArray("endpoints");

        JsonObject ep0 = endpoints.createNestedObject();
        ep0["method"] = "GET";
        ep0["path"] = "/";
        ep0["description"] = "Root endpoint - Welcome message";

        JsonObject ep1 = endpoints.createNestedObject();
        ep1["method"] = "GET";
        ep1["path"] = "/api/device/info";
        ep1["description"] = "Get device information";

        JsonObject ep2 = endpoints.createNestedObject();
        ep2["method"] = "GET";
        ep2["path"] = "/api/system";
        ep2["description"] = "Get system status";

        JsonObject ep3 = endpoints.createNestedObject();
        ep3["method"] = "GET";
        ep3["path"] = "/api/config";
        ep3["description"] = "Get configuration";

        JsonObject ep4 = endpoints.createNestedObject();
        ep4["method"] = "POST";
        ep4["path"] = "/api/config";
        ep4["description"] = "Update configuration";
    }
    else if (path == "/api/device/info")
    {
        // Device info endpoint
        JsonArray data = responseDoc.createNestedArray("data");
        JsonDocument deviceInfoDoc = buildDeviceSystemInfoJson();
        data.add(deviceInfoDoc.as<JsonObject>());
    }
    else if (path == "/api/system")
    {
        // System status endpoint
        JsonArray data = responseDoc.createNestedArray("data");
        JsonDocument deviceInfoDoc = buildDeviceSystemInfoJson();
        data.add(deviceInfoDoc.as<JsonObject>());
    }
    else if (path == "/api/config" && method == "GET")
    {
        // Get configuration
        DeviceConfig &deviceConfig = DeviceConfig::getInstance();

        JsonArray data = responseDoc.createNestedArray("data");
        JsonObject config = data.createNestedObject();

        config["device_id"] = deviceConfig.getDeviceID();
        config["device_ip"] = deviceConfig.getDeviceIP();
        config["subnet_mask"] = deviceConfig.getSubnetMask();
        config["gateway"] = deviceConfig.getGateway();
        config["dns_primary"] = deviceConfig.getDnsPrimary();
        config["dns_secondary"] = deviceConfig.getDnsSecondary();
        config["mqtt_server"] = deviceConfig.getMqttServer();
        config["mqtt_port"] = deviceConfig.getMqttPort();
        config["mqtt_username"] = deviceConfig.getMqttUsername();
    }
    else if (path == "/api/config" && method == "POST")
    {
        // Update configuration
        JsonDocument bodyDoc;
        DeserializationError error = deserializeJson(bodyDoc, body);

        if (error)
        {
            responseDoc["status"] = "error";
            responseDoc["message"] = "Invalid JSON format";
        }
        else
        {
            DeviceConfig &deviceConfig = DeviceConfig::getInstance();
            bool updated = false;

            // Update device_id if provided
            if (bodyDoc.containsKey("device_id") && bodyDoc["device_id"].is<const char *>())
            {
                deviceConfig.setDeviceID(bodyDoc["device_id"].as<String>());
                updated = true;
                Serial.println("✏️ Device ID updated");
            }

            // Update device_ip if provided
            if (bodyDoc.containsKey("device_ip") && bodyDoc["device_ip"].is<const char *>())
            {
                deviceConfig.setDeviceIP(bodyDoc["device_ip"].as<String>());
                updated = true;
                Serial.println("✏️ Device IP updated");
            }

            // Update MQTT server if provided
            if (bodyDoc.containsKey("mqtt_server") && bodyDoc["mqtt_server"].is<const char *>())
            {
                deviceConfig.setMqttServer(bodyDoc["mqtt_server"].as<String>());
                updated = true;
                Serial.println("✏️ MQTT Server updated");
            }

            // Update MQTT port if provided
            if (bodyDoc.containsKey("mqtt_port") && bodyDoc["mqtt_port"].is<int>())
            {
                deviceConfig.setMqttPort(bodyDoc["mqtt_port"].as<int>());
                updated = true;
                Serial.println("✏️ MQTT Port updated");
            }

            // Update MQTT username if provided
            if (bodyDoc.containsKey("mqtt_username") && bodyDoc["mqtt_username"].is<const char *>())
            {
                deviceConfig.setMqttUsername(bodyDoc["mqtt_username"].as<String>());
                updated = true;
                Serial.println("✏️ MQTT Username updated");
            }

            // Update MQTT password if provided
            if (bodyDoc.containsKey("mqtt_password") && bodyDoc["mqtt_password"].is<const char *>())
            {
                deviceConfig.setMqttPassword(bodyDoc["mqtt_password"].as<String>());
                updated = true;
                Serial.println("✏️ MQTT Password updated");
            }

            if (updated)
            {
                if (deviceConfig.saveConfig())
                {
                    responseDoc["status"] = "success";
                    responseDoc["message"] = "Configuration updated - Device restarting to apply changes";
                    Serial.println("💾 Configuration saved to EEPROM");
                    Serial.println("⏰ Scheduling device restart in 2 seconds...");
                }
                else
                {
                    responseDoc["status"] = "error";
                    responseDoc["message"] = "Failed to save configuration";
                }
            }
            else
            {
                responseDoc["status"] = "warning";
                responseDoc["message"] = "No configuration fields provided";
            }
        }
    }
    else
    {
        // 404 Not Found
        notFound = true;
        responseDoc["status"] = "error";
        responseDoc["message"] = "Endpoint not found";
    }

    // Serialize JSON response
    String jsonResponse;
    serializeJson(responseDoc, jsonResponse);

    // Send HTTP response header
    if (notFound)
    {
        client.println("HTTP/1.1 404 Not Found");
    }
    else
    {
        client.println("HTTP/1.1 200 OK");
    }

    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();

    // Send response body
    client.println(jsonResponse);

    // Give browser time to receive data
    delay(1);

    // Close connection
    client.stop();
    Serial.println("📱 Client disconnected");
}

// Setup routes - placeholder untuk compatibility
void ApiRouting::setupRoutes()
{
    if (!server)
    {
        Serial.println("❌ Server not initialized");
        return;
    }

    Serial.println("✅ API routes setup complete");
}
