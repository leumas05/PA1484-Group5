#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <time.h>
#include <LilyGo_AMOLED.h>
#include <LV_Helper.h>
#include <lvgl.h>
#include <credentials.h>
#include "weather_api.h"
#include "wifi_manager.h"
#include "weather_forecast.h"


// PlatformIO automatically compile weather_forecast.cpp
LilyGo_Class amoled;
static lv_obj_t* tileview;
static lv_obj_t* t1;
static lv_obj_t* t_boot;
static lv_obj_t* t2;
static lv_obj_t* t3;
static lv_obj_t* t1_label;
static lv_obj_t* t1_wifi_indicator;  // WiFi status indicator for tile #1
static lv_obj_t* t_boot_label;
static lv_obj_t* t2_label;
static lv_obj_t* t3_label;
static lv_obj_t* t2_dropdown; //
static lv_obj_t* t2_param_label; //
static uint16_t t2_selected_index = 0; //
static lv_obj_t* t2_dropdown_cities; //
static lv_obj_t* t2_city_label; //
static uint16_t t2_selected_city_index = 0; //
static lv_obj_t* t4;
static lv_obj_t* row_day[7];
static lv_obj_t* row_icon[7];
static lv_obj_t* row_temp[7];
static unsigned long boot_start_ms = 0;
static bool boot_switched = false;
static float lastTemperature = NAN;

// Timer callback: update WiFi status indicator on tile #1 every second
static void wifi_indicator_timer_cb(lv_timer_t* timer)
{
  LV_UNUSED(timer);
  if (!t1_wifi_indicator) return;
  // Show tick or X based on WiFi connection status
  if (WiFi.status() == WL_CONNECTED) {
    lv_label_set_text(t1_wifi_indicator, LV_SYMBOL_OK);  // Checkmark/tick
    lv_obj_set_style_text_color(t1_wifi_indicator, lv_color_make(0, 200, 0), 0);  // Green
  } else {
    lv_label_set_text(t1_wifi_indicator, LV_SYMBOL_CLOSE);  // X
    lv_obj_set_style_text_color(t1_wifi_indicator, lv_color_make(200, 0, 0), 0);  // Red
  }
}

// Timer callback: update temperature on tile #3 every 60 seconds
static void temperature_timer_cb(lv_timer_t* timer)
{
  LV_UNUSED(timer);
  if (!t3_label) return;
  
  // Check if WiFi is connected
  if (WiFi.status() != WL_CONNECTED) {
    lv_label_set_text(t3_label, "Waiting for WiFi...");
    return;
  }
  
  // WiFi is connected, fetch temperature
  lv_label_set_text(t3_label, "Fetching temp...");
  
  String statusMsg;
  lastTemperature = getCurrentTemperature(statusMsg);
  
  if (!isnan(lastTemperature)) {
    char tempStr[64];
    snprintf(tempStr, sizeof(tempStr), "Temp: %.1f°C", lastTemperature);
    lv_label_set_text(t3_label, tempStr);
  } else {
    char errorStr[64];
    snprintf(errorStr, sizeof(errorStr), "Error:\n(%s)", statusMsg.c_str());
    lv_label_set_text(t3_label, errorStr);
  }
}

// Function: Creates UI
static void create_ui()
{
  // Fullscreen Tileview
  tileview = lv_tileview_create(lv_scr_act());
  lv_obj_set_size(tileview, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
  lv_obj_set_scrollbar_mode(tileview, LV_SCROLLBAR_MODE_OFF);

  // Add tiles: boot screen at 0, then regular tiles at 1..2, and tile 3 (temperature)
  t_boot = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_HOR);
  t1 = lv_tileview_add_tile(tileview, 1, 0, LV_DIR_HOR);
  t2 = lv_tileview_add_tile(tileview, 2, 0, LV_DIR_HOR);
  t3 = lv_tileview_add_tile(tileview, 3, 0, LV_DIR_HOR);

    // Boot tile (shows first for a short time)
  {
    t_boot_label = lv_label_create(t_boot);
    lv_label_set_text(t_boot_label, "Grupp 5 v.0.3");
    lv_obj_set_style_text_font(t_boot_label, &lv_font_montserrat_28, 0);
    lv_obj_center(t_boot_label);
  }

  // Tile #1
  {
    t1_label = lv_label_create(t1);
    lv_label_set_text(t1_label, "Welcome to WeAPP!");
    lv_obj_set_style_text_font(t1_label, &lv_font_montserrat_28, 0);
    lv_obj_center(t1_label);
    
    // Add WiFi indicator in top-right corner
    t1_wifi_indicator = lv_label_create(t1);
    lv_label_set_text(t1_wifi_indicator, LV_SYMBOL_CLOSE);  // Start with X
    lv_obj_set_style_text_font(t1_wifi_indicator, &lv_font_montserrat_20, 0);
    lv_obj_align(t1_wifi_indicator, LV_ALIGN_TOP_RIGHT, -10, 10);
  }

  // Tile #2 SETTINGS TILE
  {
    t2_label = lv_label_create(t2);
    lv_label_set_text(t2_label, "");
    lv_obj_set_style_text_font(t2_label, &lv_font_montserrat_28, 0);
    lv_obj_center(t2_label);

    // Dropdown to choose which weather parameter to show
    t2_dropdown = lv_dropdown_create(t2);
    const char* dd_opts = "Temperature\nWind\nRain\nHumidity\nPressure";
    lv_dropdown_set_options_static(t2_dropdown, dd_opts);
    lv_dropdown_set_selected(t2_dropdown, 0);  // Default to Temperature
    lv_obj_set_width(t2_dropdown, 200);
    lv_obj_align(t2_dropdown, LV_ALIGN_TOP_MID, 0, 20);
    // Label that shows the chosen parameter's current value
    t2_param_label = lv_label_create(t2);
    lv_label_set_text(t2_param_label, "Select parameter...");
    lv_obj_set_style_text_font(t2_param_label, &lv_font_montserrat_20, 0);
    lv_obj_align_to(t2_param_label, t2_dropdown, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);
    // Dropdown event: update immediately when user selects a different option
    lv_obj_add_event_cb(t2_dropdown, [](lv_event_t* e){
      lv_obj_t* dd = lv_event_get_target(e);
      t2_selected_index = lv_dropdown_get_selected(dd);
      // Direct call to update immediately
      extern void update_t2_param_display();
      update_t2_param_display();
    }, LV_EVENT_VALUE_CHANGED, NULL);

    t2_dropdown_cities = lv_dropdown_create(t2);
    const char* dd_opts_city = "Karlskrona\nStockholm\nGothenburg\nHaparanda";
    lv_dropdown_set_options_static(t2_dropdown_cities, dd_opts_city);
    lv_dropdown_set_selected(t2_dropdown_cities, 0);  // Default to Karlskrona
    lv_obj_set_width(t2_dropdown_cities, 200);
    lv_obj_align(t2_dropdown_cities, LV_ALIGN_TOP_LEFT, 0, 20);

    t2_city_label = lv_label_create(t2);
    lv_label_set_text(t2_city_label, "Select city...");
    lv_obj_set_style_text_font(t2_city_label, &lv_font_montserrat_20, 0);
    lv_obj_align_to(t2_city_label, t2_dropdown_cities, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);

    lv_obj_add_event_cb(t2_dropdown_cities, [](lv_event_t* e){
      lv_obj_t* dd = lv_event_get_target(e);
      t2_selected_city_index = lv_dropdown_get_selected(dd);
      // Direct call to update immediately
      extern void update_t2_city_display();
      update_t2_city_display();
      extern void update_t2_param_display();
      update_t2_param_display();  // Also update the parameter display for the new city
      // Also fetch an updated forecast for the newly selected city (if WiFi is ready)
      if (WiFi.status() == WL_CONNECTED) {
        ForecastDay days[7];
        String status;
        if (getSevenDayForecast(t2_selected_city_index, days, status)) {
          update_forecast_ui(days);
        } else {
          Serial.println("Forecast failed: " + status);
        }
      }
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // Reset button: restores default parameter and city selection (INTE TESTAD ÄN)
    {
      lv_obj_t* reset_btn = lv_btn_create(t2);
      lv_obj_set_size(reset_btn, 120, 40);
      lv_obj_align(reset_btn, LV_ALIGN_TOP_RIGHT, -10, 18);
      lv_obj_t* lbl = lv_label_create(reset_btn);
      lv_label_set_text(lbl, "Reset");
      lv_obj_center(lbl);
      lv_obj_add_event_cb(reset_btn, [](lv_event_t* e){
        LV_UNUSED(e);
        // Reset dropdown selections to defaults
        if (t2_dropdown) lv_dropdown_set_selected(t2_dropdown, 0);
        t2_selected_index = 0;
        if (t2_dropdown_cities) lv_dropdown_set_selected(t2_dropdown_cities, 0);
        t2_selected_city_index = 0;

        // Update displays
        update_t2_param_display();
        update_t2_city_display();
        
      }, LV_EVENT_CLICKED, NULL);
    }
  }

  // Tile #3 - Temperature Display (formerly tile #4)
  {
    t3_label = lv_label_create(t3);
    lv_label_set_text(t3_label, "Loading...");
    lv_obj_set_style_text_font(t3_label, &lv_font_montserrat_28, 0);
    lv_obj_center(t3_label);
  }

  // Set tileview to start at boot tile (0,0) after all tiles are created
  lv_obj_set_tile_id(tileview, 0, 0, LV_ANIM_OFF);

   // Create a timer to refresh WiFi indicator on tile #1 every 1s
  lv_timer_create(wifi_indicator_timer_cb, 1000, NULL);
  // Create a timer to refresh temperature every 5s (5000ms)
  lv_timer_create(temperature_timer_cb, 5000, NULL);
  // Create a timer to refresh the selected parameter on tile #2 every 10s
  lv_timer_create([](lv_timer_t* t){
    (void)t;
    update_t2_param_display();
  }, 10000, NULL);
  // Trigger first temperature fetch after 5 seconds to allow WiFi to connect
  lv_timer_create([](lv_timer_t* t){ 
    temperature_timer_cb(t); 
    lv_timer_del(t); 
  }, 5000, NULL);

  
  // Tile #4 — Forecast
  // Create the forecast UI (adds the tile and the rows)
  create_forecast_ui();
  }

static void create_forecast_ui() {
    // Add Tile #4
    t4 = lv_tileview_add_tile(tileview, 4, 0, LV_DIR_HOR);

    // Create 7 rows for day, icon, and temperature
    for (int i = 0; i < 7; i++) {
        row_day[i] = lv_label_create(t4);
        row_icon[i] = lv_label_create(t4);
        row_temp[i] = lv_label_create(t4);

        // Set font
        lv_obj_set_style_text_font(row_day[i], &lv_font_montserrat_40, 0);
        lv_obj_set_style_text_font(row_icon[i], &lv_font_montserrat_40, 0);
        lv_obj_set_style_text_font(row_temp[i], &lv_font_montserrat_40, 0);

        // Position labels in columns, Y shifts by 40 each row
        int y = 20 + i * 40;
        lv_obj_align(row_day[i], LV_ALIGN_TOP_LEFT, 10, y);
        lv_obj_align(row_icon[i], LV_ALIGN_TOP_LEFT, 240, y);
        lv_obj_align(row_temp[i], LV_ALIGN_TOP_LEFT, 450, y);

        // Initialize with placeholder text
        lv_label_set_text(row_day[i], "...");
        lv_label_set_text(row_icon[i], "-");
        lv_label_set_text(row_temp[i], "--°C");
    }
  }

static void update_forecast_ui(ForecastDay days[7]) {
    for (int i = 0; i < 7; i++) {
        lv_label_set_text(row_day[i], days[i].date);

        const char* icon = "?";
        switch (days[i].symbol) {
            case 1: icon = "☀"; break;
            case 2: icon = "🌤"; break;
            case 3: icon = "⛅"; break;
            case 4: icon = "☁"; break;
            case 5: icon = "🌫"; break;
            case 6: icon = "🌧"; break;
            case 7: icon = "🌦"; break;
            case 8: icon = "⛈"; break;
        }
        lv_label_set_text(row_icon[i], icon);

        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f°C", days[i].temperature);
        lv_label_set_text(row_temp[i], buf);
    }
}

// Must have function: Setup is run once on startup
void setup()
{
  // Start Serial over USB
  Serial.begin(115200);
  // short delay to allow Serial Monitor to open
  delay(200);

  // Print Hello World to serial
  Serial.println("Hello, World from LilyGO!");
  // Initialize display and LVGL
  Serial.println("Initializing display...");
  if (!amoled.begin()) {
    Serial.println("Failed to initialize display (amoled.begin()).");
  } else {
    Serial.println("Display initialized.");
  }

  // Start LVGL helper and create UI
  beginLvglHelper(amoled, /*debug=*/true);
  create_ui();
  // Start non-blocking WiFi connection so the boot screen shows immediately
  initWiFi();
  // Start boot timer
  boot_start_ms = millis();
  // 7-day forecast: update every hour
  lv_timer_create([](lv_timer_t* timer) {
      ForecastDay days[7];
      String status;

      if (getSevenDayForecast(0, days, status)) {
          update_forecast_ui(days);
      } else {
          Serial.println("Forecast error: " + status);
      }
  }, 3600000, NULL);

  // First update after 3 seconds (WiFi needs time)
  lv_timer_create([](lv_timer_t* timer) {
    if (WiFi.status() != WL_CONNECTED) return;  // wait for WiFi
    ForecastDay days[7];
    String status;
    Serial.println("Fetching forecast...");
    if (!getSevenDayForecast(0, days, status)) {
        Serial.println("Forecast failed: " + status);
    } else {
        Serial.println("Forecast success");
        update_forecast_ui(days);
        lv_timer_del(timer);  // stop one-shot
    }
}, 5000, NULL);  // give WiFi a few seconds to connect
}

// Must have function: Loop runs continously on device after setup
void loop()
{
  /* Let LVGL do its work. Call the timer handler frequently. */
  lv_timer_handler();
  delay(5);
  // After 3 seconds, switch away from boot tile to the first normal tile
  if (!boot_switched && boot_start_ms != 0 && (millis() - boot_start_ms) >= 3000) {
    boot_switched = true;
  // Switch to tile (1,0) which is t1
  // lv_tileview_set_tile is not part of this LVGL build; use lv_obj_set_tile_id
  lv_obj_set_tile_id(tileview, 1, 0, LV_ANIM_OFF);
    // Hide boot tile
    lv_obj_add_flag(t_boot, LV_OBJ_FLAG_HIDDEN);
  }
}

// Update the displayed parameter on tile #2 according to t2_selected_index
void update_t2_param_display()
{
  if (!t2_param_label) return;

  if (WiFi.status() != WL_CONNECTED) {
    lv_label_set_text(t2_param_label, "WiFi not connected");
    return;
  }

  String err;
  char buf[128];

  switch (t2_selected_index) {
    case 0: { // Temperature
      float v = getCurrentTemperature(err);
      if (!isnan(v)) {
        snprintf(buf, sizeof(buf), "Temp: %.1f °C", v);
      } else {
        snprintf(buf, sizeof(buf), "Temp err: %s", err.c_str());
      }
      break;
    }
    case 1: { // Wind (show speed and direction together)
      String errSpeed, errDir;
      float speed = getCurrentWindSpeed(errSpeed);
      float dir = getCurrentWindDirection(errDir);
      bool haveSpeed = !isnan(speed);
      bool haveDir = (dir >= 0);

      if (haveSpeed && haveDir) {
        snprintf(buf, sizeof(buf), "Wind: %.1f m/s, Dir: %d°", speed, dir);
      } else if (haveSpeed) {
        snprintf(buf, sizeof(buf), "Wind: %.1f m/s", speed);
      } else if (haveDir) {
        snprintf(buf, sizeof(buf), "Wind dir: %d°", dir);
      } else {
        if (errSpeed.length()) snprintf(buf, sizeof(buf), "Wind err: %s", errSpeed.c_str());
        else if (errDir.length()) snprintf(buf, sizeof(buf), "Wind err: %s", errDir.c_str());
        else snprintf(buf, sizeof(buf), "Wind: unavailable");
      }
      break;
    }
    case 2: { // Rain
      float v = getCurrentRain(err);
      if (!isnan(v)) {
        snprintf(buf, sizeof(buf), "Rain: %.1f mm", v);
      } else {
        snprintf(buf, sizeof(buf), "Rain err: %s", err.c_str());
      }
      break;
    }
    case 3: { // Humidity
      float v = getCurrentHumidity(err);
      if (!isnan(v)) {
        snprintf(buf, sizeof(buf), "Humidity: %.1f %%", v);
      } else {
        snprintf(buf, sizeof(buf), "Humidity err: %s", err.c_str());
      }
      break;
    }
    case 4: { // Pressure
      float v = getCurrentPressure(err);
      if (!isnan(v)) {
        snprintf(buf, sizeof(buf), "Pressure: %.1f hPa", v);
      } else {
        snprintf(buf, sizeof(buf), "Pressure err: %s", err.c_str());
      }
      break;
    }
  }
  lv_label_set_text(t2_param_label, buf);
}

void update_t2_city_display()
{
  if (!t2_city_label) return;

  if (WiFi.status() != WL_CONNECTED) {
    lv_label_set_text(t2_city_label, "");
    return;
  }

  String err;
  char buf[128];
  int new_station = -1;
  const char* cityName = "Unknown";

  switch (t2_selected_city_index) {
    case 0: cityName = "Karlskrona"; break;
    case 1: cityName = "Stockholm"; break;
    case 2: cityName = "Gothenburg"; break;
    case 3: cityName = "Haparanda"; break;
  }

  new_station = change_station_nr(cityName);

  /*if (new_station > 0) {
    snprintf(buf, sizeof(buf), "City: %s (Station %d)", cityName, new_station);
  } else {
    snprintf(buf, sizeof(buf), "City: %s", cityName);
  }*/

  lv_label_set_text(t2_city_label, buf);
}