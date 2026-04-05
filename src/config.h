#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>  // Required for Serial in debug macros

// =============================================================================
// HARDWARE CONFIGURATION
// =============================================================================

// DHT11 Temperature/Humidity Sensor
#define DHT_PIN 4           // GPIO pin connected to DHT11 data pin
#define DHT_TYPE DHT11      // Sensor type (DHT11 or DHT22)

// Relay for Pump Control
#define RELAY_PIN 5         // GPIO pin connected to relay signal
#define RELAY_ACTIVE_LOW false  // Set to true if relay triggers on LOW signal

// =============================================================================
// WIFI CONFIGURATION
// =============================================================================

// Set WiFi credentials in secrets.h

// WiFi connection settings
#define WIFI_CONNECT_TIMEOUT_MS 10000   // Timeout for initial connection
#define WIFI_RECONNECT_INTERVAL_MS 5000 // Interval between reconnection attempts

// =============================================================================
// MQTT CONFIGURATION
// =============================================================================

// MQTT Broker settings - UPDATE THESE for your server
#define MQTT_BROKER "192.168.0.178"  // IP or hostname of MQTT broker
#define MQTT_PORT 1883               // Default MQTT port
#define MQTT_USERNAME "hydro"        // MQTT username (leave empty if no auth)
// MQTT password - passed via environment variable at compile time


// MQTT Client ID (should be unique per device)
#define MQTT_CLIENT_ID "hydro_tower_1"

// MQTT Topics - Publishing
#define MQTT_TOPIC_SENSORS "hydro/sensors"       // Temp/humidity readings
#define MQTT_TOPIC_STATUS "hydro/status"         // Device status

// MQTT Topics - Subscribing
#define MQTT_TOPIC_PUMP_CONTROL "hydro/control/pump"      // Manual pump control
#define MQTT_TOPIC_SCHEDULE_CONFIG "hydro/config/schedule" // Schedule updates
#define MQTT_TOPIC_SENSOR_CONFIG "hydro/config/sensor"     // Sensor interval

// =============================================================================
// TIMING CONFIGURATION
// =============================================================================

// Sensor reading interval (in seconds)
#define DEFAULT_SENSOR_INTERVAL_SEC 60

// Status publish interval (in seconds)
#define STATUS_INTERVAL_SEC 300  // Every 5 minutes

// NTP Time server
#define NTP_SERVER "pool.ntp.org"
#define NTP_GMT_OFFSET_SEC 36000      // GMT offset in seconds for +10:00 timezone
#define NTP_DAYLIGHT_OFFSET_SEC 0 // Daylight saving offset

// =============================================================================
// PUMP SCHEDULE DEFAULTS
// =============================================================================

// Maximum number of schedule entries
#define MAX_SCHEDULES 8

// Default pump duration if not specified (in minutes)
#define DEFAULT_PUMP_DURATION_MIN 5

// =============================================================================
// PREFERENCES (NVS) KEYS
// =============================================================================

#define PREF_NAMESPACE "hydro"
#define PREF_KEY_SENSOR_INTERVAL "sensorInt"
#define PREF_KEY_SCHEDULE_COUNT "schedCount"
#define PREF_KEY_SCHEDULE_PREFIX "sched_"

// =============================================================================
// DEBUG SETTINGS
// =============================================================================

#define DEBUG_SERIAL true
#define SERIAL_BAUD_RATE 115200

#if DEBUG_SERIAL
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(...)
#endif

#endif // CONFIG_H
