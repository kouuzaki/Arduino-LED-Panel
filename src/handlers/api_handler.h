#ifndef API_HANDLER_H
#define API_HANDLER_H

#include <Arduino.h>
#include <Ethernet.h>
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

        // Read HTTP request line with fixed buffer (no String concatenation!)
        char requestLine[128];
        int idx = 0;
        boolean foundRequestLine = false;
        unsigned long timeout = millis() + 500; // 500ms timeout
        
        while (client.connected() && millis() < timeout)
        {
            if (client.available())
            {
                char c = client.read();
                if (idx < sizeof(requestLine) - 1)
                {
                    requestLine[idx++] = c;
                    // Check for end of request line
                    if (idx >= 2 && requestLine[idx-2] == '\r' && requestLine[idx-1] == '\n')
                    {
                        requestLine[idx-2] = '\0'; // Null terminate before \r\n
                        foundRequestLine = true;
                        break;
                    }
                }
            }
        }

        if (!foundRequestLine || idx == 0)
        {
            client.stop();
            return;
        }

        // Parse request line: "GET /api/device/info HTTP/1.1"
        char method[16] = {0};
        char path[128] = {0};
        
        int parsed = sscanf(requestLine, "%15s %127s", method, path);
        
        if (parsed != 2)
        {
            handleNotFound(client);
            delay(1);
            client.stop();
            return;
        }

        // Normalize path: remove query string
        char *queryPos = strchr(path, '?');
        if (queryPos != NULL)
        {
            *queryPos = '\0'; // Null terminate at ?
        }

        // Remove trailing slash (but keep single /)
        int pathLen = strlen(path);
        if (pathLen > 1 && path[pathLen-1] == '/')
        {
            path[pathLen-1] = '\0';
        }

        // Debug: print what we received
        Serial.print("REQ: ");
        Serial.print(method);
        Serial.print(" ");
        Serial.println(path);

        // Handle GET /api/device/info
        if (strcmp(method, "GET") == 0 && strcmp(path, "/api/device/info") == 0)
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
        // Pre-calculate JSON size by building it to a buffer first
        char tempBuffer[350];
        
        unsigned long ms = millis();
        unsigned long seconds = ms / 1000;
        unsigned long hours = (seconds % 86400) / 3600;
        unsigned long minutes = (seconds % 3600) / 60;
        unsigned long secs = seconds % 60;

        IPAddress ip = Ethernet.localIP();

        extern int __heap_start, *__brkval;
        int v;
        int freeRam = (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);

        // Calculate content length
        int len = snprintf(tempBuffer, sizeof(tempBuffer),
            "{\"device_id\":\"iot_led_panel\",\"ip\":\"%d.%d.%d.%d\",\"mac_address\":\"DE:AD:BE:EF:FE:ED\",\"uptime\":\"%02lu:%02lu:%02lu\",\"uptime_ms\":%lu,\"free_memory\":%d,\"total_memory\":2048,\"network\":{\"gateway\":\"192.168.1.1\",\"subnet_mask\":\"255.255.255.0\",\"dns_primary\":\"192.168.1.1\",\"dns_secondary\":\"8.8.8.8\"},\"services\":{\"api\":\"running\",\"mqtt\":\"active\",\"led_panel\":\"scanning\"}}",
            ip[0], ip[1], ip[2], ip[3], hours, minutes, secs, ms, freeRam);
        
        // Send HTTP headers with accurate Content-Length
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.print("Content-Length: ");
        client.println(len);
        client.println("Connection: close");
        client.println();
        
        // Send JSON body
        client.print(tempBuffer);
    }

    void handleNotFound(EthernetClient &client)
    {
        const char *response = "{\"error\":\"Not Found\"}";
        sendHttpHeaders(client, "application/json", strlen(response));
        client.print(response);
    }
};

#endif
