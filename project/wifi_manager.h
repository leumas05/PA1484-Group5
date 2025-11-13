#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

// Initialize WiFi connection (non-blocking)
void initWiFi();

// Connect to WiFi (blocking with timeout)
void connectWiFi();

// Check if WiFi is connected
bool isWiFiConnected();

#endif // WIFI_MANAGER_H
