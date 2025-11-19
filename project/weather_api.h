#ifndef WEATHER_API_H
#define WEATHER_API_H

#include <Arduino.h>

// Function to fetch current temperature from SMHI API
// Returns temperature in Celsius or NAN if request fails
// statusMsg will contain a user-friendly status message
float getCurrentTemperature(String& statusMsg);

// Function to fetch current wind data from SMHI API
// Returns wind speed in m/s or NAN if request fails
// windDirection will contain wind direction in degrees (0-360)
// statusMsg will contain a user-friendly status message
float getCurrentWind(String& statusMsg, float& windDirection);

#endif
