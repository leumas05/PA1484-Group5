#include "weather_api.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <weather_forecast.h>
#include <cstring>   

int station_nr;
int paraM;

int change_station_nr(const char* new_station) {
  if (new_station == nullptr) {
    return -1;
  }

  if (strcmp(new_station, "Karlskrona") == 0) {
    station_nr = 65090;
    return station_nr;
  } else if (strcmp(new_station, "Stockholm") == 0) {
    station_nr = 98230;
    return station_nr;
  } else if (strcmp(new_station, "Gothenburg") == 0) {
    station_nr = 71420;
    return station_nr;
  } else if (strcmp(new_station, "Haparanda") == 0) {
    station_nr = 163960;
    return station_nr;
  }

  return -1; // Invalid station name
}

//"https://opendata-download-metobs.smhi.se/api/version/1.0/parameter/4/station/65090/period/latest-hour/data.json"
// Build URL using Arduino `String` concatenation to avoid pointer arithmetic
// Helper: build API URL for current parameter and station
//static String build_smhi_url(int parameter, int station) {
//  return String("https://opendata-download-metobs.smhi.se/api/version/1.0/parameter/") + String(parameter) + String("/station/") + String(station) + String("/period/latest-hour/data.json");
//}


// Function to fetch current temperature from SMHI API
// Returns temperature in Celsius or NAN if request fails
// statusMsg will contain a user-friendly status message
float getCurrentTemperature(String& statusMsg) {
  HTTPClient http;
  float temperature = NAN; // Return NAN if request fails
  
  // Begin HTTP connection to SMHI API
  paraM = 1;
  String url=("https://opendata-download-metobs.smhi.se/api/version/1.0/parameter/") + String(paraM) + String("/station/") + String(station_nr) + String("/period/latest-hour/data.json");
  http.begin(url);
  
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
  paraM = 3;
  String url=("https://opendata-download-metobs.smhi.se/api/version/1.0/parameter/") + String(paraM) + String("/station/") + String(station_nr) + String("/period/latest-hour/data.json");
  http.begin(url);
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
  paraM = 4;
  String url=("https://opendata-download-metobs.smhi.se/api/version/1.0/parameter/") + String(paraM) + String("/station/") + String(station_nr) + String("/period/latest-hour/data.json");
  http.begin(url);
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

  // First, fetch wind speed (parameter 7)
  paraM = 7;
  String url=("https://opendata-download-metobs.smhi.se/api/version/1.0/parameter/") + String(paraM) + String("/station/") + String(station_nr) + String("/period/latest-hour/data.json");
  http.begin(url);
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

  // First, fetch wind speed (parameter 6)
  paraM = 6;
  String url=("https://opendata-download-metobs.smhi.se/api/version/1.0/parameter/") + String(paraM) + String("/station/") + String(station_nr) + String("/period/latest-hour/data.json");
  http.begin(url);
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

  // First, fetch wind speed (parameter 9)
  paraM = 9;
  String url=("https://opendata-download-metobs.smhi.se/api/version/1.0/parameter/") + String(paraM) + String("/station/") + String(station_nr) + String("/period/latest-hour/data.json");
  http.begin(url);
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
