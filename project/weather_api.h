#ifndef WEATHER_API_H
#define WEATHER_API_H

#include <Arduino.h>

// Function to fetch current temperature from SMHI API
// Returns temperature in Celsius or NAN if request fails
// statusMsg will contain a user-friendly status message
float getCurrentTemperature(String& statusMsg);

#endif
