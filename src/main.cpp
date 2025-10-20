#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port to connect (for native USB boards)
  }
  Serial.println("Hello, World from LilyGO!");
}

void loop() {
  // Nothing here — just idle
}