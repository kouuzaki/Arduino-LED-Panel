#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoJson.h>
#include "ApiRouting.h"
#include <NetworkManager.h>
#include <MqttManager.h>
#include <DeviceConfig.h>
#include <time.h>
#include "interface/system-info-builder.h"

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
    // Initialize EthernetServer on port 8080
    server = new EthernetServer(8080);
    Serial.println("🔧 ApiRouting instance created (EthernetServer on port 8080)");
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
        server->begin();  // CRITICAL: Start listening on port 8080
        serverStarted = true;
        Serial.println("🚀 API Server started on port 8080");
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

    // Read HTTP request with strict timeout
    unsigned long startTime = millis();
    const unsigned long REQUEST_TIMEOUT = 200;  // 200ms timeout
    bool headerComplete = false;
    
    while ((millis() - startTime) < REQUEST_TIMEOUT)
    {
        if (client.available())
        {
            char c = client.read();
            httpRequest += c;
            startTime = millis();  // Reset timeout on each character received

            // Check if we've received end of HTTP request (blank line)
            if (httpRequest.endsWith("\n\n") || httpRequest.endsWith("\r\n\r\n"))
            {
                headerComplete = true;
                break;
            }
        }
    }

    // If we didn't get complete header, still try to parse what we have
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
            path.trim();  // Remove whitespace
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

    Serial.println("📝 Request: " + method + " " + path);
    
    // Print free RAM
    extern int __heap_start, *__brkval;
    int v;
    int freeRam = (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
    Serial.print("📊 Free RAM: ");
    Serial.print(freeRam);
    Serial.println(" bytes");

    // Route and send response directly to client (memory efficient!)
    if (path == "/" || path == "/index.html")
    {
        sendRootResponse(client);
    }
    else if (path == "/api/device/info")
    {
        sendDeviceInfoResponse(client);
    }
    else if (path == "/api/system")
    {
        sendSystemInfoResponse(client);
    }
    else if (path == "/api/config" && method == "GET")
    {
        sendConfigResponse(client);
    }
    else if (path == "/api/config" && method == "POST")
    {
        sendConfigUpdateResponse(client, body);
    }
    else
    {
        sendNotFoundResponse(client);
    }
    
    client.stop();
    Serial.println("📱 Client disconnected");
}

// ============================================================================
// MEMORY-EFFICIENT RESPONSE HANDLERS - Serialize directly to client!
// ============================================================================

void ApiRouting::sendDeviceInfoResponse(EthernetClient &client)
{
    Serial.println("🔍 Building device info...");
    
    JsonDocument doc;
    
    // Use shared helper function for consistency
    buildDeviceSystemInfo(doc);
    
    // Send HTTP headers
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    
    // Serialize JSON directly to client (NO intermediate String!)
    serializeJson(doc, client);
    
    Serial.println("✅ Device info sent");
}

void ApiRouting::sendRootResponse(EthernetClient &client)
{
    JsonDocument doc;
    
    DeviceConfig &deviceConfig = DeviceConfig::getInstance();
    String localIP = NetworkManager::getInstance().getLocalIP();
    
    doc["welcome"] = "Arduino Mega IoT Device API";
    doc["status"] = "running";
    doc["version"] = "1.0.0";
    doc["device_id"] = deviceConfig.getDeviceID();
    doc["device_ip"] = localIP;
    doc["api_port"] = 8080;
    
    // Add endpoints array
    JsonArray endpoints = doc["endpoints"].to<JsonArray>();
    
    JsonObject ep0 = endpoints.add<JsonObject>();
    ep0["method"] = "GET";
    ep0["path"] = "/";
    ep0["description"] = "Root endpoint - Welcome message";
    
    JsonObject ep1 = endpoints.add<JsonObject>();
    ep1["method"] = "GET";
    ep1["path"] = "/api/device/info";
    ep1["description"] = "Get device information";
    
    JsonObject ep2 = endpoints.add<JsonObject>();
    ep2["method"] = "GET";
    ep2["path"] = "/api/system";
    ep2["description"] = "Get system status";
    
    JsonObject ep3 = endpoints.add<JsonObject>();
    ep3["method"] = "GET";
    ep3["path"] = "/api/config";
    ep3["description"] = "Get configuration";
    
    JsonObject ep4 = endpoints.add<JsonObject>();
    ep4["method"] = "POST";
    ep4["path"] = "/api/config";
    ep4["description"] = "Update configuration";
    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    serializeJson(doc, client);
}

void ApiRouting::sendSystemInfoResponse(EthernetClient &client)
{
    JsonDocument doc;
    
    // Use shared helper function
    buildDeviceSystemInfo(doc);
    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    serializeJson(doc, client);
}

void ApiRouting::sendConfigResponse(EthernetClient &client)
{
    JsonDocument doc;
    DeviceConfig &deviceConfig = DeviceConfig::getInstance();
    
    doc["device_id"] = deviceConfig.getDeviceID();
    doc["device_ip"] = deviceConfig.getDeviceIP();
    doc["subnet_mask"] = deviceConfig.getSubnetMask();
    doc["gateway"] = deviceConfig.getGateway();
    doc["dns_primary"] = deviceConfig.getDnsPrimary();
    doc["dns_secondary"] = deviceConfig.getDnsSecondary();
    doc["mqtt_server"] = deviceConfig.getMqttServer();
    doc["mqtt_port"] = deviceConfig.getMqttPort();
    doc["mqtt_username"] = deviceConfig.getMqttUsername();
    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    serializeJson(doc, client);
}

void ApiRouting::sendConfigUpdateResponse(EthernetClient &client, const String &body)
{
    JsonDocument bodyDoc;
    DeserializationError error = deserializeJson(bodyDoc, body);
    
    JsonDocument responseDoc;
    
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
        if (bodyDoc["device_id"].is<const char *>())
        {
            deviceConfig.setDeviceID(bodyDoc["device_id"].as<String>());
            updated = true;
            Serial.println("✏️ Device ID updated");
        }
        
        // Update device_ip if provided
        if (bodyDoc["device_ip"].is<const char *>())
        {
            deviceConfig.setDeviceIP(bodyDoc["device_ip"].as<String>());
            updated = true;
            Serial.println("✏️ Device IP updated");
        }
        
        // Update MQTT server if provided
        if (bodyDoc["mqtt_server"].is<const char *>())
        {
            deviceConfig.setMqttServer(bodyDoc["mqtt_server"].as<String>());
            updated = true;
            Serial.println("✏️ MQTT Server updated");
        }
        
        // Update MQTT port if provided
        if (bodyDoc["mqtt_port"].is<int>())
        {
            deviceConfig.setMqttPort(bodyDoc["mqtt_port"].as<int>());
            updated = true;
            Serial.println("✏️ MQTT Port updated");
        }
        
        // Update MQTT username if provided
        if (bodyDoc["mqtt_username"].is<const char *>())
        {
            deviceConfig.setMqttUsername(bodyDoc["mqtt_username"].as<String>());
            updated = true;
            Serial.println("✏️ MQTT Username updated");
        }
        
        // Update MQTT password if provided
        if (bodyDoc["mqtt_password"].is<const char *>())
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
                responseDoc["message"] = "Configuration updated successfully";
                Serial.println("💾 Configuration saved to EEPROM");
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
    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    serializeJson(responseDoc, client);
}

void ApiRouting::sendNotFoundResponse(EthernetClient &client)
{
    JsonDocument doc;
    doc["error"] = "not_found";
    doc["message"] = "Endpoint not found";
    
    client.println("HTTP/1.1 404 Not Found");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    serializeJson(doc, client);
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
