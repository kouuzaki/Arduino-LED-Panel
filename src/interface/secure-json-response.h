#ifndef SECURE_API_RESPONSE_H
#define SECURE_API_RESPONSE_H

#include <ArduinoJson.h>

// Secure response function that returns plain JSON data
String createSecureJsonResponse(const String &message, const JsonDocument &data);

// Function to parse incoming request data
bool decryptRequestData(const String &data, JsonDocument &jsonDoc);

// Function to check if request wants encrypted response (deprecated - always returns false)
bool isSecureRequest();

#endif
