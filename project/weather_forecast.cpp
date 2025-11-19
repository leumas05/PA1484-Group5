#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "weather_forecast.h"

// Karlskrona coordinates
const City cities[] = {
    {"Karlskrona", 56.1612, 15.5869, "KAR"}
};
const int CITY_COUNT = sizeof(cities) / sizeof(cities[0]);

bool getSevenDayForecast(int cityIndex, ForecastDay out[7], String& statusMsg) {
    if (cityIndex < 0 || cityIndex >= CITY_COUNT) {
        statusMsg = "Invalid city index";
        return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        statusMsg = "WiFi not connected";
        return false;
    }

    const City& city = cities[cityIndex];
    String url = String("https://api.met.no/weatherapi/locationforecast/2.0/compact?lat=")
                 + city.lat + "&lon=" + city.lon;

    HTTPClient http;
    http.begin(url);
    http.addHeader("User-Agent", "KarlskronaWeatherApp/1.0"); // Mandatory for met.no

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        statusMsg = "HTTP error " + String(httpCode);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    // Debug: uncomment to see raw JSON
    // Serial.println("Raw JSON:");
    // Serial.println(payload);

    // Allocate sufficient buffer for 7-day forecast
    DynamicJsonDocument doc(12 * 1024);
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        statusMsg = error.c_str();
        return false;
    }

    // Get timeseries array
    JsonArray timeseries = doc["properties"]["timeseries"].as<JsonArray>();
    if (timeseries.isNull()) {
        statusMsg = "No timeseries data";
        return false;
    }

    int filledDays = 0;
    String lastDate = "";

    for (JsonObject entry : timeseries) {
        const char* timeStr = entry["time"]; // e.g., "2025-11-19T12:00:00Z"
        String dateStr = String(timeStr).substring(0, 10); // "YYYY-MM-DD"

        // Pick only one entry per day at 12:00
        if (dateStr != lastDate && String(timeStr).endsWith("T12:00:00Z")) {
            lastDate = dateStr;

            out[filledDays].temperature = entry["data"]["instant"]["details"]["air_temperature"] | NAN;

            // Pick Wsymb2 symbol (if available)
            JsonArray nextHours = entry["data"]["next_12_hours"]["summary"];
            if (!nextHours.isNull()) {
                // For simplicity, take first symbol
                out[filledDays].symbol = entry["data"]["next_12_hours"]["summary"]["symbol_code"].as<String>() == "clearsky" ? 1 : 4;
            } else {
                out[filledDays].symbol = 4; // Default cloud
            }

            strncpy(out[filledDays].date, dateStr.c_str(), sizeof(out[filledDays].date));
            out[filledDays].date[sizeof(out[filledDays].date)-1] = '\0';

            filledDays++;
            if (filledDays >= 7) break;
        }
    }

    if (filledDays < 7) {
        statusMsg = "Not enough forecast data";
        return false;
    }

    return true;
}