#ifndef WEATHER_API_H
#define WEATHER_API_H

#include <Arduino.h>

// Function to fetch current temperature from SMHI API
// Returns temperature in Celsius or NAN if request fails
// statusMsg will contain a user-friendly status message
float getCurrentTemperature(String& statusMsg);

// Function to fetch current wind data from SMHI API
// Returns wind speed in m/s or NAN if request fails
// statusMsg will contain a user-friendly status message
float getCurrentWindSpeed(String& statusMsg);

// Function to fetch current wind direction from SMHI API
// Returns wind direction in degrees (0-360) or NAN if request fails
// statusMsg will contain a user-friendly status message
float getCurrentWindDirection(String& statusMsg);

// Function to fetch current rainfall from SMHI API
// Returns rainfall in mm or NAN if request fails
// statusMsg will contain a user-friendly status message
float getCurrentRain(String& statusMsg);

// Function to fetch current humidity from SMHI API
// Returns humidity in percentage (0-100) or NAN if request fails
// statusMsg will contain a user-friendly status message
float getCurrentHumidity(String& statusMsg);

float getCurrentPressure(String& statusMsg);

#endif
