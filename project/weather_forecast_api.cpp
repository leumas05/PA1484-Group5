#include "weather_forecast_api.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <map>
#include <vector>

bool getWeatherForecastTemp(DailyForecast forecast[7], String& statusMsg, float lat, float lon) {
  // Initialize all forecast entries as invalid
  for (int i = 0; i < 7; i++) {
    forecast[i].valid = false;
    forecast[i].minTemp = NAN;
    forecast[i].maxTemp = NAN;
    forecast[i].date = "";
  }

  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    statusMsg = "WiFi not connected";
    return false;
  }

  // Build the URL with the provided coordinates
  String forecastUrl = "https://opendata-download-metfcst.smhi.se/api/category/snow1g/version/1/geotype/point/lon/";
  forecastUrl += String(lon, 2);  // 2 decimal places
  forecastUrl += "/lat/";
  forecastUrl += String(lat, 2);  // 2 decimal places
  forecastUrl += "/data.json";

  HTTPClient http;
  http.begin(forecastUrl);
  
  int httpCode = http.GET();
  
  if (httpCode != HTTP_CODE_OK) {
    if (httpCode == -1) {
      statusMsg = "Connection failed";
    } else {
      statusMsg = "HTTP error: " + String(httpCode);
    }
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();
  
  // Parse JSON response
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);
  
  if (error) {
    statusMsg = "Parse error";
    return false;
  }

  // Navigate to the timeSeries array
  JsonArray timeSeries = doc["timeSeries"].as<JsonArray>();
  
  if (timeSeries.size() == 0) {
    statusMsg = "No forecast data";
    return false;
  }

  // Use a map to store min/max temps for each day
  // Key: date string (YYYY-MM-DD), Value: pair of vectors (temps for that day)
  std::map<String, std::vector<float>> dailyTemps;
  
  // Process each time entry in the forecast
  for (JsonObject entry : timeSeries) {
    // Get the time (ISO 8601 format: "2025-12-01T18:00:00Z")
    const char* timeStr = entry["time"];
    if (!timeStr) continue;
    
    // Extract just the date part (first 10 characters: YYYY-MM-DD)
    String dateStr = String(timeStr).substring(0, 10);
    
    // Get the data object which contains air_temperature
    JsonObject data = entry["data"].as<JsonObject>();
    if (data.containsKey("air_temperature")) {
      float temp = data["air_temperature"].as<float>();
      dailyTemps[dateStr].push_back(temp);
    }
  }

  // Convert the map to our forecast array (take first 7 days)
  int dayIndex = 0;
  for (auto& pair : dailyTemps) {
    if (dayIndex >= 7) break;
    
    const String& date = pair.first;
    const std::vector<float>& temps = pair.second;
    
    if (temps.size() > 0) {
      // Calculate min and max temperature for this day
      float minTemp = temps[0];
      float maxTemp = temps[0];
      
      for (float temp : temps) {
        if (temp < minTemp) minTemp = temp;
        if (temp > maxTemp) maxTemp = temp;
      }
      
      forecast[dayIndex].date = date;
      forecast[dayIndex].minTemp = minTemp;
      forecast[dayIndex].maxTemp = maxTemp;
      forecast[dayIndex].valid = true;
      dayIndex++;
    }
  }

  if (dayIndex == 0) {
    statusMsg = "No valid forecast";
    return false;
  }

  statusMsg = "OK";
  return true;
}
