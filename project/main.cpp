#include <Arduino.h>
#include <TFT_eSPI.h>  // Display driver
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();  // Create TFT instance

void setup() {
  Serial.begin(115200);
  delay(500);  // give serial monitor time to open

  Serial.println("Hello, World from LilyGO T-Display AMOLED!");

  // Initialize the display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("Hello, World!", 30, 50);

  // Optional: draw some graphics
  tft.drawRect(20, 40, 200, 60, TFT_GREEN);
}

void loop() {
  // Nothing — just idle
}