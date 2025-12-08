#ifndef API_HANDLER_H
#define API_HANDLER_H

#include <Arduino.h>
#include <Ethernet.h>
#include <avr/wdt.h>
#include "../interface/DeviceSystemInfo.h"
#include "storage/FileStorage.h"
#include <ArduinoJson.h>

// Forward declare MqttDisplayHandler
class MqttDisplayHandler;

class ApiHandler
{
private:
    EthernetServer server;
    static const uint16_t API_PORT = 8080;
    MqttDisplayHandler *displayHandler; // Pointer to display handler

public:
    ApiHandler() : server(API_PORT), displayHandler(nullptr) {}

    void begin()
    {
        server.begin();
        Serial.print("API Server started on port ");
        Serial.println(API_PORT);
    }

    /**
     * @brief Set reference to MqttDisplayHandler for display commands
     * @param handler Pointer to initialized MqttDisplayHandler
     */
    void setDisplayHandler(MqttDisplayHandler *handler)
    {
        displayHandler = handler;
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

        // --- 3. Read headers (for POST content length) ---
        int contentLength = 0;
        unsigned long deadline = millis() + 1000;

        // Read headers line by line
        while (client.connected() && millis() < deadline)
        {
            if (client.available())
            {
                String line = client.readStringUntil('\n');
                line.trim(); // Remove \r

                if (line.length() == 0)
                {
                    // Empty line = End of Headers
                    break;
                }

                // Check for Content-Length (Case Insensitive)
                if (line.startsWith("Content-Length:") || line.startsWith("content-length:") || line.startsWith("Content-length:"))
                {
                    int separatorIndex = line.indexOf(':');
                    if (separatorIndex != -1)
                    {
                        String val = line.substring(separatorIndex + 1);
                        val.trim();
                        contentLength = val.toInt();
                    }
                }
            }
        }

        // --- 4. Route Handler ---
        if (strcmp(method, "GET") == 0 && strcmp(path, "/api/device/info") == 0)
        {
            handleDeviceInfo(client);
        }
        else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/display/text") == 0)
        {
            handleDisplayText(client, contentLength);
        }
        else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/display/clear") == 0)
        {
            handleDisplayClear(client);
        }
        else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/display/brightness") == 0)
        {
            handleDisplayBrightness(client, contentLength);
        }
        else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/device/config") == 0)
        {
            // Read body if contentLength provided
            if (contentLength <= 0 || contentLength > 2048)
            {
                // Bad content length
                client.println("HTTP/1.1 400 Bad Request");
                client.println("Content-Type: application/json");
                client.println("Connection: close");
                client.println();
                client.print("{\"error\":\"invalid content-length\"}");
            }
            else
            {
                // Read body
                char *body = (char *)malloc(contentLength + 1);
                if (!body)
                {
                    client.println("HTTP/1.1 500 Internal Server Error");
                    client.println("Content-Type: application/json");
                    client.println("Connection: close");
                    client.println();
                    client.print("{\"error\":\"out of memory\"}");
                }
                else
                {
                    int read = 0;
                    unsigned long deadline = millis() + 1000;
                    while (read < contentLength && millis() < deadline)
                    {
                        if (client.available())
                        {
                            body[read++] = client.read();
                        }
                    }
                    body[read] = '\0';

                    // parse JSON
                    DynamicJsonDocument doc(2048);
                    DeserializationError err = deserializeJson(doc, body);
                    free(body);

                    if (err)
                    {
                        client.println("HTTP/1.1 400 Bad Request");
                        client.println("Content-Type: application/json");
                        client.println("Connection: close");
                        client.println();
                        client.print("{\"error\":\"invalid json\"}");
                    }
                    else
                    {
                        // validate fields and save
                        JsonObject obj = doc.as<JsonObject>();
                        // minimal validation
                        if (!obj.containsKey("device_id") || !obj.containsKey("mqtt_server"))
                        {
                            client.println("HTTP/1.1 422 Unprocessable Entity");
                            client.println("Content-Type: application/json");
                            client.println("Connection: close");
                            client.println();
                            client.print("{\"error\":\"missing required fields\"}");
                        }
                        else
                        {
                            // Optional: accept mqtt_heartbeat_interval as string or numeric
                            long hb = 0;
                            if (obj.containsKey("mqtt_heartbeat_interval"))
                            {
                                // Accept numeric or string values
                                if (obj["mqtt_heartbeat_interval"].is<long>())
                                {
                                    hb = obj["mqtt_heartbeat_interval"];
                                }
                                else if (obj["mqtt_heartbeat_interval"].is<const char *>())
                                {
                                    const char *s = obj["mqtt_heartbeat_interval"];
                                    hb = strtol(s, nullptr, 10);
                                }

                                // Validate range (1 ms .. 86400000 ms)
                                if (hb <= 0 || hb > 86400000L)
                                {
                                    client.println("HTTP/1.1 422 Unprocessable Entity");
                                    client.println("Content-Type: application/json");
                                    client.println("Connection: close");
                                    client.println();
                                    client.print("{\"error\":\"invalid mqtt_heartbeat_interval (must be 1..86400000)\"}");
                                    client.stop();
                                    return;
                                }

                                // Normalize: store as numeric value in JSON
                                obj["mqtt_heartbeat_interval"] = hb;

                                // Apply immediately to runtime manager (if available)
                                // mqttManager is declared extern in DeviceSystemInfo.h
                                mqttManager.setHeartbeatInterval((unsigned long)hb);
                            }

                            bool ok = FileStorage::saveDeviceConfig(doc);
                            if (ok)
                            {
                                client.println("HTTP/1.1 200 OK");
                                client.println("Content-Type: application/json");
                                client.println("Connection: close");
                                client.println();
                                client.print("{\"ok\":true,\"message\":\"Config saved. Device restarting...\"}");

                                // Close connection and delay to ensure response is sent
                                delay(100);

                                // Trigger software reset
                                triggerReset();
                            }
                            else
                            {
                                client.println("HTTP/1.1 500 Internal Server Error");
                                client.println("Content-Type: application/json");
                                client.println("Connection: close");
                                client.println();
                                client.print("{\"error\":\"failed to save\"}");
                            }
                        }
                    }
                }
            }
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
    // Trigger software reset via watchdog timer
    void triggerReset()
    {
        // Disable interrupts
        cli();

        // Enable watchdog timer with shortest timeout (~16ms)
        wdt_enable(WDTO_15MS);

        // Wait for watchdog to trigger reset
        while (1)
            ;
    }

    void handleDeviceInfo(EthernetClient &client)
    {
        // Build standard API response
        JsonDocument response;
        SystemInfo::buildFullApiResponse(response);

        // Send Response
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println("Connection: close");
        client.println();

        serializeJson(response, client);
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

    // POST /api/display/text - Display text on LED matrix
    // Body: {"text":"HELLO\nWORLD", "brightness":200}
    void handleDisplayText(EthernetClient &client, int contentLength)
    {
        if (!displayHandler)
        {
            client.println("HTTP/1.1 503 Service Unavailable");
            client.println("Content-Type: application/json");
            client.println("Connection: close");
            client.println();
            client.print("{\"error\":\"display service not available\"}");
            return;
        }

        if (contentLength <= 0 || contentLength > 512)
        {
            client.println("HTTP/1.1 400 Bad Request");
            client.println("Content-Type: application/json");
            client.println("Connection: close");
            client.println();
            client.print("{\"error\":\"invalid content-length\"}");
            return;
        }

        char *body = (char *)malloc(contentLength + 1);
        if (!body)
        {
            client.println("HTTP/1.1 500 Internal Server Error");
            client.println("Content-Type: application/json");
            client.println("Connection: close");
            client.println();
            client.print("{\"error\":\"out of memory\"}");
            return;
        }

        int read = 0;
        unsigned long deadline = millis() + 1000;
        while (read < contentLength && millis() < deadline)
        {
            if (client.available())
            {
                body[read++] = client.read();
            }
        }
        body[read] = '\0';

        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, body);
        free(body);

        if (err)
        {
            client.println("HTTP/1.1 400 Bad Request");
            client.println("Content-Type: application/json");
            client.println("Connection: close");
            client.println();
            client.print("{\"error\":\"invalid json\"}");
            return;
        }

        if (!doc.containsKey("text"))
        {
            client.println("HTTP/1.1 422 Unprocessable Entity");
            client.println("Content-Type: application/json");
            client.println("Connection: close");
            client.println();
            client.print("{\"error\":\"missing required field: text\"}");
            return;
        }

        const char *text = doc["text"];

        // Optional brightness
        if (doc.containsKey("brightness"))
        {
            int brightness = doc["brightness"];
            if (brightness < 0)
                brightness = 0;
            if (brightness > 255)
                brightness = 255;
            displayHandler->setBrightness(brightness);
        }

        // Display text
        displayHandler->displayText(text);

        // Send response
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println("Connection: close");
        client.println();
        client.print("{\"ok\":true,\"message\":\"Text displayed\",\"action\":\"text\"}");
    }

    // POST /api/display/clear - Clear display
    void handleDisplayClear(EthernetClient &client)
    {
        if (!displayHandler)
        {
            client.println("HTTP/1.1 503 Service Unavailable");
            client.println("Content-Type: application/json");
            client.println("Connection: close");
            client.println();
            client.print("{\"error\":\"display service not available\"}");
            return;
        }

        displayHandler->clearDisplay();

        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println("Connection: close");
        client.println();
        client.print("{\"ok\":true,\"message\":\"Display cleared\",\"action\":\"clear\"}");
    }

    // POST /api/display/brightness - Set display brightness
    // Body: {"brightness":200}
    void handleDisplayBrightness(EthernetClient &client, int contentLength)
    {
        if (!displayHandler)
        {
            client.println("HTTP/1.1 503 Service Unavailable");
            client.println("Content-Type: application/json");
            client.println("Connection: close");
            client.println();
            client.print("{\"error\":\"display service not available\"}");
            return;
        }

        if (contentLength <= 0 || contentLength > 128)
        {
            client.println("HTTP/1.1 400 Bad Request");
            client.println("Content-Type: application/json");
            client.println("Connection: close");
            client.println();
            client.print("{\"error\":\"invalid content-length\"}");
            return;
        }

        char *body = (char *)malloc(contentLength + 1);
        if (!body)
        {
            client.println("HTTP/1.1 500 Internal Server Error");
            client.println("Content-Type: application/json");
            client.println("Connection: close");
            client.println();
            client.print("{\"error\":\"out of memory\"}");
            return;
        }

        int read = 0;
        unsigned long deadline = millis() + 1000;
        while (read < contentLength && millis() < deadline)
        {
            if (client.available())
            {
                body[read++] = client.read();
            }
        }
        body[read] = '\0';

        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, body);
        free(body);

        if (err)
        {
            client.println("HTTP/1.1 400 Bad Request");
            client.println("Content-Type: application/json");
            client.println("Connection: close");
            client.println();
            client.print("{\"error\":\"invalid json\"}");
            return;
        }

        if (!doc.containsKey("brightness"))
        {
            client.println("HTTP/1.1 422 Unprocessable Entity");
            client.println("Content-Type: application/json");
            client.println("Connection: close");
            client.println();
            client.print("{\"error\":\"missing required field: brightness\"}");
            return;
        }

        int brightness = doc["brightness"];
        if (brightness < 0 || brightness > 255)
        {
            client.println("HTTP/1.1 422 Unprocessable Entity");
            client.println("Content-Type: application/json");
            client.println("Connection: close");
            client.println();
            client.print("{\"error\":\"brightness must be 0-255\"}");
            return;
        }

        displayHandler->setBrightness(brightness);

        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println("Connection: close");
        client.println();
        char response[200];
        snprintf(response, sizeof(response), "{\"ok\":true,\"message\":\"Brightness set to %d\",\"action\":\"brightness\",\"value\":%d}", brightness, brightness);
        client.print(response);
    }
};

#endif