#ifndef MQTT_TEXT_HANDLER_H
#define MQTT_TEXT_HANDLER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <HUB08Panel.h>

// ============================================
// MQTT TEXT RENDERING HANDLER
// ============================================
// Struktur MQTT Topics:
//   device/iot_led_panel/command/text      -> {"text":"Hello","x":0,"y":16,"color":1}
//   device/iot_led_panel/command/clear     -> any payload to clear
//   device/iot_led_panel/command/brightness -> {"brightness":200}
//   device/iot_led_panel/status/display    -> current display text
//
// ============================================

class MqttTextHandler
{
private:
    PubSubClient &client;
    HUB08_Panel &panel;
    const char *device_name;

    char textTopic[64];
    char clearTopic[64];
    char brightnessTopic[64];
    char statusTopic[64];

    char lastText[128];
    int lastX;
    int lastY;
    uint8_t lastBrightness;

public:
    MqttTextHandler(PubSubClient &mqttClient, HUB08_Panel &ledPanel, const char *name)
        : client(mqttClient), panel(ledPanel), device_name(name),
          lastX(0), lastY(16), lastBrightness(200)
    {
        memset(lastText, 0, sizeof(lastText));

        // Build topic names
        snprintf(textTopic, sizeof(textTopic), "device/%s/command/text", device_name);
        snprintf(clearTopic, sizeof(clearTopic), "device/%s/command/clear", device_name);
        snprintf(brightnessTopic, sizeof(brightnessTopic), "device/%s/command/brightness", device_name);
        snprintf(statusTopic, sizeof(statusTopic), "device/%s/status/display", device_name);
    }

    // Subscribe to all MQTT text rendering topics
    void subscribe()
    {
        client.subscribe(textTopic);
        client.subscribe(clearTopic);
        client.subscribe(brightnessTopic);
    }

    // Handle incoming MQTT message (call from PubSubClient callback)
    void handleMessage(const char *topic, byte *payload, unsigned int length)
    {
        if (strcmp(topic, textTopic) == 0)
        {
            handleTextCommand(payload, length);
        }
        else if (strcmp(topic, clearTopic) == 0)
        {
            handleClearCommand();
        }
        else if (strcmp(topic, brightnessTopic) == 0)
        {
            handleBrightnessCommand(payload, length);
        }
    }

    // Publish current display status
    void publishStatus()
    {
        char statusBuffer[256];
        snprintf(statusBuffer, sizeof(statusBuffer),
                 "{\"text\":\"%s\",\"x\":%d,\"y\":%d,\"brightness\":%d}",
                 lastText, lastX, lastY, lastBrightness);

        client.publish(statusTopic, statusBuffer);
    }

private:
    // Parse text command: {"text":"Hello","x":0,"y":16,"color":1}
    void handleTextCommand(byte *payload, unsigned int length)
    {
        // Create null-terminated payload
        char buffer[256];
        if (length >= sizeof(buffer) - 1)
            length = sizeof(buffer) - 1;

        memcpy(buffer, payload, length);
        buffer[length] = '\0';

        // Simple JSON parsing (no external lib)
        // Extract text field
        const char *textStart = strstr(buffer, "\"text\":\"");
        if (!textStart)
            return;

        textStart += 8; // Skip '"text":"'
        const char *textEnd = strchr(textStart, '"');
        if (!textEnd)
            return;

        int textLen = textEnd - textStart;
        if (textLen >= sizeof(lastText) - 1)
            textLen = sizeof(lastText) - 1;

        memcpy(lastText, textStart, textLen);
        lastText[textLen] = '\0';

        // Extract x coordinate
        const char *xStart = strstr(buffer, "\"x\":");
        if (xStart)
        {
            lastX = atoi(xStart + 4);
        }

        // Extract y coordinate
        const char *yStart = strstr(buffer, "\"y\":");
        if (yStart)
        {
            lastY = atoi(yStart + 4);
        }

        // Render text on panel
        panel.fillScreen(0);   // Clear to black
        panel.setTextColor(1); // White/On
        panel.setCursor(lastX, lastY);
        panel.drawTextMultilineCentered(lastText);
        panel.swapBuffers(true); // Display with buffer copy

        // Publish status
        publishStatus();

        Serial.print("MQTT: Text rendered: ");
        Serial.println(lastText);
    }

    // Handle clear command
    void handleClearCommand()
    {
        panel.fillScreen(0);
        panel.swapBuffers(true); // Display the clear
        memset(lastText, 0, sizeof(lastText));

        publishStatus();
        Serial.println("MQTT: Display cleared");
    }

    // Parse brightness command: {"brightness":200}
    void handleBrightnessCommand(byte *payload, unsigned int length)
    {
        char buffer[64];
        if (length >= sizeof(buffer) - 1)
            length = sizeof(buffer) - 1;

        memcpy(buffer, payload, length);
        buffer[length] = '\0';

        // Extract brightness value
        const char *brightnessStart = strstr(buffer, "\"brightness\":");
        if (!brightnessStart)
            return;

        uint8_t brightness = atoi(brightnessStart + 13); // Skip '"brightness":'

        if (brightness > 255)
            brightness = 255;

        panel.setBrightness(brightness);
        lastBrightness = brightness;

        publishStatus();

        Serial.print("MQTT: Brightness set to ");
        Serial.println(brightness);
    }
};

#endif
