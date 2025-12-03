#include "weather_history_api.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <time.h>

bool getWeatherHistory(HistoryPoint history[], int maxDays, int &outDays, String &statusMsg, int station_nr) {
  // Initialize
  outDays = 0;
  for (int i=0;i<maxDays;i++) {
    history[i].date = String("");
    history[i].avg = NAN;
    history[i].valid = false;
  }

  if (WiFi.status() != WL_CONNECTED) {
    statusMsg = "WiFi not connected";
    return false;
  }

  // Build URL for parameter 1 (temperature) -- latest months
  String url = String("https://opendata-download-metobs.smhi.se/api/version/latest/parameter/1/station/") + String(station_nr) + String("/period/latest-months/data.json");

  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    if (httpCode == -1) statusMsg = "Connection failed";
    else statusMsg = "HTTP error: " + String(httpCode);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    statusMsg = "Parse error";
    return false;
  }

  JsonArray values = doc["value"].as<JsonArray>();
  if (values.size() == 0) {
    statusMsg = "No history values";
    return false;
  }

  // Collect ALL dates from the JSON (up to 64 to handle full latest-months period safely)
  String dateList[64];
  float sum[64];
  int cnt[64];
  int dateCount = 0;
  int totalDatesFound = 0;  // Track total unique dates found

  for (JsonObject v : values) {
    // Get date/time from common keys. Some endpoints return numeric epoch-ms in "date".
    String ds = String("");

    if (v.containsKey("date")) {
      // If date is a string, use it; if numeric, convert from milliseconds -> YYYY-MM-DD
      if (v["date"].is<const char*>()) {
        ds = String(v["date"].as<const char*>());
      } else {
        // treat as epoch milliseconds
        long long ms = v["date"].as<long long>();
        time_t secs = (time_t)(ms / 1000);
        struct tm *ptm = gmtime(&secs);
        if (ptm) {
          char buf[16];
          snprintf(buf, sizeof(buf), "%04d-%02d-%02d", ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday);
          ds = String(buf);
        }
      }
    }
    // fallback keys that may be strings
    if (ds.length() == 0) {
      if (v.containsKey("dateTime") && v["dateTime"].is<const char*>()) ds = String(v["dateTime"].as<const char*>());
      else if (v.containsKey("validTime") && v["validTime"].is<const char*>()) ds = String(v["validTime"].as<const char*>());
      else if (v.containsKey("referenceTime") && v["referenceTime"].is<const char*>()) ds = String(v["referenceTime"].as<const char*>());
    }

    if (ds.length() < 10) continue;
    ds = ds.substring(0,10); // YYYY-MM-DD

    // Value: some APIs return numbers as strings, handle both
    float val = NAN;
    if (v.containsKey("value")) {
      if (v["value"].is<const char*>()) {
        val = atof(v["value"].as<const char*>());
      } else {
        val = v["value"].as<float>();
      }
    } else continue;

    // Find index in dateList
    int idx = -1;
    for (int i=0;i<dateCount;i++) {
      if (dateList[i] == ds) { idx = i; break; }
    }
    if (idx < 0) {
      // New date found
      if (dateCount >= 64) {
        // Array full - shift all entries left to drop oldest, add newest at end
        totalDatesFound++;
        for (int i = 0; i < 63; i++) {
          dateList[i] = dateList[i + 1];
          sum[i] = sum[i + 1];
          cnt[i] = cnt[i + 1];
        }
        idx = 63;  // Use last slot for new date
        dateList[idx] = ds;
        sum[idx] = 0.0f;
        cnt[idx] = 0;
      } else {
        // Still have space
        idx = dateCount;
        dateList[idx] = ds;
        sum[idx] = 0.0f;
        cnt[idx] = 0;
        dateCount++;
        totalDatesFound++;
      }
    }
    sum[idx] += val;
    cnt[idx]++;
  }

  // If array never filled, use actual count
  if (dateCount < 64) {
    totalDatesFound = dateCount;
  }

  // All entries in dateList now contain the MOST RECENT dates
  // Take only maxDays from the end (most recent)
  int startIdx = (dateCount > maxDays) ? (dateCount - maxDays) : 0;
  int filled = 0;
  for (int i = startIdx; i < dateCount && filled < maxDays; i++) {
    if (cnt[i] > 0) {
      history[filled].date = dateList[i];
      history[filled].avg = sum[i] / (float)cnt[i];
      history[filled].valid = true;
      filled++;
    }
  }

  outDays = filled;
  statusMsg = "OK";
  return true;
}
