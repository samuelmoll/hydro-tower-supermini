/**
 * ESP32-C3 Hydroponic Tower Controller
 * 
 * Features:
 * - DHT11 temperature/humidity monitoring
 * - Pump control via relay with scheduling
 * - WiFi connectivity with automatic reconnection
 * - MQTT communication for remote monitoring and control
 * - NTP time synchronization for accurate scheduling
 * 
 * Hardware:
 * - ESP32-C3 Super Mini
 * - DHT11 sensor on GPIO 4
 * - Relay module on GPIO 5
 */

#include <Arduino.h>
#include "config.h"
#include "wifi_manager.h"
#include "mqtt_handler.h"
#include "sensors.h"
#include "pump_controller.h"

// Status publishing interval
static unsigned long lastStatusPublish = 0;

// System start time
static unsigned long systemStartTime = 0;

/**
 * Callback for new sensor readings
 * Publishes sensor data to MQTT
 */
void onSensorReading(SensorReading& reading) {
    if (reading.valid && mqttIsConnected()) {
        mqttPublishSensors(reading.temperature, reading.humidity);
    }
}

/**
 * Callback for pump control commands from MQTT
 */
void onPumpControl(const char* action) {
    DEBUG_PRINTF("[Main] Pump control command: %s\n", action);
    
    if (strcmp(action, "on") == 0) {
        pumpTurnOn(DEFAULT_PUMP_DURATION_MIN);
    } else if (strcmp(action, "off") == 0) {
        pumpTurnOff();
    } else if (strcmp(action, "auto") == 0) {
        pumpSetAuto();
    }
}

/**
 * Callback for schedule configuration from MQTT
 */
void onScheduleConfig(JsonArray& schedules) {
    DEBUG_PRINTLN("[Main] Received schedule update");
    pumpUpdateSchedulesFromJson(schedules);
}

/**
 * Callback for sensor interval configuration from MQTT
 */
void onSensorConfig(int intervalSec) {
    DEBUG_PRINTF("[Main] Sensor interval changed to %d seconds\n", intervalSec);
    sensorsSetInterval(intervalSec);
}

/**
 * Callback for timezone configuration from MQTT
 */
void onTimezoneConfig(int gmtOffsetSec, int dstOffsetSec) {
    DEBUG_PRINTF("[Main] Timezone changed: GMT%+d DST%+d\n", gmtOffsetSec / 3600, dstOffsetSec / 3600);
    pumpSetTimezone(gmtOffsetSec, dstOffsetSec);
}

/**
 * Publish periodic status update
 */
void publishStatus() {
    unsigned long uptime = millis() - systemStartTime;
    mqttPublishStatus(pumpIsOn(), wifiGetRSSI(), uptime);
}

void setup() {
    // Record start time
    systemStartTime = millis();
    
    // Initialize serial for debugging
    Serial.begin(SERIAL_BAUD_RATE);
    delay(1000); // Wait for serial to initialize
    
    DEBUG_PRINTLN("\n\n================================");
    DEBUG_PRINTLN("ESP32-C3 Hydroponic Controller");
    DEBUG_PRINTLN("================================\n");
    
    // Initialize pump controller (sets up GPIO)
    pumpInit();
    
    // Initialize sensors
    sensorsInit();
    
    // Initialize WiFi (blocking initial connection)
    wifiManagerInit();
    
    // Wait for WiFi before continuing
    DEBUG_PRINTLN("[Main] Waiting for WiFi connection...");
    DEBUG_PRINTLN("[Main] Using SSID: " WIFI_SSID);
    DEBUG_PRINTLN("[Main] Using WIFI password: " WIFI_PASSWORD);
    DEBUG_PRINTLN("[Main] Using MQTT password: " MQTT_PASSWORD);
    int attempts = 0;
    while (!wifiIsConnected() && attempts < 30) {
        delay(500);
        DEBUG_PRINT(".");
        attempts++;
    }
    DEBUG_PRINTLN();
    
    if (wifiIsConnected()) {
        DEBUG_PRINTLN("[Main] WiFi connected, initializing MQTT...");
        
        // Sync time with NTP
        pumpSyncTime();
        
        // Initialize MQTT
        mqttInit();
        
        // Register MQTT callbacks
        mqttOnPumpControl(onPumpControl);
        mqttOnScheduleConfig(onScheduleConfig);
        mqttOnSensorConfig(onSensorConfig);
        mqttOnTimezoneConfig(onTimezoneConfig);
    } else {
        DEBUG_PRINTLN("[Main] WiFi not connected, MQTT will retry when connected");
    }
    
    // Start sensor reading task
    sensorsStartTask(onSensorReading);
    
    DEBUG_PRINTLN("\n[Main] Setup complete!\n");
    DEBUG_PRINTF("[Main] Free heap: %d bytes\n", ESP.getFreeHeap());
    DEBUG_PRINTF("[Main] Sensor interval: %d seconds\n", sensorsGetInterval());
    DEBUG_PRINTF("[Main] Schedules loaded: %d\n", pumpGetScheduleCount());
}

void loop() {
    // Handle MQTT connection and messages
    mqttLoop();
    
    // Handle pump scheduling
    pumpLoop();
    
    // Publish status periodically
    unsigned long now = millis();
    if (now - lastStatusPublish >= STATUS_INTERVAL_SEC * 1000UL) {
        lastStatusPublish = now;
        if (mqttIsConnected()) {
            publishStatus();
        }
    }
    
    // Small delay to prevent watchdog issues
    delay(10);
}