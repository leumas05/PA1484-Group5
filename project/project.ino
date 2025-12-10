#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <time.h>
#include <Preferences.h>
#include <LilyGo_AMOLED.h>
#include <LV_Helper.h>
#include <lvgl.h>
#include <credentials.h>
#include "weather_api.h"
#include "weather_forecast_api.h"
#include "weather_history_api.h"
#include "wifi_manager.h"
LV_IMG_DECLARE(clear_day_50dp_E3E3E3_FILL0_wght400_GRAD0_opsz48);
LV_IMG_DECLARE(cloud_50dp_E3E3E3_FILL0_wght400_GRAD0_opsz48);
LV_IMG_DECLARE(foggy_50dp_E3E3E3_FILL0_wght400_GRAD0_opsz48);
LV_IMG_DECLARE(question_mark_50dp_E3E3E3_FILL0_wght400_GRAD0_opsz48);
LV_IMG_DECLARE(rainy_50dp_E3E3E3_FILL0_wght400_GRAD0_opsz48);
LV_IMG_DECLARE(snowing_50dp_E3E3E3_FILL0_wght400_GRAD0_opsz48);
LV_IMG_DECLARE(thunderstorm_50dp_E3E3E3_FILL0_wght400_GRAD0_opsz48);

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
static lv_obj_t* t2_dropdown; //
static lv_obj_t* t2_param_label; //
static uint16_t t2_selected_index = 0; //
static lv_obj_t* t2_dropdown_cities; //
static uint16_t t2_selected_city_index = 0; //
static Preferences preferences;  // NVS storage for saved selections
static uint8_t saved_param_index = 0;
static uint8_t saved_city_index = 0;
static bool has_saved_selection = false;
static bool prefs_ready = false;
// City coordinates (lat, lon)
static float city_coordinates[][2] = {
  {56.16, 15.59},  // Karlskrona
  {59.33, 18.07},  // Stockholm
  {57.71, 11.97},  // Göteborg
  {67.86, 20.23},  // Kiruna
  {55.61, 13.00}   // Malmö
};
// Tile #3 chart widgets for forecast graph
static lv_obj_t* t3_chart;
static lv_chart_series_t* t3_series_min;
static lv_chart_series_t* t3_series_max;
static lv_obj_t* t3_title_label;
static lv_obj_t* t3_date_labels[7];  // X-axis date labels
static lv_obj_t* t3_symbol_labels[7];  // Symbol code labels
static lv_obj_t* t3_symbol_images[7];  // Symbol icons for select codes
// Tile #4 globals (history)
static const int T4_WINDOW_DAYS = 7;            // Visible days per window
static const int T4_HISTORY_MAX_DAYS = 155;     // Approximately five months of data
static lv_obj_t* t4_chart;
static lv_chart_series_t* t4_series;
static lv_obj_t* t4_title_label;
static lv_obj_t* t4_date_labels[T4_WINDOW_DAYS];  // X-axis date labels for visible window
static lv_obj_t* t4_slider;  // Vertical slider to select 7-day window
static int t4_point_count = 0;
static HistoryPoint t4_full_history[T4_HISTORY_MAX_DAYS];  // Store up to five months of data
static HistoryPoint t4_history_buffer[T4_HISTORY_MAX_DAYS];  // Temporary fetch buffer (kept off stack)
static int t4_full_days = 0;  // Number of valid days in full history
static int t4_window_start = 0;  // Starting index of visible window
static unsigned long boot_start_ms = 0;
static bool boot_switched = false;
static float lastTemperature = NAN;

static uint16_t clamp_param_index(uint16_t index)
{
  if (index > 4) return 4;
  return index;
}

static uint16_t clamp_city_index(uint16_t index)
{
  if (index > 4) return 4;
  return index;
}

static void load_saved_selection()
{
  if (!prefs_ready) {
    t2_selected_index = 0;
    t2_selected_city_index = 0;
    has_saved_selection = false;
    return;
  }

  saved_param_index = clamp_param_index(preferences.getUChar("param", 0));
  saved_city_index = clamp_city_index(preferences.getUChar("city", 0));
  has_saved_selection = preferences.isKey("param") && preferences.isKey("city");

  if (has_saved_selection) {
    t2_selected_index = saved_param_index;
    t2_selected_city_index = saved_city_index;
  } else {
    t2_selected_index = 0;
    t2_selected_city_index = 0;
  }
}

static void save_current_selection()
{
  if (!prefs_ready) return;
  saved_param_index = clamp_param_index(t2_selected_index);
  saved_city_index = clamp_city_index(t2_selected_city_index);
  preferences.putUChar("param", saved_param_index);
  preferences.putUChar("city", saved_city_index);
  has_saved_selection = true;
}

static const char* get_symbol_description(int code)
{
  switch (code) {
    case 0: return "no data";
    case 1: return "clear sky";  //klar
    case 2: return "nearly clear";  //klar
    case 3: return "var cloud"; //klar
    case 4: return "half clear"; //klar
    case 5: return "cloudy"; //klar
    case 6: return "overcast"; //klar
    case 7: return "fog";
    case 8: return "light rain sh"; //klar
    case 9: return "mod rain sh"; //klar
    case 10: return "heavy rain sh"; //klar
    case 11: return "thunderstorm";
    case 12: return "light sleet sh"; //klar
    case 13: return "mod sleet sh"; //klar
    case 14: return "heavy sleet sh"; //klar
    case 15: return "light snow sh"; //klar
    case 16: return "mod snow sh"; //klar
    case 17: return "heavy snow sh"; //klar
    case 18: return "light rain"; //klar
    case 19: return "mod rain"; //klar
    case 20: return "heavy rain"; //klar
    case 21: return "thunder";
    case 22: return "light sleet"; //klar
    case 23: return "mod sleet"; //klar
    case 24: return "heavy sleet"; //klar
    case 25: return "light snow"; //klar
    case 26: return "mod snow"; //klar
    case 27: return "heavy snow"; //klar
    default: return "unknown";
  }
}

static bool is_sun_symbol(int code)
{
  return code >= 1 && code <= 2;
}

static bool is_cloud_symbol(int code)
{
  return code >= 3 && code <= 6;
}

static bool is_rain_symbol(int code)
{
  switch (code) {
    case 8:
    case 9:
    case 10:
    case 12:
    case 13:
    case 14:
    case 18:
    case 19:
    case 20:
    case 22:
    case 23:
    case 24:
      return true;
    default:
      return false;
  }
}

static bool is_snow_symbol(int code)
{
  switch (code) {
    case 15:
    case 16:
    case 17:
    case 25:
    case 26:
    case 27:
      return true;
    default:
      return false;
  }
}

static bool is_thunder_symbol(int code)
{
  return code == 11 || code == 21;
}

static bool is_fog_symbol(int code)
{
  return code == 7;
}

static bool is_question_symbol(int code)
{
  return code == 0;
}

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

// Timer callback: update 7-day forecast on tile #3 every 5 minutes 
static void forecast_timer_cb(lv_timer_t* timer)
{
  LV_UNUSED(timer);
  if (!t3_chart || !t3_title_label) return;
  
  // Check if WiFi is connected
  if (WiFi.status() != WL_CONNECTED) {
    lv_label_set_text(t3_title_label, "Waiting for WiFi...");
    return;
  }
  
  // WiFi is connected, fetch 7-day forecast
  lv_label_set_text(t3_title_label, "Fetching forecast...");
  
  DailyForecast forecast[7];
  String statusMsg;
  
  // Get coordinates for the currently selected city
  float lat = city_coordinates[t2_selected_city_index][0];
  float lon = city_coordinates[t2_selected_city_index][1];

  const uint8_t forecast_param_index = 0;  // Always fetch temperature for forecast tile
  if (getWeatherForecast(forecast, statusMsg, lat, lon, forecast_param_index)) {
    const char* paramName = "Temperature";
    const char* unit = "°C";
    
    // Get city name based on selected city index
    const char* cityName = "Unknown";
    switch(t2_selected_city_index) {
      case 0: cityName = "Karlskrona"; break;
      case 1: cityName = "Stockholm"; break;
      case 2: cityName = "Gothenburg"; break;
      case 3: cityName = "Kiruna"; break;
      case 4: cityName = "Malmo"; break;
    }
    
    String titleText = "7-Day Forecast in " + String(cityName) + " - " + paramName + " (" + unit + ")";
    lv_label_set_text(t3_title_label, titleText.c_str());
    
    // Properly clear existing chart data by setting point count to 0 and back to 7
    lv_chart_set_point_count(t3_chart, 0);
    lv_chart_set_point_count(t3_chart, 7);
    
    // Also clear all values
    lv_chart_set_all_value(t3_chart, t3_series_min, LV_CHART_POINT_NONE);
    lv_chart_set_all_value(t3_chart, t3_series_max, LV_CHART_POINT_NONE);
    
    // First pass: find min/max values for Y-axis range
    float globalMin = 999999;
    float globalMax = -999999;
    
    for (int i = 0; i < 7; i++) {
      if (forecast[i].valid) {
        float minVal = forecast[i].minTemp;
        float maxVal = forecast[i].maxTemp;
        if (!isnan(minVal) && minVal < globalMin) globalMin = minVal;
        if (!isnan(maxVal) && maxVal > globalMax) globalMax = maxVal;
      }
    }
    
    // Set Y-axis range with some padding
    if (globalMin < 999999 && globalMax > -999999) {
      float range = globalMax - globalMin;
      int yMin = (int)(globalMin - range * 0.1);
      int yMax = (int)(globalMax + range * 0.1);
      lv_chart_set_range(t3_chart, LV_CHART_AXIS_PRIMARY_Y, yMin, yMax);
    }
    
    // Get chart dimensions for positioning symbol labels
    int chart_x = lv_obj_get_x(t3_chart);
    int chart_width = lv_obj_get_width(t3_chart);
    
    // Second pass: update chart with forecast data and position symbol labels
    for (int i = 0; i < 7; i++) {
      if (forecast[i].valid) {
        int minVal = LV_CHART_POINT_NONE;
        int maxVal = LV_CHART_POINT_NONE;
        if (!isnan(forecast[i].minTemp) && !isnan(forecast[i].maxTemp)) {
          float avgTemp = (forecast[i].minTemp + forecast[i].maxTemp) / 2.0f;
          minVal = (int)avgTemp;
          maxVal = (int)avgTemp;
        }
        lv_chart_set_next_value(t3_chart, t3_series_min, minVal);
        lv_chart_set_next_value(t3_chart, t3_series_max, maxVal);
      }
    }
    
    // Set X-axis labels with dates (MM-DD format) and position them to align with data points
    // Get chart padding to calculate data point positions
    lv_coord_t left_pad = lv_obj_get_style_pad_left(t3_chart, LV_PART_MAIN);
    lv_coord_t right_pad = lv_obj_get_style_pad_right(t3_chart, LV_PART_MAIN);
    int plot_area_width = chart_width - left_pad - right_pad;
    int plot_x_start = chart_x + left_pad;
    
    for (int i = 0; i < 7; i++) {
      // Calculate X position to align with data points
      int x_center = plot_x_start + (i * plot_area_width) / 7 + (plot_area_width / 14);
      
      if (t3_date_labels[i]) {
        if (forecast[i].valid && forecast[i].date.length() >= 10) {
          String date = forecast[i].date.substring(5); // Get MM-DD
          lv_label_set_text(t3_date_labels[i], date.c_str());
          
          // Position date label centered under the data point
          lv_obj_set_x(t3_date_labels[i], x_center - 15);
        } else {
          lv_label_set_text(t3_date_labels[i], "");
        }
      }
      
      // Position symbol codes above the date labels, aligned on X-axis
      bool has_symbol = forecast[i].valid && forecast[i].symbolCode >= 0;
      bool show_sun_icon = has_symbol && is_sun_symbol(forecast[i].symbolCode);
      bool show_cloud_icon = has_symbol && is_cloud_symbol(forecast[i].symbolCode);
      bool show_fog_icon = has_symbol && is_fog_symbol(forecast[i].symbolCode);
      bool show_question_icon = has_symbol && is_question_symbol(forecast[i].symbolCode);
      bool show_rain_icon = has_symbol && is_rain_symbol(forecast[i].symbolCode);
      bool show_snow_icon = has_symbol && is_snow_symbol(forecast[i].symbolCode);
      bool show_thunder_icon = has_symbol && is_thunder_symbol(forecast[i].symbolCode);
      bool show_any_icon = show_thunder_icon || show_rain_icon || show_snow_icon || show_fog_icon || show_cloud_icon || show_sun_icon || show_question_icon;
      int date_y = t3_date_labels[i] ? lv_obj_get_y(t3_date_labels[i]) : 0;

      if (t3_symbol_images[i]) {
        if (show_any_icon) {
          const lv_img_dsc_t* icon_src = NULL;
          if (show_thunder_icon) {
            icon_src = &thunderstorm_50dp_E3E3E3_FILL0_wght400_GRAD0_opsz48;
          } else if (show_rain_icon) {
            icon_src = &rainy_50dp_E3E3E3_FILL0_wght400_GRAD0_opsz48;
          } else if (show_snow_icon) {
            icon_src = &snowing_50dp_E3E3E3_FILL0_wght400_GRAD0_opsz48;
          } else if (show_fog_icon) {
            icon_src = &foggy_50dp_E3E3E3_FILL0_wght400_GRAD0_opsz48;
          } else if (show_cloud_icon) {
            icon_src = &cloud_50dp_E3E3E3_FILL0_wght400_GRAD0_opsz48;
          } else if (show_sun_icon) {
            icon_src = &clear_day_50dp_E3E3E3_FILL0_wght400_GRAD0_opsz48;
          } else if (show_question_icon) {
            icon_src = &question_mark_50dp_E3E3E3_FILL0_wght400_GRAD0_opsz48;
          }
          if (icon_src) {
            lv_img_set_src(t3_symbol_images[i], icon_src);
            lv_obj_clear_flag(t3_symbol_images[i], LV_OBJ_FLAG_HIDDEN);
            lv_coord_t img_w = lv_obj_get_width(t3_symbol_images[i]);
            lv_coord_t img_h = lv_obj_get_height(t3_symbol_images[i]);
            if (img_w == 0) img_w = 50;
            if (img_h == 0) img_h = 50;
            lv_obj_set_pos(t3_symbol_images[i], x_center - (img_w / 2), date_y - img_h - 10);
          } else {
            lv_obj_add_flag(t3_symbol_images[i], LV_OBJ_FLAG_HIDDEN);
          }
        } else {
          lv_obj_add_flag(t3_symbol_images[i], LV_OBJ_FLAG_HIDDEN);
        }
      }

      if (t3_symbol_labels[i]) {
        if (show_any_icon) {
          lv_label_set_text(t3_symbol_labels[i], "");
          lv_obj_add_flag(t3_symbol_labels[i], LV_OBJ_FLAG_HIDDEN);
        } else if (has_symbol) {
          lv_obj_clear_flag(t3_symbol_labels[i], LV_OBJ_FLAG_HIDDEN);
          lv_obj_set_pos(t3_symbol_labels[i], x_center - 8, date_y - 40);
          const char* meaning = get_symbol_description(forecast[i].symbolCode);
          lv_label_set_text(t3_symbol_labels[i], meaning);
        } else {
          lv_label_set_text(t3_symbol_labels[i], "");
          lv_obj_clear_flag(t3_symbol_labels[i], LV_OBJ_FLAG_HIDDEN);
        }
      }
    }
    
    lv_chart_set_axis_tick(t3_chart, LV_CHART_AXIS_PRIMARY_Y, 5, 3, 6, 1, true, 50);
    
    lv_chart_refresh(t3_chart);
  } else {
    char errorStr[64];
    snprintf(errorStr, sizeof(errorStr), "Error: %s", statusMsg.c_str());
    lv_label_set_text(t3_title_label, errorStr);
  }
}

// Function: Creates UI
static void create_ui()
{
  // Fullscreen Tileview
  tileview = lv_tileview_create(lv_scr_act());
  lv_obj_set_size(tileview, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
  lv_obj_set_scrollbar_mode(tileview, LV_SCROLLBAR_MODE_OFF);

  // Add tiles: boot screen at 0, then regular tiles at 1..2, and tile 3 (7-day forecast)
  t_boot = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_HOR);
  t1 = lv_tileview_add_tile(tileview, 1, 0, LV_DIR_HOR);
  t2 = lv_tileview_add_tile(tileview, 2, 0, LV_DIR_HOR);
  t3 = lv_tileview_add_tile(tileview, 3, 0, LV_DIR_HOR);
  // Tile #4 (Weather History)
  lv_obj_t* t4 = lv_tileview_add_tile(tileview, 4, 0, LV_DIR_HOR);

    // Boot tile (shows first for a short time)
  {
    t_boot_label = lv_label_create(t_boot);
    lv_label_set_text(t_boot_label, "Group 5 v.3.0");
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
    lv_dropdown_set_selected(t2_dropdown, clamp_param_index(t2_selected_index));
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
      // Parameter changes only affect tile #2 and history
      extern void history_timer_cb(lv_timer_t* t);
      history_timer_cb(NULL);
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // City dropdown controls the displayed location
    t2_dropdown_cities = lv_dropdown_create(t2);
    const char* dd_opts_city = "Karlskrona\nStockholm\nGothenburg\nKiruna\nMalmo";
    lv_dropdown_set_options_static(t2_dropdown_cities, dd_opts_city);
    lv_dropdown_set_selected(t2_dropdown_cities, clamp_city_index(t2_selected_city_index));  // Default to Karlskrona
    lv_obj_set_width(t2_dropdown_cities, 200);
    lv_obj_align(t2_dropdown_cities, LV_ALIGN_TOP_LEFT, 0, 20);

    lv_obj_add_event_cb(t2_dropdown_cities, [](lv_event_t* e){
      lv_obj_t* dd = lv_event_get_target(e);
      t2_selected_city_index = lv_dropdown_get_selected(dd);
      // Direct call to update immediately
      extern void update_t2_param_display();
      update_t2_param_display();  // Also update the parameter display for the new city
      // Also fetch an updated forecast for the newly selected city (if WiFi is ready)
      forecast_timer_cb(NULL);  // Trigger immediate forecast update
      // Also update history display (tile #4) for the new city
      extern void history_timer_cb(lv_timer_t* t);
      history_timer_cb(NULL);
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // Reset button: restores saved parameter and city selection
    lv_obj_t* reset_btn = lv_btn_create(t2);
    lv_obj_set_size(reset_btn, 120, 40);
    lv_obj_align(reset_btn, LV_ALIGN_TOP_RIGHT, -10, 18);
    lv_obj_t* reset_lbl = lv_label_create(reset_btn);
    lv_label_set_text(reset_lbl, "Reset");
    lv_obj_center(reset_lbl);
    lv_obj_add_event_cb(reset_btn, [](lv_event_t* e){
      LV_UNUSED(e);
      uint16_t target_param = has_saved_selection ? saved_param_index : 0;
      uint16_t target_city = has_saved_selection ? saved_city_index : 0;
      target_param = clamp_param_index(target_param);
      target_city = clamp_city_index(target_city);

      t2_selected_index = target_param;
      t2_selected_city_index = target_city;

      if (t2_dropdown) lv_dropdown_set_selected(t2_dropdown, target_param);
      if (t2_dropdown_cities) lv_dropdown_set_selected(t2_dropdown_cities, target_city);

      update_t2_param_display();
      forecast_timer_cb(NULL);
      extern void history_timer_cb(lv_timer_t* t);
      history_timer_cb(NULL);
    }, LV_EVENT_CLICKED, NULL);

    // Save button: stores current selections for future resets/boots
    lv_obj_t* save_btn = lv_btn_create(t2);
    lv_obj_set_size(save_btn, 120, 40);
    lv_obj_align_to(save_btn, reset_btn, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_t* save_lbl = lv_label_create(save_btn);
    lv_label_set_text(save_lbl, "Save");
    lv_obj_center(save_lbl);
    lv_obj_add_event_cb(save_btn, [](lv_event_t* e){
      LV_UNUSED(e);
      save_current_selection();
    }, LV_EVENT_CLICKED, NULL);
  }

  // Tile #3 - 7-Day Weather Forecast Graph
  {
    // Title label at top
    t3_title_label = lv_label_create(t3);
    lv_label_set_text(t3_title_label, "Loading forecast...");
    lv_obj_set_style_text_font(t3_title_label, &lv_font_montserrat_16, 0);
    lv_obj_align(t3_title_label, LV_ALIGN_TOP_MID, 0, 10);
    
    // Create chart
    t3_chart = lv_chart_create(t3);
    lv_obj_set_size(t3_chart, lv_disp_get_hor_res(NULL) - 60, lv_disp_get_ver_res(NULL) - 100);
    lv_obj_align(t3_chart, LV_ALIGN_CENTER, 10, 0);
    lv_chart_set_type(t3_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(t3_chart, 7);
    lv_chart_set_div_line_count(t3_chart, 5, 7);
    
    // Enable Y-axis tick labels to show values
    lv_chart_set_axis_tick(t3_chart, LV_CHART_AXIS_PRIMARY_Y, 5, 3, 6, 1, true, 50);
    
    // Add two series: min and max values
    t3_series_min = lv_chart_add_series(t3_chart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);
    t3_series_max = lv_chart_add_series(t3_chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    
    // Create X-axis date labels below the chart (will be positioned dynamically)
    int chart_bottom_y = (lv_disp_get_ver_res(NULL) / 2) + (lv_disp_get_ver_res(NULL) - 100) / 2;
    
    for (int i = 0; i < 7; i++) {
      // Create symbol code labels above the date labels (below the chart)
      t3_symbol_labels[i] = lv_label_create(t3);
      lv_label_set_text(t3_symbol_labels[i], "");
      lv_obj_set_style_text_font(t3_symbol_labels[i], &lv_font_montserrat_12, 0);
      lv_obj_set_pos(t3_symbol_labels[i], 0, chart_bottom_y - 18);

      t3_symbol_images[i] = lv_img_create(t3);
      lv_obj_set_style_bg_color(t3_symbol_images[i], lv_color_white(), 0);
      lv_obj_set_style_bg_opa(t3_symbol_images[i], LV_OPA_COVER, 0);
      lv_img_set_src(t3_symbol_images[i], &cloud_50dp_E3E3E3_FILL0_wght400_GRAD0_opsz48);
      lv_obj_add_flag(t3_symbol_images[i], LV_OBJ_FLAG_HIDDEN);
      
      t3_date_labels[i] = lv_label_create(t3);
      lv_label_set_text(t3_date_labels[i], "--");
      lv_obj_set_style_text_font(t3_date_labels[i], &lv_font_montserrat_12, 0);
      lv_obj_set_pos(t3_date_labels[i], 0, chart_bottom_y + 5);
    }
    
    // Add refresh button in top-right corner
    lv_obj_t* refresh_btn = lv_btn_create(t3);
    lv_obj_set_size(refresh_btn, 80, 35);
    lv_obj_align(refresh_btn, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_t* refresh_lbl = lv_label_create(refresh_btn);
    lv_label_set_text(refresh_lbl, LV_SYMBOL_REFRESH " Refresh");
    lv_obj_set_style_text_font(refresh_lbl, &lv_font_montserrat_12, 0);
    lv_obj_center(refresh_lbl);
    lv_obj_add_event_cb(refresh_btn, [](lv_event_t* e){
      LV_UNUSED(e);
      // Trigger immediate forecast update
      forecast_timer_cb(NULL);
    }, LV_EVENT_CLICKED, NULL);
  }

  // Tile #4 - Weather History Graph (last 30 days, showing 7 at a time)
  {
    // Title label at top
    t4_title_label = lv_label_create(t4);
    lv_label_set_text(t4_title_label, "Loading history...");
    lv_obj_set_style_text_font(t4_title_label, &lv_font_montserrat_16, 0);
    lv_obj_align(t4_title_label, LV_ALIGN_TOP_MID, 0, 10);

    // Create vertical slider on the right side
    t4_slider = lv_slider_create(t4);
    lv_obj_set_size(t4_slider, 20, lv_disp_get_ver_res(NULL) - 120);  // Vertical slider
    lv_obj_align(t4_slider, LV_ALIGN_RIGHT_MID, -5, 0);
    lv_slider_set_range(t4_slider, 0, 1);  // Avoid division by zero before data loads
    lv_slider_set_value(t4_slider, 0, LV_ANIM_OFF);
    lv_obj_add_state(t4_slider, LV_STATE_DISABLED);  // Enable once data arrives
    
    // Slider event callback to update displayed window
    lv_obj_add_event_cb(t4_slider, [](lv_event_t* e){
      LV_UNUSED(e);
      extern void update_t4_display_window();
      update_t4_display_window();
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // Create chart (showing 7 days at a time)
    t4_chart = lv_chart_create(t4);
    lv_obj_set_size(t4_chart, lv_disp_get_hor_res(NULL) - 100, lv_disp_get_ver_res(NULL) - 100);
    lv_obj_align(t4_chart, LV_ALIGN_CENTER, -10, 0);
    lv_chart_set_type(t4_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(t4_chart, T4_WINDOW_DAYS); // Show configured window
    lv_chart_set_div_line_count(t4_chart, 5, 7);
    lv_chart_set_axis_tick(t4_chart, LV_CHART_AXIS_PRIMARY_Y, 5, 3, 6, 1, true, 50);

    // Add one series for daily average temperature
    t4_series = lv_chart_add_series(t4_chart, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);

    // Create X-axis date labels below the chart (7 visible days)
    int chart_width = lv_disp_get_hor_res(NULL) - 100;
    int label_spacing = chart_width / T4_WINDOW_DAYS;
    int chart_bottom_y = (lv_disp_get_ver_res(NULL) / 2) + (lv_disp_get_ver_res(NULL) - 100) / 2;
    for (int i = 0; i < T4_WINDOW_DAYS; i++) {
      t4_date_labels[i] = lv_label_create(t4);
      lv_label_set_text(t4_date_labels[i], "--");
      lv_obj_set_style_text_font(t4_date_labels[i], &lv_font_montserrat_12, 0);
      int x_pos = (i * label_spacing) + (label_spacing / 2) - 10;
      lv_obj_set_pos(t4_date_labels[i], x_pos - 12, chart_bottom_y + 5);
    }
    
    // Initialize full history storage
    t4_full_days = 0;
    t4_window_start = 0;
    for (int i=0; i<T4_HISTORY_MAX_DAYS; i++) {
      t4_full_history[i].date = "";
      t4_full_history[i].avg = NAN;
      t4_full_history[i].valid = false;
    }

    // Add refresh button in top-right corner
    lv_obj_t* refresh_btn4 = lv_btn_create(t4);
    lv_obj_set_size(refresh_btn4, 80, 35);
    lv_obj_align(refresh_btn4, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_t* refresh_lbl4 = lv_label_create(refresh_btn4);
    lv_label_set_text(refresh_lbl4, LV_SYMBOL_REFRESH " Refresh");
    lv_obj_set_style_text_font(refresh_lbl4, &lv_font_montserrat_12, 0);
    lv_obj_center(refresh_lbl4);
    // Refresh callback triggers history fetch
    lv_obj_add_event_cb(refresh_btn4, [](lv_event_t* e){
      LV_UNUSED(e);
      // Trigger immediate history update
      extern void history_timer_cb(lv_timer_t* t);
      history_timer_cb(NULL);
    }, LV_EVENT_CLICKED, NULL);
  }

  // Set tileview to start at boot tile (0,0) after all tiles are created
  lv_obj_set_tile_id(tileview, 0, 0, LV_ANIM_OFF);

   // Create a timer to refresh WiFi indicator on tile #1 every 1s
  lv_timer_create(wifi_indicator_timer_cb, 1000, NULL);
  // Create a timer to refresh the selected parameter on tile #2 every 10s
  lv_timer_create([](lv_timer_t* t){
    (void)t;
    update_t2_param_display();
  }, 10000, NULL);
  // Create a timer to refresh 7-day forecast every 30 seconds (30000ms)
  lv_timer_create(forecast_timer_cb, 30000, NULL);
  // Trigger first forecast fetch after 10 seconds to allow WiFi to connect
  lv_timer_create([](lv_timer_t* t){ 
    forecast_timer_cb(t); 
    lv_timer_del(t); 
  }, 10000, NULL);
  
  }

// Forecast UI and update functions removed (tile #4)

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

  prefs_ready = preferences.begin("weapp", false);
  if (!prefs_ready) {
    Serial.println("Failed to initialize preferences storage.");
    t2_selected_index = 0;
    t2_selected_city_index = 0;
    has_saved_selection = false;
  } else {
    load_saved_selection();
  }

  // Start LVGL helper and create UI
  beginLvglHelper(amoled, /*debug=*/true);
  create_ui();
  // Start non-blocking WiFi connection so the boot screen shows immediately
  initWiFi();
  // Start boot timer
  boot_start_ms = millis();
    // Forecast fetch removed (tile #4 removed)
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

// Timer callback: update weather history on tile #4
// Helper function to update the 7-day display window based on slider position
void update_t4_display_window()
{
  if (!t4_chart || !t4_slider || t4_full_days <= 0) return;

  // Get slider value (0-23)
  t4_window_start = lv_slider_get_value(t4_slider);
  
  // Ensure we don't go beyond available data
  if (t4_window_start + T4_WINDOW_DAYS > t4_full_days) {
    t4_window_start = (t4_full_days > T4_WINDOW_DAYS) ? (t4_full_days - T4_WINDOW_DAYS) : 0;
  }

  // Calculate range for the visible window
  float windowMin = 999999;
  float windowMax = -999999;
  for (int i = t4_window_start; i < t4_window_start + T4_WINDOW_DAYS && i < t4_full_days; i++) {
    if (t4_full_history[i].valid && !isnan(t4_full_history[i].avg)) {
      if (t4_full_history[i].avg < windowMin) windowMin = t4_full_history[i].avg;
      if (t4_full_history[i].avg > windowMax) windowMax = t4_full_history[i].avg;
    }
  }
  
  if (windowMin < 999999 && windowMax > -999999) {
    float range = windowMax - windowMin;
    int yMin = (int)(windowMin - range * 0.1);
    int yMax = (int)(windowMax + range * 0.1);
    lv_chart_set_range(t4_chart, LV_CHART_AXIS_PRIMARY_Y, yMin, yMax);
  }

  // Update chart with 7-day window
  lv_chart_set_all_value(t4_chart, t4_series, LV_CHART_POINT_NONE);
  
  for (int i = 0; i < T4_WINDOW_DAYS; i++) {
    int dataIdx = t4_window_start + i;
    if (dataIdx < t4_full_days) {
      int val = LV_CHART_POINT_NONE;
      if (t4_full_history[dataIdx].valid && !isnan(t4_full_history[dataIdx].avg)) {
        val = (int)t4_full_history[dataIdx].avg;
      }
      lv_chart_set_next_value(t4_chart, t4_series, val);
      
      // Update date label
      if (t4_date_labels[i]) {
        if (t4_full_history[dataIdx].valid && t4_full_history[dataIdx].date.length() >= 10) {
          String d = t4_full_history[dataIdx].date.substring(5); // MM-DD
          lv_label_set_text(t4_date_labels[i], d.c_str());
        } else {
          lv_label_set_text(t4_date_labels[i], "");
        }
      }
    } else {
      // No more data
      lv_chart_set_next_value(t4_chart, t4_series, LV_CHART_POINT_NONE);
      if (t4_date_labels[i]) {
        lv_label_set_text(t4_date_labels[i], "");
      }
    }
  }

  lv_chart_refresh(t4_chart);
}

void history_timer_cb(lv_timer_t* timer)
{
  LV_UNUSED(timer);
  if (!t4_chart || !t4_title_label) return;

  if (WiFi.status() != WL_CONNECTED) {
    lv_label_set_text(t4_title_label, "Waiting for WiFi...");
    return;
  }

  lv_label_set_text(t4_title_label, "Fetching history...");

  // Determine station based on selected city index
  const char* cityName = "Unknown";
  switch(t2_selected_city_index) {
    case 0: cityName = "Karlskrona"; break;
    case 1: cityName = "Stockholm"; break;
    case 2: cityName = "Gothenburg"; break;
    case 3: cityName = "Kiruna"; break;
    case 4: cityName = "Malmo"; break;
  }

  int station = change_station_nr(cityName);
  if (station < 0) {
    lv_label_set_text(t4_title_label, "Unknown station");
    return;
  }

  // Map t2_selected_index to SMHI parameter ID
  int parameter_id = 1; // Default: Temperature
  switch(t2_selected_index) {
    case 0: parameter_id = 1; break;  // Temperature
    case 1: parameter_id = 4; break;  // Wind Speed
    case 2: parameter_id = 7; break;  // Precipitation
    case 3: parameter_id = 6; break;  // Humidity
    case 4: parameter_id = 9; break;  // Pressure
  }

  // Fetch history (daily averages) for up to 5 months
  for (int i=0;i<T4_HISTORY_MAX_DAYS;i++) { t4_history_buffer[i].date=""; t4_history_buffer[i].avg = NAN; t4_history_buffer[i].valid=false; }
  int outDays = 0;
  String statusMsg;

  if (!getWeatherHistory(t4_history_buffer, T4_HISTORY_MAX_DAYS, outDays, statusMsg, station, parameter_id)) {
    char buf[64];
    snprintf(buf, sizeof(buf), "Err: %s", statusMsg.c_str());
    lv_label_set_text(t4_title_label, buf);
    return;
  }

  if (outDays <= 0) {
    lv_label_set_text(t4_title_label, "No history data");
    return;
  }

  // Store full history data in REVERSE order (most recent first)
  t4_full_days = outDays;
  for (int i = 0; i < outDays && i < T4_HISTORY_MAX_DAYS; i++) {
    t4_full_history[i] = t4_history_buffer[outDays - 1 - i];  // Reverse order
  }

  // Reset window to start (most recent data)
  t4_window_start = 0;
  if (t4_slider) {
    int maxStart = (t4_full_days > T4_WINDOW_DAYS) ? (t4_full_days - T4_WINDOW_DAYS) : 0;
    int sliderMax = (maxStart > 0) ? maxStart : 1;
    lv_slider_set_range(t4_slider, 0, sliderMax);
    lv_slider_set_value(t4_slider, 0, LV_ANIM_OFF);
    if (maxStart > 0) {
      lv_obj_clear_state(t4_slider, LV_STATE_DISABLED);
    } else {
      lv_obj_add_state(t4_slider, LV_STATE_DISABLED);
    }
  }

  // Update the display with the initial 7-day window
  update_t4_display_window();
  
  // Build title with city name and parameter name
  String paramName;
  String unit;
  switch(t2_selected_index) {
    case 0: paramName = "Temperature"; unit = "°C"; break;
    case 1: paramName = "Wind Speed"; unit = "m/s"; break;
    case 2: paramName = "Precipitation"; unit = "mm"; break;
    case 3: paramName = "Humidity"; unit = "%"; break;
    case 4: paramName = "Pressure"; unit = "hPa"; break;
    default: paramName = "Weather"; unit = ""; break;
  }
  
  char titleBuf[100];
  snprintf(titleBuf, sizeof(titleBuf), "History in %s - %s (%s)", cityName, paramName.c_str(), unit.c_str());
  lv_label_set_text(t4_title_label, titleBuf);
}