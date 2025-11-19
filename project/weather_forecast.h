#pragma once
#include <Arduino.h>

struct ForecastDay {
    char date[11];       // YYYY-MM-DD
    float temperature;   // temperature at 12:00
    int symbol;          // Wsymb2 weather symbol
};

struct City {
    const char* name;
    float lat;
    float lon;
    String station;
};

// Example city list (you can extend this later)
extern const City cities[];
extern const int CITY_COUNT;

// Fetch 7-day forecast for a given city index
bool getSevenDayForecast(int cityIndex, ForecastDay out[7], String& statusMsg);