#ifndef MQTT_DISPLAY_HANDLER_H
#define MQTT_DISPLAY_HANDLER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <HUB08Panel.h>
#include <ArduinoJson.h>
#include <avr/wdt.h>
#include "MqttResponseHandler.h"

class MqttDisplayHandler
{
private:
    PubSubClient &client;
    HUB08_Panel &panel;
    const char *device_name;
    char cmdTopic[64];
    MqttResponseHandler *responseHandler;

    // State cache
    char lastText[128];
    uint8_t lastBrightness;
    int lastX, lastY;

public:
    MqttDisplayHandler(PubSubClient &mqttClient, HUB08_Panel &ledPanel, const char *name)
        : client(mqttClient), panel(ledPanel), device_name(name),
          responseHandler(nullptr), lastBrightness(128), lastX(0), lastY(8)
    {
        memset(lastText, 0, sizeof(lastText));
        snprintf(cmdTopic, sizeof(cmdTopic), "device/%s/cmd/display", device_name);
    }

    /**
     * @brief Set the response handler for publishing command responses
     * @param handler Pointer to initialized MqttResponseHandler
     */
    void setResponseHandler(MqttResponseHandler *handler)
    {
        responseHandler = handler;
    }

    void setDeviceId(const char *newId)
    {
        device_name = newId;
        snprintf(cmdTopic, sizeof(cmdTopic), "device/%s/cmd/display", device_name);
    }

    void subscribe()
    {
        if (client.connected())
        {
            client.subscribe(cmdTopic);
            Serial.print("Display Handler Subscribed: ");
            Serial.println(cmdTopic);
        }
    }

    void handleMessage(const char *topic, byte *payload, unsigned int length)
    {
        if (strcmp(topic, cmdTopic) != 0)
            return;

        // 1. Siapkan Buffer (Mutable) untuk Zero-Copy Mode
        // ArduinoJson v7 bisa memodifikasi buffer ini langsung tanpa alokasi RAM tambahan
        char jsonBuffer[256];
        if (length >= sizeof(jsonBuffer))
            length = sizeof(jsonBuffer) - 1;
        memcpy(jsonBuffer, payload, length);
        jsonBuffer[length] = '\0';

        // 4. Ambil Filter
        JsonDocument filter;
        filter["action"] = true;
        filter["text"] = true;
        filter["brightness"] = true;

        // 3. Deserialize JSON
        // JsonDocument di v7 otomatis mengatur memori secara efisien
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, jsonBuffer, DeserializationOption::Filter(filter));

        if (error)
        {
            Serial.print(F("JSON Error: "));
            Serial.println(error.c_str());
            return;
        }

        // 4. Ambil Action
        const char *action = doc["action"];
        if (!action)
            return; // Tidak ada action, abaikan

        // 5. Routing Logic
        if (strcmp(action, "text") == 0)
        {
            processText(doc);
        }
        else if (strcmp(action, "clear") == 0)
        {
            processClear();
        }
        else if (strcmp(action, "restart") == 0)
        {
            processRestart();
        }
    }

private:
    // Payload: {"action":"text", "text":"HALO\nWORLD", "brightness":200}
    // brightness is optional, defaults to 255 if not specified
    void processText(JsonDocument &doc)
    {
        // Handle brightness if provided
        if (doc.containsKey("brightness"))
        {
            int val = doc["brightness"];
            if (val < 0)
                val = 0;
            if (val > 255)
                val = 255;

            lastBrightness = (uint8_t)val;
            panel.setBrightness(lastBrightness);

            Serial.print(F("Brightness set to: "));
            Serial.println(val);
        }

        // Ambil Text (default empty string jika null)
        const char *newText = doc["text"] | "";

        // Simpan ke cache
        strlcpy(lastText, newText, sizeof(lastText));
        // Handle Newline Manual (Opsional)
        // ArduinoJson otomatis handle \n standar, tapi jika user kirim raw string "\\n":
        String s = String(lastText);
        s.replace("\\n", "\n");

        // Render
        panel.fillScreen(0);
        panel.setCursor(0, 8);
        panel.drawTextMultilineCentered(s);
        panel.swapBuffers(true);

        Serial.print(F("Display Text: "));
        Serial.println(s);
    }

    // Payload: {"action":"clear"}
    void processClear()
    {
        panel.fillScreen(0);
        panel.swapBuffers(true);
        memset(lastText, 0, sizeof(lastText)); // Clear cache text
        Serial.println(F("Display Cleared"));

        // Publish response
        if (responseHandler)
            responseHandler->publishSuccess("clear", "Display cleared successfully");
    }

    // Payload: {"action":"restart"}
    // Device will publish response then restart via watchdog timer
    void processRestart()
    {
        Serial.println(F("Restart command received"));

        // Publish response BEFORE restart
        if (responseHandler)
            responseHandler->publishSuccess("restart", "Device will restart in 2 seconds");

        // Give MQTT time to publish response
        delay(500);

        // Trigger watchdog reset
        Serial.println(F("Initiating watchdog restart..."));
        delay(500);

        wdt_enable(WDTO_2S); // Enable watchdog with 2-second timeout
        // MCU will reset when watchdog expires
        while (1)
            ; // Wait for reset
    }
};

#endif