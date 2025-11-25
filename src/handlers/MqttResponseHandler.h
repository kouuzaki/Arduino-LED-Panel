#ifndef MQTT_RESPONSE_HANDLER_H
#define MQTT_RESPONSE_HANDLER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

/**
 * @class MqttResponseHandler
 * @brief Standardized MQTT response publisher for commands
 *
 * Publishes command responses to {device_id}/response topic
 * with consistent JSON format: { action, status, message, data (optional) }
 */
class MqttResponseHandler
{
private:
    PubSubClient &mqttClient;
    char deviceId[32];
    char responseTopic[64];

public:
    /**
     * @brief Constructor
     * @param client Reference to PubSubClient instance
     * @param id Device ID (used to construct response topic)
     */
    MqttResponseHandler(PubSubClient &client, const char *id)
        : mqttClient(client)
    {
        strlcpy(deviceId, id, sizeof(deviceId));
        snprintf(responseTopic, sizeof(responseTopic), "%s/response", deviceId);
    }

    /**
     * @brief Set device ID (used if changed dynamically)
     * @param id New device ID
     */
    void setDeviceId(const char *id)
    {
        strlcpy(deviceId, id, sizeof(deviceId));
        snprintf(responseTopic, sizeof(responseTopic), "%s/response", deviceId);
    }

    /**
     * @brief Publish a success response
     * @param action Command action name
     * @param message Human-readable status message
     * @param data Optional JSON data to include in response
     */
    void publishSuccess(const char *action, const char *message, const char *data = nullptr)
    {
        publishResponse(action, "success", message, data);
    }

    /**
     * @brief Publish an error response
     * @param action Command action name
     * @param message Human-readable error message
     * @param data Optional JSON data to include in response
     */
    void publishError(const char *action, const char *message, const char *data = nullptr)
    {
        publishResponse(action, "error", message, data);
    }

    /**
     * @brief Publish a generic response with custom status
     * @param action Command action name
     * @param status Status string (e.g., "success", "error", "pending")
     * @param message Human-readable message
     * @param data Optional JSON data to include in response
     */
    void publishResponse(const char *action, const char *status, const char *message, const char *data = nullptr)
    {
        if (!mqttClient.connected())
        {
            Serial.println("MQTT Response: Not connected, cannot publish");
            return;
        }

        // Build response JSON
        JsonDocument response;
        response["action"] = action;
        response["status"] = status;
        response["message"] = message;
        if (data)
        {
            response["data"] = data;
        }
        response["timestamp"] = millis();

        // Serialize and publish
        char buffer[512];
        size_t len = serializeJson(response, buffer, sizeof(buffer));

        bool ok = mqttClient.publish(responseTopic, (uint8_t *)buffer, len, 1); // QoS 1

        // Log
        Serial.print("MQTT Response: [");
        Serial.print(action);
        Serial.print("] ");
        Serial.print(status);
        Serial.print(" (");
        Serial.print(len);
        Serial.print(" bytes) -> ");
        Serial.println(ok ? "OK" : "FAILED");
    }
};

#endif
