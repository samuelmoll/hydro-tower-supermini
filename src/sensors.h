#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>

/**
 * DHT11 Sensor Reader for ESP32
 * Handles temperature and humidity readings with averaging
 */

// Sensor reading structure
struct SensorReading {
    float temperature;  // Celsius
    float humidity;     // Percentage
    bool valid;         // True if reading was successful
    unsigned long timestamp;
};

/**
 * Initialize DHT sensor
 */
void sensorsInit();

/**
 * Read sensor values
 * Returns a SensorReading struct with validity flag
 */
SensorReading sensorsRead();

/**
 * Get last valid reading
 * Returns cached reading if recent read failed
 */
SensorReading sensorsGetLast();

/**
 * Set sensor reading interval (in seconds)
 * This affects the FreeRTOS task timing
 */
void sensorsSetInterval(int intervalSec);

/**
 * Get current sensor interval
 */
int sensorsGetInterval();

/**
 * Start sensor reading FreeRTOS task
 * Requires a callback function for new readings
 */
void sensorsStartTask(void (*onReading)(SensorReading& reading));

#endif // SENSORS_H
