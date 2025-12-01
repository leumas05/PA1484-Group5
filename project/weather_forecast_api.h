#ifndef WEATHER_FORECAST_API_H
#define WEATHER_FORECAST_API_H

#include <Arduino.h>

// Structure to hold daily weather data
struct DailyForecast {
  String date;        // Date string (e.g., "2025-12-01")
  float minTemp;      // Minimum temperature in Celsius
  float maxTemp;      // Maximum temperature in Celsius
  float minWind;      // Minimum wind speed in m/s
  float maxWind;      // Maximum wind speed in m/s
  float totalRain;    // Total precipitation in mm
  float minHumidity;  // Minimum relative humidity in %
  float maxHumidity;  // Maximum relative humidity in %
  float minPressure;  // Minimum air pressure in hPa
  float maxPressure;  // Maximum air pressure in hPa
  bool valid;         // Whether this forecast entry is valid
};

// Function to fetch 7-day weather forecast from SMHI API
// Returns true if successful, false otherwise
// forecast array should have at least 7 elements
// statusMsg will contain a user-friendly status message
// lat and lon specify the location coordinates
// Parameter types: 0=Temperature, 1=Wind, 2=Rain, 3=Humidity, 4=Pressure
bool getWeatherForecast(DailyForecast forecast[7], String& statusMsg, float lat, float lon, int parameterType);

#endif
