#include "weather_forecast.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ------------------ Cities List ------------------

const City cities[] = {
    {"Karlskrona", 56.1612, 15.5869, 65090},
    {"Stockholm", 59.33, 18.06},
    {"Gothenburg", 57.70, 11.97},
    {"Malmo", 55.61, 13.00}
};
const int CITY_COUNT = sizeof(cities) / sizeof(City);

// --------------------------------------------------

static float getParam(JsonObject step, const char* name) {
    JsonArray params = step["parameters"];
    for (JsonObject p : params) {
        if (strcmp(p["name"], name) == 0) {
            return p["values"][0].as<float>();
        }
    }
    return NAN;
}

// Extract YYYY-MM-DD into out[]
static void extractDate(const char* iso, char* outDate) {
    strncpy(outDate, iso, 10);
    outDate[10] = '\0';
}

// ------------------ Forecast Function ------------------

bool getSevenDayForecast(int cityIndex, ForecastDay out[7], String& statusMsg) {
    if (cityIndex < 0 || cityIndex >= CITY_COUNT) {
        statusMsg = "Invalid city";
        return false;
    }

    const City& C = cities[cityIndex];

    // Build URL
    char url[300];
    snprintf(url, sizeof(url),
        "https://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/"
        "geotype/point/lon/%.4f/lat/%.4f/data.json",
        C.lon, C.lat);

    HTTPClient http;
    http.begin(url);

    int code = http.GET();
    if (code != 200) {
        statusMsg = "HTTP error";
        http.end();
        return false;
    }

    JsonDocument doc;  // ~100 kB needed
    DeserializationError err = deserializeJson(doc, http.getString());
    http.end();

    if (err) {
        statusMsg = "JSON parse error";
        return false;
    }

    JsonArray series = doc["timeSeries"];
    if (!series) {
        statusMsg = "Missing timeSeries";
        return false;
    }

    // Initialize output
    for (int i = 0; i < 7; i++) {
        out[i].temperature = NAN;
        out[i].symbol = -1;
        strcpy(out[i].date, "");
    }

    int filledDays = 0;

    // Look for each day's 12:00 reading
    for (JsonObject step : series) {
        const char* time = step["validTime"];   // ISO timestamp

        // Look only for 12:00 UTC
        if (strstr(time, "T12:00:00Z") == nullptr) continue;

        if (filledDays >= 7) break;

        extractDate(time, out[filledDays].date);

        out[filledDays].temperature = getParam(step, "t");
        out[filledDays].symbol = (int)getParam(step, "Wsymb2");

        filledDays++;
    }

    if (filledDays < 7) {
        statusMsg = "Not enough data";
        return false;
    }

    statusMsg = "OK";
    return true;
}