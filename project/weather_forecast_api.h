#ifndef WEATHER_FORECAST_API_H
#define WEATHER_FORECAST_API_H

#include <Arduino.h>

// Structure to hold daily temperature data
struct DailyForecast {
  String date;        // Date string (e.g., "2025-12-01")
  float minTemp;      // Minimum temperature in Celsius
  float maxTemp;      // Maximum temperature in Celsius
  bool valid;         // Whether this forecast entry is valid
};

// Function to fetch 7-day temperature forecast from SMHI API
// Returns true if successful, false otherwise
// forecast array should have at least 7 elements
// statusMsg will contain a user-friendly status message
// lat and lon specify the location coordinates
bool getWeatherForecastTemp(DailyForecast forecast[7], String& statusMsg, float lat, float lon);

#endif
