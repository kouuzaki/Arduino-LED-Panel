#ifndef API_HANDLER_H
#define API_HANDLER_H

#include <Arduino.h>
#include <Ethernet.h>
#include <ArduinoJson.h>
#include <DeviceSystemInfo.h>

class ApiHandler
{
private:
    EthernetServer server;
    static const uint16_t API_PORT = 8080;

public:
    ApiHandler() : server(API_PORT) {}

    void begin()
    {
        server.begin();
        Serial.print("API Server on port ");
        Serial.println(API_PORT);
    }

    void handleClient()
    {
        EthernetClient client = server.available();
        if (!client)
            return;

        // Read HTTP request line by line
        String requestLine = "";
        boolean foundRequestLine = false;
        
        while (client.connected())
        {
            if (client.available())
            {
                char c = client.read();
                requestLine += c;
                
                // Check if we've read the complete request line
                if (requestLine.endsWith("\r\n"))
                {
                    foundRequestLine = true;
                    break;
                }
            }
        }

        if (!foundRequestLine)
        {
            client.stop();
            return;
        }

        // Parse request line: "GET /api/device/info HTTP/1.1\r\n"
        int firstSpace = requestLine.indexOf(' ');
        int secondSpace = requestLine.indexOf(' ', firstSpace + 1);
        
        if (firstSpace == -1 || secondSpace == -1)
        {
            handleNotFound(client);
            delay(1);
            client.stop();
            return;
        }

        String method = requestLine.substring(0, firstSpace);
        String path = requestLine.substring(firstSpace + 1, secondSpace);

        // Normalize path: remove query string
        int queryPos = path.indexOf('?');
        if (queryPos != -1)
        {
            path = path.substring(0, queryPos);
        }

        // Remove trailing slash
        if (path.length() > 1 && path.endsWith("/"))
        {
            path = path.substring(0, path.length() - 1);
        }

        // Debug: print what we received
        Serial.print("REQ: ");
        Serial.print(method);
        Serial.print(" ");
        Serial.println(path);

        // Handle GET /api/device/info
        if (method == "GET" && path == "/api/device/info")
        {
            handleDeviceInfo(client);
        }
        else
        {
            handleNotFound(client);
        }

        delay(1);
        client.stop();
    }

private:
    void sendHttpHeaders(EthernetClient &client, const char *contentType, int contentLength)
    {
        client.println("HTTP/1.1 200 OK");
        client.print("Content-Type: ");
        client.println(contentType);
        client.print("Content-Length: ");
        client.println(contentLength);
        client.println("Connection: close");
        client.println();
    }

    void handleDeviceInfo(EthernetClient &client)
    {
        // Build response to temporary buffer first
        char buffer[400];
        
        // Create doc dan serialize to buffer
        StaticJsonDocument<256> doc;
        
        // Device info
        doc["device_id"] = "iot_led_panel";

        // Calculate uptime
        unsigned long ms = millis();
        unsigned long seconds = ms / 1000;
        unsigned long hours = (seconds % 86400) / 3600;
        unsigned long minutes = (seconds % 3600) / 60;
        unsigned long secs = seconds % 60;

        // IP Address
        IPAddress ip = Ethernet.localIP();
        char ipBuf[16];
        snprintf(ipBuf, sizeof(ipBuf), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
        doc["ip"] = ipBuf;

        doc["mac_address"] = "DE:AD:BE:EF:FE:ED";

        // Uptime string HH:mm:ss
        char uptimeBuf[12];
        snprintf(uptimeBuf, sizeof(uptimeBuf), "%02lu:%02lu:%02lu", hours, minutes, secs);
        doc["uptime"] = uptimeBuf;

        doc["uptime_ms"] = ms;

        // Free memory
        extern int __heap_start, *__brkval;
        int v;
        int freeRam = (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
        doc["free_memory"] = freeRam;
        doc["total_memory"] = 2048;

        // Network config
        JsonObject network = doc.createNestedObject("network");
        network["gateway"] = "192.168.1.1";
        network["subnet_mask"] = "255.255.255.0";
        network["dns_primary"] = "192.168.1.1";
        network["dns_secondary"] = "8.8.8.8";

        // Services
        JsonObject services = doc.createNestedObject("services");
        services["api"] = "running";
        services["mqtt"] = "active";
        services["led_panel"] = "scanning";

        // Serialize to buffer
        int contentLength = serializeJson(doc, buffer, sizeof(buffer));
        
        // Send headers with accurate Content-Length
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.print("Content-Length: ");
        client.println(contentLength);
        client.println("Connection: close");
        client.println();
        
        // Send JSON body
        client.print(buffer);
    }

    void handleNotFound(EthernetClient &client)
    {
        const char *response = "{\"error\":\"Not Found\"}";
        sendHttpHeaders(client, "application/json", strlen(response));
        client.print(response);
    }
};

#endif
