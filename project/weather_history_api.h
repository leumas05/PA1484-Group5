#ifndef WEATHER_HISTORY_API_H
#define WEATHER_HISTORY_API_H

#include <Arduino.h>

// Structure to hold one historical daily value
struct HistoryPoint {
  String date; // YYYY-MM-DD
  float avg;   // average value for the day (e.g., temperature °C)
  bool valid;  // whether this entry contains valid data
};

// Fetch historical station data aggregated per-day for the latest months
// - history: array to fill with up to maxDays elements
// - maxDays: maximum number of days to fill (e.g., 30)
// - outDays: returns the number of days actually filled
// - statusMsg: human readable status on error
// - station_nr: SMHI station id (e.g., 65090)
// Returns true on success and fills history/outDays
bool getWeatherHistory(HistoryPoint history[], int maxDays, int &outDays, String &statusMsg, int station_nr);

#endif
