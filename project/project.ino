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
#include <WiFiClientSecure.h>

LilyGo_Class amoled;

static lv_obj_t* tileview;
static lv_obj_t* t1;
static lv_obj_t* t2;
static lv_obj_t* t3;
static lv_obj_t* t1_label;
static lv_obj_t* t2_label;
static lv_obj_t* t3_label;
static lv_obj_t* t_weather_label; // ADDED: label to show weather
static bool t2_dark = false;  // start tile #2 in light mode
static bool t3_change = false;  // start tile #3 in light mode
static unsigned long boot_start_ms = 0;
static bool boot_switched = false;
// ADDED: SMHI config & timers
static const char* SMHI_BASE = "https://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/geotype/point";
static const float WEATHER_LAT = 59.3293;   // fill with your latitude
static const float WEATHER_LON = 18.0686;   // fill with your longitude
static unsigned long weather_last_ms = 0;
static const unsigned long WEATHER_UPDATE_INTERVAL_MS = 10 * 60 * 1000UL; // 10 minutes

// Function: Tile #2 Color change
static void apply_tile_colors(lv_obj_t* tile, lv_obj_t* label, bool dark)
{
  // Background
  lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(tile, dark ? lv_color_black() : lv_color_white(), 0);

  // Text
  lv_obj_set_style_text_color(label, dark ? lv_color_white() : lv_color_black(), 0);
}

static void on_tile2_clicked(lv_event_t* e)
{
  LV_UNUSED(e);
  t2_dark = !t2_dark;
  apply_tile_colors(t2, t2_label, t2_dark);
}

//Funktion: TIle #3 Color change
static void on_tile3_clicked(lv_event_t*e){
  LV_UNUSED(e);
  t3_change = !t3_change;
  apply_tile_colors(t3, t3_label, t3_change);

  if(t3_change == false){
    lv_obj_set_style_bg_color(t3, lv_color_hex(0x0000FF), 0); // blue background
    lv_obj_set_style_text_color(t3_label, lv_color_hex(0xFFC0CB), 0); // pink text
  }
  else if(t3_change == true){
    lv_obj_set_style_bg_color(t3, lv_color_hex(0xFFC0CB), 0); // pink background
    lv_obj_set_style_text_color(t3_label, lv_color_hex(0x0000FF), 0); // blue text
  }
}

// Timer callback: update WiFi status on tile #3 every second
static void wifi_status_timer_cb(lv_timer_t* timer)
{
  LV_UNUSED(timer);
  if (!t3_label) return;
  // Show short connected/disconnected status per user request
  if (WiFi.status() == WL_CONNECTED) {
    lv_label_set_text(t3_label, "Connected");
    lv_obj_set_style_text_color(t3_label, lv_color_hex(0x0000FF), 0);
  } else {
    lv_label_set_text(t3_label, "Disconnected");
    lv_obj_set_style_text_color(t3_label, lv_color_hex(0x0000FF), 0);
  }
}

// Function: Creates UI
static void create_ui()
{
  // Fullscreen Tileview
  tileview = lv_tileview_create(lv_scr_act());
  lv_obj_set_size(tileview, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
  lv_obj_set_scrollbar_mode(tileview, LV_SCROLLBAR_MODE_OFF);

  // Add two horizontal tiles
  t1 = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_HOR);
  t2 = lv_tileview_add_tile(tileview, 1, 0, LV_DIR_HOR);
  // Tile #3
  t3 = lv_tileview_add_tile(tileview, 2, 0, LV_DIR_HOR);

  // Tile #1
  {
    t1_label = lv_label_create(t1);
    lv_label_set_text(t1_label, "GROUP 5, VERSION 0.1");
    lv_obj_set_style_text_font(t1_label, &lv_font_montserrat_28, 0);
    lv_obj_center(t1_label);
    apply_tile_colors(t1, t1_label, /*dark=*/false);
  }

  // Tile #2
  {
    t2_label = lv_label_create(t2);
    lv_label_set_text(t2_label, "Hello World! 2");
    lv_obj_set_style_text_font(t2_label, &lv_font_montserrat_28, 0);
    lv_obj_center(t2_label);

    apply_tile_colors(t2, t2_label, /*dark=*/false);
    lv_obj_add_flag(t2, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(t2, on_tile2_clicked, LV_EVENT_CLICKED, NULL);

    // Start hidden so t_boot shows first; we'll reveal t2 after 5s
    lv_obj_add_flag(t2, LV_OBJ_FLAG_HIDDEN);

    // ADDED: weather label under the main t2 label
    t_weather_label = lv_label_create(t2);
    lv_label_set_text(t_weather_label, "Weather: --");
    lv_obj_set_style_text_font(t_weather_label, &lv_font_montserrat_14, 0);
    lv_obj_align(t_weather_label, LV_ALIGN_CENTER, 0, 40);
  }

  // Tile #3
  {
    t3_label = lv_label_create(t3);
    lv_label_set_text(t3_label, "Hello World! 3");
    lv_obj_set_style_text_font(t3_label, &lv_font_montserrat_28, 0);
    lv_obj_center(t3_label);

    // Pink background and blue text for tile #3
    lv_obj_set_style_bg_opa(t3, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(t3, lv_color_hex(0xFFC0CB), 0); // pink
    lv_obj_set_style_text_color(t3_label, lv_color_hex(0x0000FF), 0); // blue

    lv_obj_add_flag(t3, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(t3, on_tile3_clicked, LV_EVENT_CLICKED, NULL);
  }
  // Create a timer to refresh WiFi status every 1s
  lv_timer_create(wifi_status_timer_cb, 1000, NULL);
  
}

// Function: Connects to WIFI
static void connect_wifi()
{
  Serial.printf("Connecting to WiFi SSID: %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < 15000) {
    delay(250);
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected.");
  } else {
    Serial.println("WiFi could not connect (timeout).");
  }
}

// ADDED: Fetch SMHI point forecast (simple: gets first timeSeries entry)
static void fetch_weather()
{
  if (WiFi.status() != WL_CONNECTED) return;

  // Build URL: .../geotype/point/lon/{lon}/lat/{lat}/data.json
  char url[256];
  snprintf(url, sizeof(url), "%s/lon/%.6f/lat/%.6f/data.json", SMHI_BASE, WEATHER_LON, WEATHER_LAT);

  WiFiClientSecure *client = new WiFiClientSecure();
  client->setInsecure(); // skip cert validation (simplest for ESP32). For production, validate cert.
  HTTPClient https;
  if (!https.begin(*client, url)) {
    Serial.println("Failed to begin HTTPS");
    delete client;
    return;
  }

  int httpCode = https.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("SMHI HTTP error: %d\n", httpCode);
    https.end();
    delete client;
    return;
  }

  String payload = https.getString();
  https.end();
  delete client;

  // Parse JSON (adjust size if necessary)
  const size_t cap = 32 * 1024;
  DynamicJsonDocument doc(cap);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.print("JSON parse error: ");
    Serial.println(err.c_str());
    return;
  }

  // timeSeries -> first entry -> parameters array -> find "t" and "Wsymb2"
  JsonArray ts = doc["timeSeries"].as<JsonArray>();
  if (ts.size() == 0) return;
  JsonObject first = ts[0].as<JsonObject>();
  JsonArray params = first["parameters"].as<JsonArray>();
  float temp = NAN;
  int symb = -1;
  for (JsonObject p : params) {
    const char* name = p["name"];
    if (strcmp(name, "t") == 0) {
      temp = p["values"][0].as<float>();
    } else if (strcmp(name, "Wsymb2") == 0) {
      symb = p["values"][0].as<int>();
    }
  }

  char buf[64];
  if (!isnan(temp)) {
    if (symb >= 0) {
      snprintf(buf, sizeof(buf), "Temp: %.1f C, Wsymb:%d", temp, symb);
    } else {
      snprintf(buf, sizeof(buf), "Temp: %.1f C", temp);
    }
  } else {
    snprintf(buf, sizeof(buf), "Weather: unavailable");
  }

  if (t_weather_label) lv_label_set_text(t_weather_label, buf);
  Serial.printf("Weather updated: %s\n", buf);
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
  // Try to connect WiFi first so initial status is available on the tile
  connect_wifi();
  create_ui();
  boot_start_ms = millis();

  // ADDED: initial weather fetch (if online)
  if (WiFi.status() == WL_CONNECTED) {
    fetch_weather();
    weather_last_ms = millis();
  }
}

// Must have function: Loop runs continously on device after setup
void loop()
{
  /* Let LVGL do its work. Call the timer handler frequently. */
  lv_timer_handler();
  delay(5);
  if (!boot_switched && boot_start_ms != 0 && (millis() - boot_start_ms) >= 5000) {
    boot_switched = true;
    // Fixed: switch tileview to tile 1 (column 1, row 0) instead of hiding undefined t_boot
    lv_tileview_set_tile_act(tileview, 1, 0, LV_ANIM_ON);
    lv_obj_clear_flag(t2, LV_OBJ_FLAG_HIDDEN);     // ensure t2 visible
  }

  // Periodic weather updates
  if ((millis() - weather_last_ms) >= WEATHER_UPDATE_INTERVAL_MS) {
    weather_last_ms = millis();
    if (WiFi.status() == WL_CONNECTED) fetch_weather();
  }
}