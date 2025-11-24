#ifndef MQTT_DISPLAY_HANDLER_H
#define MQTT_DISPLAY_HANDLER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <HUB08Panel.h>
#include <ArduinoJson.h>

class MqttDisplayHandler
{
private:
    PubSubClient &client;
    HUB08_Panel &panel;
    const char *device_name;
    char cmdTopic[64];

    // State cache
    char lastText[128];
    uint8_t lastBrightness;
    int lastX, lastY;

public:
    MqttDisplayHandler(PubSubClient &mqttClient, HUB08_Panel &ledPanel, const char *name)
        : client(mqttClient), panel(ledPanel), device_name(name),
          lastBrightness(128), lastX(0), lastY(8)
    {
        memset(lastText, 0, sizeof(lastText));
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

        // 2. Buat Filter (Kunci Hemat Memori)
        // Kita hanya mengambil field yang diperlukan, sisanya dibuang
        JsonDocument filter;
        filter["action"] = true;
        filter["text"] = true;
        filter["value"] = true;

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
        else if (strcmp(action, "brightness") == 0)
        {
            processBrightness(doc);
        }
    }

private:
    // Payload: {"action":"text", "text":"HALO"}
    void processText(JsonDocument &doc)
    {
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
    }

    // Payload: {"action":"brightness", "value":100}
    void processBrightness(JsonDocument &doc)
    {
        if (doc.containsKey("value"))
        {
            int val = doc["value"];

            // Constrain value 0-255
            if (val < 0)
                val = 0;
            if (val > 255)
                val = 255;

            lastBrightness = (uint8_t)val;
            panel.setBrightness(lastBrightness);

            Serial.print(F("Brightness: "));
            Serial.println(val);
        }
    }
};

#endif