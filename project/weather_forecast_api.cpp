#include "weather_forecast_api.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <map>
#include <vector>

bool getWeatherForecast(DailyForecast forecast[7], String& statusMsg, float lat, float lon, int parameterType) {
  // Initialize all forecast entries as invalid
  for (int i = 0; i < 7; i++) {
    forecast[i].valid = false;
    forecast[i].minTemp = NAN;
    forecast[i].maxTemp = NAN;
    forecast[i].minWind = NAN;
    forecast[i].maxWind = NAN;
    forecast[i].totalRain = NAN;
    forecast[i].minHumidity = NAN;
    forecast[i].maxHumidity = NAN;
    forecast[i].minPressure = NAN;
    forecast[i].maxPressure = NAN;
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

    // Use maps to store all weather parameters for each day
  // Key: date string (YYYY-MM-DD), Value: vectors of measurements
  std::map<String, std::vector<float>> dailyTemps;
  std::map<String, std::vector<float>> dailyWinds;
  std::map<String, std::vector<float>> dailyRain;
  std::map<String, std::vector<float>> dailyHumidity;
  std::map<String, std::vector<float>> dailyPressure;
  
  // Process each time entry in the forecast
  for (JsonObject entry : timeSeries) {
    // Get the time (ISO 8601 format: "2025-12-01T18:00:00Z")
    const char* timeStr = entry["time"];
    if (!timeStr) continue;
    
    // Extract just the date part (first 10 characters: YYYY-MM-DD)
    String dateStr = String(timeStr).substring(0, 10);
    
        // Get the data object which contains all parameters
    JsonObject data = entry["data"].as<JsonObject>();
    
    // Collect all available parameters
    if (data.containsKey("air_temperature")) {
      float temp = data["air_temperature"].as<float>();
      dailyTemps[dateStr].push_back(temp);
    }
    
    if (data.containsKey("wind_speed")) {
      float wind = data["wind_speed"].as<float>();
      dailyWinds[dateStr].push_back(wind);
    }
    
    if (data.containsKey("precipitation_amount_mean")) {
      float rain = data["precipitation_amount_mean"].as<float>();
      dailyRain[dateStr].push_back(rain);
    }
    
    if (data.containsKey("relative_humidity")) {
      float humidity = data["relative_humidity"].as<float>();
      dailyHumidity[dateStr].push_back(humidity);
    }
    
    if (data.containsKey("air_pressure_at_mean_sea_level")) {
      float pressure = data["air_pressure_at_mean_sea_level"].as<float>();
      dailyPressure[dateStr].push_back(pressure);
    }
  }

    // Convert the maps to our forecast array (take first 7 days)
  int dayIndex = 0;
  for (auto& pair : dailyTemps) {
    if (dayIndex >= 7) break;
    
    const String& date = pair.first;

        forecast[dayIndex].date = date;
    forecast[dayIndex].valid = false;
    
    // Process temperature
    const std::vector<float>& temps = pair.second;
    if (temps.size() > 0) {      
        float minTemp = temps[0];
        float maxTemp = temps[0];
              for (float temp : temps) {
        if (temp < minTemp) minTemp = temp;
        if (temp > maxTemp) maxTemp = temp;
      }
      forecast[dayIndex].minTemp = minTemp;
      forecast[dayIndex].maxTemp = maxTemp;
      forecast[dayIndex].valid = true;
      }
    
    // Process wind speed
    if (dailyWinds.count(date) > 0) {
      const std::vector<float>& winds = dailyWinds[date];
      if (winds.size() > 0) {
        float minWind = winds[0];
        float maxWind = winds[0];
        for (float wind : winds) {
          if (wind < minWind) minWind = wind;
          if (wind > maxWind) maxWind = wind;
        }
        forecast[dayIndex].minWind = minWind;
        forecast[dayIndex].maxWind = maxWind;
      }
    }
    
    // Process precipitation (sum for the day)
    if (dailyRain.count(date) > 0) {
      const std::vector<float>& rains = dailyRain[date];
      float totalRain = 0;
      for (float rain : rains) {
        totalRain += rain;
      }
      forecast[dayIndex].totalRain = totalRain;
    }
    
    // Process humidity
    if (dailyHumidity.count(date) > 0) {
      const std::vector<float>& humidities = dailyHumidity[date];
      if (humidities.size() > 0) {
        float minHum = humidities[0];
        float maxHum = humidities[0];
        for (float hum : humidities) {
          if (hum < minHum) minHum = hum;
          if (hum > maxHum) maxHum = hum;
        }
        forecast[dayIndex].minHumidity = minHum;
        forecast[dayIndex].maxHumidity = maxHum;
      }
    }
    
    // Process pressure
    if (dailyPressure.count(date) > 0) {
      const std::vector<float>& pressures = dailyPressure[date];
      if (pressures.size() > 0) {
        float minPres = pressures[0];
        float maxPres = pressures[0];
        for (float pres : pressures) {
          if (pres < minPres) minPres = pres;
          if (pres > maxPres) maxPres = pres;
        }
        forecast[dayIndex].minPressure = minPres;
        forecast[dayIndex].maxPressure = maxPres;
      }
    }
    
    dayIndex++;
  }

  if (dayIndex == 0) {
    statusMsg = "No valid forecast";
    return false;
  }

  statusMsg = "OK";
  return true;
}
