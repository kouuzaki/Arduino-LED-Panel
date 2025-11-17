#ifndef JSON_CONFIG_H
#define JSON_CONFIG_H

#include <ArduinoJson.h>

// Optimized JSON document sizes untuk Arduino UNO
// Gunakan StaticJsonDocument instead of JsonDocument

// Size definitions (dalam bytes)
#define JSON_BUFFER_SMALL 128      // Untuk simple status responses
#define JSON_BUFFER_MEDIUM 256     // Untuk device info
#define JSON_BUFFER_LARGE 512      // Untuk complete system info
#define JSON_BUFFER_REQUEST 256    // Untuk parsing requests

// Typedef untuk mudah digunakan
using JsonSmall = StaticJsonDocument<JSON_BUFFER_SMALL>;
using JsonMedium = StaticJsonDocument<JSON_BUFFER_MEDIUM>;
using JsonLarge = StaticJsonDocument<JSON_BUFFER_LARGE>;
using JsonRequest = StaticJsonDocument<JSON_BUFFER_REQUEST>;

#endif
