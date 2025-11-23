#include "weather_api.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <weather_forecast.h>

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

// Function to fetch current wind direction from SMHI API
// Returns wind direction in degrees (0-360) or NAN if request fails
// statusMsg will contain a user-friendly status message
float getCurrentWindDirection(String& statusMsg) {
  HTTPClient http;
  float windDirection = NAN; // Return NAN if request fails

  http.begin("https://opendata-download-metobs.smhi.se/api/version/1.0/parameter/3/station/65090/period/latest-hour/data.json");
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();

    // Parse JSON response
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      JsonArray values = doc["value"].as<JsonArray>();
      if (values.size() > 0) {
        JsonObject latestValue = values[values.size() - 1];
        windDirection = latestValue["value"].as<float>();
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
  return windDirection;
}

float getCurrentWindSpeed(String& statusMsg) {
  HTTPClient http;
  float windSpeed = NAN; // Return NAN if request fails

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
  
  return windSpeed;
}

float getCurrentRain(String& statusMsg) {
  HTTPClient http;
  float rain = NAN; // Return NAN if request fails

  // First, fetch wind speed (parameter 4)
  http.begin("https://opendata-download-metobs.smhi.se/api/version/1.0/parameter/7/station/65090/period/latest-hour/data.json");
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
        rain = latestValue["value"].as<float>();
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
  
  return rain;
}

float getCurrentHumidity(String& statusMsg) {
  HTTPClient http;
  float humidity = NAN; // Return NAN if request fails

  // First, fetch wind speed (parameter 4)
  http.begin("https://opendata-download-metobs.smhi.se/api/version/1.0/parameter/6/station/65090/period/latest-hour/data.json");
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
        humidity = latestValue["value"].as<float>();
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
  
  return humidity;
}

float getCurrentPressure(String& statusMsg) {
  HTTPClient http;
  float pressure = NAN; // Return NAN if request fails

  // First, fetch wind speed (parameter 4)
  http.begin("https://opendata-download-metobs.smhi.se/api/version/1.0/parameter/9/station/65090/period/latest-hour/data.json");
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
        pressure = latestValue["value"].as<float>();
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
  
  return pressure;
}
