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
        // Calculate JSON size (approximate)
        // Format: {"device_id":"iot_led_panel",...} ~350 bytes
        const char *contentType = "application/json";
        const int contentLength = 350; // Approximate
        
        // Send headers first
        client.println("HTTP/1.1 200 OK");
        client.print("Content-Type: ");
        client.println(contentType);
        client.print("Content-Length: ");
        client.println(contentLength);
        client.println("Connection: close");
        client.println();
        
        // Serialize JSON directly to client
        serializeDeviceSystemInfoJson(client);
    }

    void handleNotFound(EthernetClient &client)
    {
        const char *response = "{\"error\":\"Not Found\"}";
        sendHttpHeaders(client, "application/json", strlen(response));
        client.print(response);
    }
};

#endif
