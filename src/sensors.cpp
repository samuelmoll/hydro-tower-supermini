#include "sensors.h"
#include "config.h"
#include <DHT.h>

// DHT sensor instance
static DHT dht(DHT_PIN, DHT_TYPE);

// Last valid reading (cached)
static SensorReading lastReading = {0, 0, false, 0};

// Reading interval in seconds
static volatile int readingIntervalSec = DEFAULT_SENSOR_INTERVAL_SEC;

// Callback for new readings
static void (*readingCallback)(SensorReading& reading) = nullptr;

// Task handle
static TaskHandle_t sensorTaskHandle = NULL;

/**
 * FreeRTOS task for periodic sensor reading
 */
void sensorReadTask(void* parameter) {
    DEBUG_PRINTLN("[Sensors] Reading task started");
    
    while (true) {
        SensorReading reading = sensorsRead();
        
        if (reading.valid && readingCallback != nullptr) {
            readingCallback(reading);
        }
        
        // Wait for next reading interval
        vTaskDelay((readingIntervalSec * 1000) / portTICK_PERIOD_MS);
    }
}

void sensorsInit() {
    DEBUG_PRINTLN("[Sensors] Initializing DHT sensor...");
    
    dht.begin();
    
    // Wait for sensor to stabilize
    delay(2000);
    
    // Take initial reading
    SensorReading initial = sensorsRead();
    if (initial.valid) {
        DEBUG_PRINTF("[Sensors] Initial reading: %.1f°C, %.1f%%\n", 
                     initial.temperature, initial.humidity);
    } else {
        DEBUG_PRINTLN("[Sensors] Initial reading failed - check wiring");
    }
}

SensorReading sensorsRead() {
    SensorReading reading;
    reading.timestamp = millis();
    
    // Read humidity (takes ~250ms)
    float h = dht.readHumidity();
    // Read temperature
    float t = dht.readTemperature();
    
    // Check if readings are valid
    if (isnan(h) || isnan(t)) {
        DEBUG_PRINTLN("[Sensors] Failed to read from DHT sensor");
        reading.valid = false;
        reading.temperature = lastReading.temperature;
        reading.humidity = lastReading.humidity;
    } else {
        reading.valid = true;
        reading.temperature = t;
        reading.humidity = h;
        
        // Update cached reading
        lastReading = reading;
        
        DEBUG_PRINTF("[Sensors] Temp: %.1f°C, Humidity: %.1f%%\n", t, h);
    }
    
    return reading;
}

SensorReading sensorsGetLast() {
    return lastReading;
}

void sensorsSetInterval(int intervalSec) {
    if (intervalSec >= 10 && intervalSec <= 3600) {
        readingIntervalSec = intervalSec;
        DEBUG_PRINTF("[Sensors] Interval set to %d seconds\n", intervalSec);
    } else {
        DEBUG_PRINTLN("[Sensors] Invalid interval (must be 10-3600 seconds)");
    }
}

int sensorsGetInterval() {
    return readingIntervalSec;
}

void sensorsStartTask(void (*onReading)(SensorReading& reading)) {
    readingCallback = onReading;
    
    // Create FreeRTOS task for sensor reading
    // Stack size: 4096 bytes, Priority: 2, Core: 1
    xTaskCreatePinnedToCore(
        sensorReadTask,
        "Sensor_Reader",
        4096,
        NULL,
        2,
        &sensorTaskHandle,
        1  // Run on core 1
    );
    
    DEBUG_PRINTLN("[Sensors] Reading task created");
}
