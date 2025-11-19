#include "weather_api.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Function to fetch current temperature from SMHI API
// Returns temperature in Celsius or NAN if request fails
// statusMsg will contain a user-friendly status message
float getCurrentTemperature(String& statusMsg) {
  HTTPClient http;
  float temperature = NAN; // Return NAN if request fails
  
  // Begin HTTP connection to SMHI API
  http.begin("https://opendata-download-metobs.smhi.se/api/version/1.0/parameter/1/station/65090/period/latest-hour/data.json");
  
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    
    // Parse JSON response
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      // Navigate JSON structure to get the latest temperature value
      // The API returns: {"value": [...], "station": {...}, "parameter": {...}, "period": {...}}
      // We need the last value from the "value" array
      JsonArray values = doc["value"].as<JsonArray>();
      
      if (values.size() > 0) {
        // Get the last (most recent) measurement
        JsonObject latestValue = values[values.size() - 1];
        temperature = latestValue["value"].as<float>();
        statusMsg = "OK";
      } else {
        statusMsg = "No data";
      }
    } else {
      statusMsg = "Parse error";
    }
  } else if (httpCode == -1) {
    statusMsg = "Connection failed";
  } else {
    statusMsg = "HTTP error: " + String(httpCode);
  }
  
  http.end();
  return temperature;
}

float getCurrentWind(String& statusMsg, float& windDirection) {
  HTTPClient http;
  float windSpeed = NAN; // Return NAN if request fails
  windDirection = NAN; // Initialize wind direction

  // First, fetch wind speed (parameter 4)
  http.begin("https://opendata-download-metobs.smhi.se/api/version/1.0/parameter/4/station/65090/period/latest-hour/data.json");
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();

    // Parse JSON response
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      // Navigate JSON structure to get the latest wind value
      JsonArray values = doc["value"].as<JsonArray>();

      if (values.size() > 0) {
        // Get the last (most recent) measurement
        JsonObject latestValue = values[values.size() - 1];
        windSpeed = latestValue["value"].as<float>();
        statusMsg = "OK";
      } else {
        statusMsg = "No data";
      }
    } else {
      statusMsg = "Parse error";
    }
  } else if (httpCode == -1) {
    statusMsg = "Connection failed";
  } else {
    statusMsg = "HTTP error: " + String(httpCode);
  }

  http.end();
  
  // Now fetch wind direction (parameter 3)
  if (!isnan(windSpeed)) {
    // Reuse the same http object for the second request
    http.begin("https://opendata-download-metobs.smhi.se/api/version/1.0/parameter/3/station/65090/period/latest-hour/data.json");
    int httpCode2 = http.GET();
    
    if (httpCode2 == HTTP_CODE_OK) {
      String payload = http.getString();
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload);
      
      if (!error) {
        JsonArray values = doc["value"].as<JsonArray>();
        if (values.size() > 0) {
          JsonObject latestValue = values[values.size() - 1];
          windDirection = latestValue["value"].as<float>();
        }
      }
    }
    http.end();
  }
  
  return windSpeed;
}
