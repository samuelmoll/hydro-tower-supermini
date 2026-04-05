#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

/**
 * WiFi Manager for ESP32
 * Handles WiFi connection with automatic reconnection using FreeRTOS task
 */

// WiFi connection states
enum WiFiState {
    WIFI_STATE_DISCONNECTED,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_FAILED
};

/**
 * Initialize WiFi manager and start connection
 * Creates a background FreeRTOS task for connection monitoring
 */
void wifiManagerInit();

/**
 * Get current WiFi connection state
 */
WiFiState wifiGetState();

/**
 * Check if WiFi is connected
 */
bool wifiIsConnected();

/**
 * Get WiFi signal strength (RSSI)
 * Returns 0 if not connected
 */
int wifiGetRSSI();

/**
 * Get current IP address as string
 * Returns "0.0.0.0" if not connected
 */
String wifiGetIP();

/**
 * Force reconnection attempt
 */
void wifiReconnect();

#endif // WIFI_MANAGER_H
