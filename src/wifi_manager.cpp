#include "wifi_manager.h"
#include "config.h"

// Current WiFi state
static volatile WiFiState currentState = WIFI_STATE_DISCONNECTED;

// Task handle for WiFi monitoring
static TaskHandle_t wifiTaskHandle = NULL;

// Reconnection backoff (exponential: 1s, 2s, 4s, ... up to 32s)
static int reconnectBackoff = 1000;
static const int MAX_BACKOFF = 32000;

/**
 * FreeRTOS task that monitors WiFi connection and handles reconnection
 */
void wifiMonitorTask(void* parameter) {
    DEBUG_PRINTLN("[WiFi] Monitor task started");
    
    while (true) {
        if (WiFi.status() != WL_CONNECTED) {
            if (currentState == WIFI_STATE_CONNECTED) {
                DEBUG_PRINTLN("[WiFi] Connection lost, attempting reconnect...");
                currentState = WIFI_STATE_CONNECTING;
            }
            
            // Attempt reconnection
            WiFi.disconnect();
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            
            // Wait for connection with timeout
            unsigned long startAttempt = millis();
            while (WiFi.status() != WL_CONNECTED && 
                   millis() - startAttempt < WIFI_CONNECT_TIMEOUT_MS) {
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }
            
            if (WiFi.status() == WL_CONNECTED) {
                currentState = WIFI_STATE_CONNECTED;
                reconnectBackoff = 1000; // Reset backoff on success
                DEBUG_PRINTLN("[WiFi] Connected!");
                DEBUG_PRINTF("[WiFi] IP: %s\n", WiFi.localIP().toString().c_str());
                DEBUG_PRINTF("[WiFi] RSSI: %d dBm\n", WiFi.RSSI());
            } else {
                currentState = WIFI_STATE_FAILED;
                DEBUG_PRINTF("[WiFi] Connection failed, retry in %d ms\n", reconnectBackoff);
                vTaskDelay(reconnectBackoff / portTICK_PERIOD_MS);
                
                // Exponential backoff
                reconnectBackoff = min(reconnectBackoff * 2, MAX_BACKOFF);
            }
        } else {
            currentState = WIFI_STATE_CONNECTED;
        }
        
        // Check connection status every 5 seconds when connected
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void wifiManagerInit() {
    DEBUG_PRINTLN("[WiFi] Initializing...");
    
    // Set WiFi mode to station (client)
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false); // We handle reconnection ourselves
    
    // Initial connection attempt
    currentState = WIFI_STATE_CONNECTING;
    DEBUG_PRINTF("[WiFi] Connecting to %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    // Wait for initial connection
    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && 
           millis() - startAttempt < WIFI_CONNECT_TIMEOUT_MS) {
        delay(100);
        DEBUG_PRINT(".");
    }
    DEBUG_PRINTLN();
    
    if (WiFi.status() == WL_CONNECTED) {
        currentState = WIFI_STATE_CONNECTED;
        DEBUG_PRINTLN("[WiFi] Initial connection successful!");
        DEBUG_PRINTF("[WiFi] IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        currentState = WIFI_STATE_FAILED;
        DEBUG_PRINTLN("[WiFi] Initial connection failed, will retry in background");
    }
    
    // Create FreeRTOS task for WiFi monitoring
    // Stack size: 4096 bytes, Priority: 1 (low), Core: 0
    xTaskCreatePinnedToCore(
        wifiMonitorTask,
        "WiFi_Monitor",
        4096,
        NULL,
        1,
        &wifiTaskHandle,
        0  // Run on core 0
    );
}

WiFiState wifiGetState() {
    return currentState;
}

bool wifiIsConnected() {
    return WiFi.status() == WL_CONNECTED;
}

int wifiGetRSSI() {
    if (wifiIsConnected()) {
        return WiFi.RSSI();
    }
    return 0;
}

String wifiGetIP() {
    if (wifiIsConnected()) {
        return WiFi.localIP().toString();
    }
    return "0.0.0.0";
}

void wifiReconnect() {
    if (!wifiIsConnected()) {
        reconnectBackoff = 1000; // Reset backoff
        currentState = WIFI_STATE_CONNECTING;
    }
}
