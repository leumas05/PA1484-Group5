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
    forecast[i].symbolCode = -1;
    forecast[i].date = "";
  }

  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    statusMsg = "WiFi not connected";
    return false;
  }

  // Build the URL with the provided coordinates
  String forecastUrl = "https://opendata-download-metfcst.smhi.se/api/category/snow1g/version/1/geotype/point/lon/";
  forecastUrl += String(lon, 6);  // Use 6 decimal places for better precision
  forecastUrl += "/lat/";
  forecastUrl += String(lat, 6);  // Use 6 decimal places for better precision
  forecastUrl += "/data.json";

  HTTPClient http;
  http.begin(forecastUrl);
  http.setTimeout(15000);  // 15 second timeout for larger responses
  http.setReuse(false);    // Don't reuse connection to prevent issues
  
  Serial.print("Fetching forecast from: ");
  Serial.println(forecastUrl);
  
  int httpCode = http.GET();
  
  if (httpCode != HTTP_CODE_OK) {
    if (httpCode == -1) {
      statusMsg = "Connection failed";
    } else {
      statusMsg = "HTTP error: " + String(httpCode);
    }
    Serial.print("HTTP error code: ");
    Serial.println(httpCode);
    http.end();
    return false;
  }

  int payloadSize = http.getSize();
  Serial.print("Payload size: ");
  Serial.print(payloadSize);
  Serial.println(" bytes");

  Serial.print("Free heap before download: ");
  Serial.println(ESP.getFreeHeap());

  WiFiClient* stream = http.getStreamPtr();
  stream->setTimeout(15000);

  String payload;
  const size_t expectedSize = (payloadSize > 0) ? payloadSize : 70000;
  payload.reserve(expectedSize + 16);

  unsigned long waitStart = millis();

  if (payloadSize > 0) {
    size_t remaining = payloadSize;
    while (remaining > 0) {
      if (!stream->available()) {
        if ((millis() - waitStart) > 15000) {
          Serial.println("Stream read timeout (known length)");
          break;
        }
        delay(1);
        continue;
      }
      char buffer[512];
      size_t toRead = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
      size_t readBytes = stream->readBytes(buffer, toRead);
      if (readBytes == 0) continue;
      payload.concat(buffer, readBytes);
      remaining -= readBytes;
      waitStart = millis();
    }
  } else {
    // Handle chunked transfer encoding
    while (http.connected()) {
      String line = stream->readStringUntil('\n');
      if (line.length() == 0) {
        if ((millis() - waitStart) > 15000) {
          Serial.println("Chunk header timeout");
          break;
        }
        delay(1);
        continue;
      }

      line.trim();
      if (line.length() == 0) continue;

      long chunkSize = strtol(line.c_str(), nullptr, 16);
      if (chunkSize <= 0) {
        break;
      }

      long remaining = chunkSize;
      while (remaining > 0) {
        if (!stream->available()) {
          if ((millis() - waitStart) > 15000) {
            Serial.println("Chunk read timeout");
            break;
          }
          delay(1);
          continue;
        }
        char buffer[512];
        size_t toRead = remaining < (long)sizeof(buffer) ? remaining : sizeof(buffer);
        size_t readBytes = stream->readBytes(buffer, toRead);
        if (readBytes == 0) continue;
        payload.concat(buffer, readBytes);
        remaining -= readBytes;
        waitStart = millis();
      }

      // Consume trailing CRLF after each chunk
      stream->read();
      stream->read();

      if ((millis() - waitStart) > 15000) {
        Serial.println("Overall chunk timeout");
        break;
      }
    }
  }

  http.end();

  Serial.print("Received payload length: ");
  Serial.println(payload.length());
  Serial.print("Free heap before parse: ");
  Serial.println(ESP.getFreeHeap());

  if (payload.length() == 0) {
    statusMsg = "Empty response";
    Serial.println("Received empty payload");
    return false;
  }

  if (!payload.startsWith("{")) {
    statusMsg = "Invalid JSON format";
    Serial.println("Payload doesn't start with '{'");
    Serial.println("First 120 chars: " + payload.substring(0, 120));
    return false;
  }

  DynamicJsonDocument doc(102400);
  DeserializationError error = deserializeJson(doc, payload);

  payload = String();

  Serial.print("Free heap after parse: ");
  Serial.println(ESP.getFreeHeap());

  if (error) {
    statusMsg = "Parse error";
    Serial.print("JSON parse error: ");
    Serial.println(error.c_str());
    Serial.print("Error code: ");
    Serial.println(error.code());
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
  std::map<String, int> dailySymbol;  // Store one symbol code per day
  
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

    // Store symbol code (use the one from around noon/12:00 as representative)
    if (data.containsKey("symbol_code")) {
      String timeStr_full = String(timeStr);
      // Extract hour from time string (format: "2025-12-03T12:00:00Z")
      int hour = timeStr_full.substring(11, 13).toInt();
      
      // Store symbol code if we don't have one yet, or if this is closer to noon
      if (!dailySymbol.count(dateStr) || (hour >= 11 && hour <= 13)) {
        int symbol = data["symbol_code"].as<int>();
        dailySymbol[dateStr] = symbol;
      }
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
    
    // Store symbol code for the day
    if (dailySymbol.count(date) > 0) {
      forecast[dayIndex].symbolCode = dailySymbol[date];
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