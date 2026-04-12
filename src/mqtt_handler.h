#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

/**
 * MQTT Handler for ESP32 Hydroponic System
 * Manages connection, publishing, and subscription to MQTT topics
 */

// Callback type for received messages
typedef void (*MqttMessageCallback)(const char* topic, JsonDocument& doc);

/**
 * Initialize MQTT client
 * Must be called after WiFi is connected
 */
void mqttInit();

/**
 * Handle MQTT connection and message loop
 * Call this regularly from main loop
 */
void mqttLoop();

/**
 * Check if MQTT is connected
 */
bool mqttIsConnected();

/**
 * Publish sensor data (temperature and humidity)
 */
void mqttPublishSensors(float temperature, float humidity);

/**
 * Publish device status
 */
void mqttPublishStatus(bool pumpState, int wifiRssi, unsigned long uptimeMs);

/**
 * Register callback for pump control commands
 */
void mqttOnPumpControl(void (*callback)(const char* action));

/**
 * Register callback for schedule configuration
 */
void mqttOnScheduleConfig(void (*callback)(JsonArray& schedules));

/**
 * Register callback for sensor interval configuration
 */
void mqttOnSensorConfig(void (*callback)(int intervalSec));

/**
 * Register callback for timezone configuration
 */
void mqttOnTimezoneConfig(void (*callback)(int gmtOffsetSec, int dstOffsetSec));

#endif // MQTT_HANDLER_H
