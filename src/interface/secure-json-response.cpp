#include "./secure-json-response.h"

// Create plain JSON response (AES encryption removed)
String createSecureJsonResponse(const String &message, const JsonDocument& data) {
    JsonDocument jsonDoc;
    jsonDoc["message"] = message;
    jsonDoc["data"] = data;
    jsonDoc["timestamp"] = millis();
    jsonDoc["encrypted"] = false;

    // Send plain response
    String jsonString;
    serializeJson(jsonDoc, jsonString);
    return jsonString;
}

bool decryptRequestData(const String& data, JsonDocument& jsonDoc) {
    // No decryption - just parse JSON directly
    DeserializationError error = deserializeJson(jsonDoc, data);
    return error == DeserializationError::Ok;
}

bool isSecureRequest() {
    // AES encryption disabled - always return false
    return false;
}
