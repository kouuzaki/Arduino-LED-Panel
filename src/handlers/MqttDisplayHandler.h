#ifndef MQTT_DISPLAY_HANDLER_H
#define MQTT_DISPLAY_HANDLER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <HUB08Panel.h>

class MqttDisplayHandler
{
private:
    PubSubClient &client;
    HUB08_Panel &panel;
    const char *device_name;
    char cmdTopic[64];

    // State cache (Menyimpan status terakhir)
    char lastText[128];
    uint8_t lastBrightness;
    int lastX, lastY;

public:
    MqttDisplayHandler(PubSubClient &mqttClient, HUB08_Panel &ledPanel, const char *name)
        : client(mqttClient), panel(ledPanel), device_name(name),
          lastBrightness(150), lastX(0), lastY(8)
    {
        memset(lastText, 0, sizeof(lastText));
        snprintf(cmdTopic, sizeof(cmdTopic), "device/%s/cmd/display", device_name);
    }

    // Dipanggil oleh MqttManager saat terkoneksi
    void subscribe()
    {
        if (client.connected())
        {
            client.subscribe(cmdTopic);
            Serial.print("Display Handler Subscribed: ");
            Serial.println(cmdTopic);
        }
    }

    // Router pesan masuk
    void handleMessage(const char *topic, byte *payload, unsigned int length)
    {
        // Pastikan topic benar
        if (strcmp(topic, cmdTopic) != 0)
            return;

        // Konversi payload ke C-String
        char json[256];
        if (length >= sizeof(json))
            length = sizeof(json) - 1;
        memcpy(json, payload, length);
        json[length] = '\0';

        // --- Parsing Action ---
        // Kita gunakan strstr untuk cek keberadaan key action
        if (strstr(json, "\"action\":\"text\""))
        {
            processText(json);
        }
        else if (strstr(json, "\"action\":\"clear\""))
        {
            processClear();
        }
        else if (strstr(json, "\"action\":\"brightness\""))
        {
            processBrightness(json);
        }
        else
        {
            Serial.println("DisplayHandler: Unknown Action");
        }
    }

private:
    // Payload: {"action":"text", "text":"HALO", "x":0, "y":8}
    void processText(char *json)
    {
        // 1. Parse Text
        char *ptr = strstr(json, "\"text\":\"");
        if (ptr)
        {
            ptr += 8;                     // Geser pointer setelah "text":"
            char *end = strchr(ptr, '"'); // Cari penutup kutip
            if (end)
            {
                int len = end - ptr;
                if (len >= (int)sizeof(lastText))
                    len = sizeof(lastText) - 1;

                // Copy text ke buffer
                memcpy(lastText, ptr, len);
                lastText[len] = '\0';

                // Handle Newline: Ubah karakter '\\n' (2 char) menjadi '\n' (1 char)
                // Kita gunakan String helper agar mudah replace-nya
                String s = String(lastText);
                s.replace("\\n", "\n");
                strcpy(lastText, s.c_str());
            }
        }

        // 2. Parse X (Optional)
        ptr = strstr(json, "\"x\":");
        if (ptr)
            lastX = atoi(ptr + 4);

        // 3. Parse Y (Optional)
        ptr = strstr(json, "\"y\":");
        if (ptr)
            lastY = atoi(ptr + 4);

        // 4. Render ke Panel
        panel.fillScreen(0);
        panel.setCursor(lastX, lastY);
        panel.drawTextMultilineCentered(lastText);
        panel.swapBuffers(true);

        Serial.print("Display Update: [");
        Serial.print(lastText);
        Serial.println("]");
    }

    // Payload: {"action":"clear"}
    void processClear()
    {
        panel.fillScreen(0);
        panel.swapBuffers(true);

        // Reset cache text
        memset(lastText, 0, sizeof(lastText));

        Serial.println("Display Cleared");
    }

    // Payload: {"action":"brightness", "value":100}
    void processBrightness(char *json)
    {
        char *ptr = strstr(json, "\"value\":");
        if (ptr)
        {
            // Ambil angka setelah "value":
            int val = atoi(ptr + 8);

            // Batasi nilai 0 - 255
            if (val < 0)
                val = 0;
            if (val > 255)
                val = 255;

            lastBrightness = (uint8_t)val;
            panel.setBrightness(lastBrightness);

            Serial.print("Brightness Set: ");
            Serial.println(val);
        }
    }
};

#endif