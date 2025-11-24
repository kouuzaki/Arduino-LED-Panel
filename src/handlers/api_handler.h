#ifndef API_HANDLER_H
#define API_HANDLER_H

#include <Arduino.h>
#include <Ethernet.h>
#include "DeviceSystemInfo.h"

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
        Serial.print("API Server started on port ");
        Serial.println(API_PORT);
    }

    void handleClient()
    {
        EthernetClient client = server.available();
        if (!client)
            return;

        // --- 1. Read Request Line ---
        char requestLine[128];
        int idx = 0;
        bool foundRequestLine = false;
        unsigned long timeout = millis() + 500;

        while (client.connected() && millis() < timeout)
        {
            if (client.available())
            {
                char c = client.read();
                if (idx < (int)sizeof(requestLine) - 1)
                {
                    requestLine[idx++] = c;
                    if (idx >= 2 && requestLine[idx - 2] == '\r' && requestLine[idx - 1] == '\n')
                    {
                        requestLine[idx - 2] = '\0';
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

        // --- 2. Parse Method & Path ---
        char method[16] = {0};
        char path[128] = {0};

        // Simple parsing
        sscanf(requestLine, "%15s %127s", method, path);

        // Clean path (remove querystring)
        char *q = strchr(path, '?');
        if (q)
            *q = 0;

        Serial.print("API REQ: ");
        Serial.print(method);
        Serial.print(" ");
        Serial.println(path);

        // --- 3. Route Handler ---
        if (strcmp(method, "GET") == 0 && strcmp(path, "/api/device/info") == 0)
        {
            handleDeviceInfo(client);
        }
        else
        {
            handleNotFound(client);
        }

        // Give time for browser to receive data
        delay(5);
        client.stop();
    }

private:
    void handleDeviceInfo(EthernetClient &client)
    {
        // Buffer besar untuk Full JSON
        char jsonBuffer[512];

        // Panggil fungsi central dari DeviceSystemInfo
        SystemInfo::buildFullApiJSON(jsonBuffer, sizeof(jsonBuffer));

        // Kirim HTTP Headers
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.print("Content-Length: ");
        client.println(strlen(jsonBuffer));
        client.println("Connection: close");
        client.println("Access-Control-Allow-Origin: *"); // Optional: CORS
        client.println();

        // Kirim Body JSON
        client.print(jsonBuffer);
    }

    void handleNotFound(EthernetClient &client)
    {
        const char *msg = "{\"error\":\"Not Found\"}";
        client.println("HTTP/1.1 404 Not Found");
        client.println("Content-Type: application/json");
        client.print("Content-Length: ");
        client.println(strlen(msg));
        client.println("Connection: close");
        client.println();
        client.print(msg);
    }
};

#endif