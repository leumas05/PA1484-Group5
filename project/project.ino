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

LilyGo_Class amoled;

static lv_obj_t* tileview;
static lv_obj_t* t1;
static lv_obj_t* t_boot;
static lv_obj_t* t2;
static lv_obj_t* t3;
static lv_obj_t* t1_label;
static lv_obj_t* t_boot_label;
static lv_obj_t* t2_label;
static lv_obj_t* t3_label;
static bool t2_dark = false;  // start tile #2 in light mode
static bool t3_change = false;  // start tile #3 in light mode
static unsigned long boot_start_ms = 0;
static bool boot_switched = false;

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

  // Add tiles: boot screen at 0, then regular tiles at 1..3
  t_boot = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_HOR);
  t1 = lv_tileview_add_tile(tileview, 1, 0, LV_DIR_HOR);
  t2 = lv_tileview_add_tile(tileview, 2, 0, LV_DIR_HOR);
  // Tile #3
  t3 = lv_tileview_add_tile(tileview, 3, 0, LV_DIR_HOR);

  // Tile #1
  {
    t1_label = lv_label_create(t1);
    lv_label_set_text(t1_label, "Hello World!");
    lv_obj_set_style_text_font(t1_label, &lv_font_montserrat_28, 0);
    lv_obj_center(t1_label);
    apply_tile_colors(t1, t1_label, /*dark=*/false);
  }

  // Boot tile (shows first for a short time)
  {
    t_boot_label = lv_label_create(t_boot);
    lv_label_set_text(t_boot_label, "Grupp 5 v.0.1");
    lv_obj_set_style_text_font(t_boot_label, &lv_font_montserrat_28, 0);
    lv_obj_center(t_boot_label);
    apply_tile_colors(t_boot, t_boot_label, /*dark=*/false);
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
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  // Start boot timer
  boot_start_ms = millis();
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