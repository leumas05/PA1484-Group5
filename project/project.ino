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
#include "weather_forecast_api.h"
#include "weather_history_api.h"
#include "wifi_manager.h"
//#include "sun_symbol.c"
//LV_IMG_DECLARE(sun_symbol);


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
// Tile #4 globals (history)
static lv_obj_t* t4_chart;
static lv_chart_series_t* t4_series;
static lv_obj_t* t4_title_label;
static lv_obj_t* t4_date_labels[7];  // X-axis date labels for visible window (7 days)
static lv_obj_t* t4_slider;  // Vertical slider to select 7-day window
static int t4_point_count = 0;
static HistoryPoint t4_full_history[31];  // Store full month of data
static int t4_full_days = 0;  // Number of valid days in full history
static int t4_window_start = 0;  // Starting index of 7-day window (0-23)
static unsigned long boot_start_ms = 0;
static bool boot_switched = false;
static float lastTemperature = NAN;

static const char* get_symbol_description(int code)
{
  switch (code) {
    case 0: return "no data";
    case 1: return "clear sky";
    case 2: return "nearly clear";
    case 3: return "var cloud";
    case 4: return "half clear";
    case 5: return "cloudy";
    case 6: return "overcast";
    case 7: return "fog";
    case 8: return "light rain sh";
    case 9: return "mod rain sh";
    case 10: return "heavy rain sh";
    case 11: return "thunderstorm";
    case 12: return "light sleet sh";
    case 13: return "mod sleet sh";
    case 14: return "heavy sleet sh";
    case 15: return "light snow sh";
    case 16: return "mod snow sh";
    case 17: return "heavy snow sh";
    case 18: return "light rain";
    case 19: return "mod rain";
    case 20: return "heavy rain";
    case 21: return "thunder";
    case 22: return "light sleet";
    case 23: return "mod sleet";
    case 24: return "heavy sleet";
    case 25: return "light snow";
    case 26: return "mod snow";
    case 27: return "heavy snow";
    default: return "unknown";
  }
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
  
    if (getWeatherForecast(forecast, statusMsg, lat, lon, t2_selected_index)) {
    // Build title based on selected parameter
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
        float minVal = NAN;
        float maxVal = NAN;
        
        switch(t2_selected_index) {
          case 0: // Temperature
            minVal = forecast[i].minTemp;
            maxVal = forecast[i].maxTemp;
            break;
          case 1: // Wind Speed
            if (!isnan(forecast[i].minWind)) {
              minVal = forecast[i].minWind;
              maxVal = forecast[i].maxWind;
            }
            break;
          case 2: // Rain
            if (!isnan(forecast[i].totalRain)) {
              minVal = forecast[i].totalRain;
              maxVal = forecast[i].totalRain;
            }
            break;
          case 3: // Humidity
            if (!isnan(forecast[i].minHumidity)) {
              minVal = forecast[i].minHumidity;
              maxVal = forecast[i].maxHumidity;
            }
            break;
          case 4: // Pressure
            if (!isnan(forecast[i].minPressure)) {
              minVal = forecast[i].minPressure;
              maxVal = forecast[i].maxPressure;
            }
            break;
        }
        
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
    int chart_y = lv_obj_get_y(t3_chart);
    int chart_width = lv_obj_get_width(t3_chart);
    int chart_height = lv_obj_get_height(t3_chart);
    int label_spacing = chart_width / 7;
    
    // Second pass: update chart with forecast data and position symbol labels
    for (int i = 0; i < 7; i++) {
      if (forecast[i].valid) {
        int minVal = LV_CHART_POINT_NONE;
        int maxVal = LV_CHART_POINT_NONE;
        float dataValue = 0;
        
        switch(t2_selected_index) {
          case 0: // Temperature - use average for cleaner single line
            {
              float avgTemp = (forecast[i].minTemp + forecast[i].maxTemp) / 2.0;
              minVal = (int)avgTemp;
              maxVal = (int)avgTemp;
              dataValue = avgTemp;
            }
            break;
          case 1: // Wind Speed
            if (!isnan(forecast[i].minWind)) {
              minVal = (int)forecast[i].minWind;
              maxVal = (int)forecast[i].maxWind;
              dataValue = (forecast[i].minWind + forecast[i].maxWind) / 2.0;
            }
            break;
          case 2: // Rain
            if (!isnan(forecast[i].totalRain)) {
              minVal = (int)forecast[i].totalRain;
              maxVal = (int)forecast[i].totalRain;
              dataValue = forecast[i].totalRain;
            }
            break;
          case 3: // Humidity
            if (!isnan(forecast[i].minHumidity)) {
              minVal = (int)forecast[i].minHumidity;
              maxVal = (int)forecast[i].maxHumidity;
              dataValue = (forecast[i].minHumidity + forecast[i].maxHumidity) / 2.0;
            }
            break;
          case 4: // Pressure
            if (!isnan(forecast[i].minPressure)) {
              minVal = (int)forecast[i].minPressure;
              maxVal = (int)forecast[i].maxPressure;
              dataValue = (forecast[i].minPressure + forecast[i].maxPressure) / 2.0;
            }
            break;
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
      if (t3_symbol_labels[i]) {
        if (forecast[i].valid && forecast[i].symbolCode >= 0) {
          // Get the date label's Y position and place symbol code above it
          int date_y = lv_obj_get_y(t3_date_labels[i]);
          lv_obj_set_pos(t3_symbol_labels[i], x_center - 8, date_y - 40);
          
          const char* meaning = get_symbol_description(forecast[i].symbolCode);
          lv_label_set_text(t3_symbol_labels[i], meaning);
        } else {
          lv_label_set_text(t3_symbol_labels[i], "");
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
      // Also update the forecast display for the new parameter
      forecast_timer_cb(NULL);
      // Also update history display (tile #4) for the new parameter
      extern void history_timer_cb(lv_timer_t* t);
      history_timer_cb(NULL);
    }, LV_EVENT_VALUE_CHANGED, NULL);

    t2_dropdown_cities = lv_dropdown_create(t2);
    const char* dd_opts_city = "Karlskrona\nStockholm\nGothenburg\nKiruna\nMalmo";
    lv_dropdown_set_options_static(t2_dropdown_cities, dd_opts_city);
    lv_dropdown_set_selected(t2_dropdown_cities, 0);  // Default to Karlskrona
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
        // Refresh forecast and history after reset
        forecast_timer_cb(NULL);
        extern void history_timer_cb(lv_timer_t* t);
        history_timer_cb(NULL);
        
      }, LV_EVENT_CLICKED, NULL);
    }
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
    lv_slider_set_range(t4_slider, 0, 23);  // 0-23 allows 7-day window (0-6 to 23-30)
    lv_slider_set_value(t4_slider, 0, LV_ANIM_OFF);
    
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
    lv_chart_set_point_count(t4_chart, 7); // Show 7 days
    lv_chart_set_div_line_count(t4_chart, 5, 7);
    lv_chart_set_axis_tick(t4_chart, LV_CHART_AXIS_PRIMARY_Y, 5, 3, 6, 1, true, 50);

    // Add one series for daily average temperature
    t4_series = lv_chart_add_series(t4_chart, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);

    // Create X-axis date labels below the chart (7 visible days)
    int chart_width = lv_disp_get_hor_res(NULL) - 100;
    int label_spacing = chart_width / 7;
    int chart_bottom_y = (lv_disp_get_ver_res(NULL) / 2) + (lv_disp_get_ver_res(NULL) - 100) / 2;
    for (int i = 0; i < 7; i++) {
      t4_date_labels[i] = lv_label_create(t4);
      lv_label_set_text(t4_date_labels[i], "--");
      lv_obj_set_style_text_font(t4_date_labels[i], &lv_font_montserrat_12, 0);
      int x_pos = (i * label_spacing) + (label_spacing / 2) - 10;
      lv_obj_set_pos(t4_date_labels[i], x_pos - 12, chart_bottom_y + 5);
    }
    
    // Initialize full history storage
    t4_full_days = 0;
    t4_window_start = 0;
    for (int i=0; i<31; i++) {
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
  if (t4_window_start + 7 > t4_full_days) {
    t4_window_start = (t4_full_days > 7) ? (t4_full_days - 7) : 0;
  }

  // Calculate range for the visible window
  float windowMin = 999999;
  float windowMax = -999999;
  for (int i = t4_window_start; i < t4_window_start + 7 && i < t4_full_days; i++) {
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
  
  for (int i = 0; i < 7; i++) {
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

  // Fetch history (daily averages) for up to 30 days
  HistoryPoint history[31];
  for (int i=0;i<31;i++) { history[i].date=""; history[i].avg = NAN; history[i].valid=false; }
  int outDays = 0;
  String statusMsg;

  if (!getWeatherHistory((HistoryPoint*)history, 30, outDays, statusMsg, station, parameter_id)) {
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
  for (int i = 0; i < outDays && i < 31; i++) {
    t4_full_history[i] = history[outDays - 1 - i];  // Reverse order
  }

  // Reset window to start (most recent data)
  t4_window_start = 0;
  if (t4_slider) {
    lv_slider_set_value(t4_slider, 0, LV_ANIM_OFF);
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