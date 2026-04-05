#include "mqtt_handler.h"
#include "wifi_manager.h"
#include "config.h"
#include <WiFi.h>

// WiFi client for MQTT
static WiFiClient wifiClient;

// MQTT client instance
static PubSubClient mqttClient(wifiClient);

// Callback function pointers
static void (*pumpControlCallback)(const char* action) = nullptr;
static void (*scheduleConfigCallback)(JsonArray& schedules) = nullptr;
static void (*sensorConfigCallback)(int intervalSec) = nullptr;

// Last reconnect attempt time
static unsigned long lastReconnectAttempt = 0;
static const unsigned long RECONNECT_INTERVAL = 5000;

/**
 * Internal callback for all incoming MQTT messages
 */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // Null-terminate the payload
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    
    DEBUG_PRINTF("[MQTT] Message on %s: %s\n", topic, message);
    
    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        DEBUG_PRINTF("[MQTT] JSON parse error: %s\n", error.c_str());
        return;
    }
    
    // Route to appropriate callback based on topic
    if (strcmp(topic, MQTT_TOPIC_PUMP_CONTROL) == 0) {
        if (pumpControlCallback && doc.containsKey("action")) {
            const char* action = doc["action"];
            pumpControlCallback(action);
        }
    }
    else if (strcmp(topic, MQTT_TOPIC_SCHEDULE_CONFIG) == 0) {
        if (scheduleConfigCallback && doc.containsKey("schedules")) {
            JsonArray schedules = doc["schedules"].as<JsonArray>();
            scheduleConfigCallback(schedules);
        }
    }
    else if (strcmp(topic, MQTT_TOPIC_SENSOR_CONFIG) == 0) {
        if (sensorConfigCallback && doc.containsKey("interval_sec")) {
            int interval = doc["interval_sec"];
            sensorConfigCallback(interval);
        }
    }
}

/**
 * Attempt to connect/reconnect to MQTT broker
 */
bool mqttConnect() {
    if (!wifiIsConnected()) {
        return false;
    }
    
    DEBUG_PRINTF("[MQTT] Connecting to %s:%d...\n", MQTT_BROKER, MQTT_PORT);
    
    bool connected;
    if (strlen(MQTT_USERNAME) > 0) {
        connected = mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD);
    } else {
        connected = mqttClient.connect(MQTT_CLIENT_ID);
    }
    
    if (connected) {
        DEBUG_PRINTLN("[MQTT] Connected!");
        
        // Subscribe to control and config topics
        mqttClient.subscribe(MQTT_TOPIC_PUMP_CONTROL);
        mqttClient.subscribe(MQTT_TOPIC_SCHEDULE_CONFIG);
        mqttClient.subscribe(MQTT_TOPIC_SENSOR_CONFIG);
        
        DEBUG_PRINTLN("[MQTT] Subscribed to control topics");
        
        // Publish online status
        JsonDocument doc;
        doc["status"] = "online";
        doc["client_id"] = MQTT_CLIENT_ID;
        
        char buffer[128];
        serializeJson(doc, buffer);
        mqttClient.publish(MQTT_TOPIC_STATUS, buffer, true); // Retained message
        
        return true;
    } else {
        DEBUG_PRINTF("[MQTT] Connection failed, rc=%d\n", mqttClient.state());
        return false;
    }
}

void mqttInit() {
    DEBUG_PRINTLN("[MQTT] Initializing...");
    
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setBufferSize(512); // Increase buffer for larger messages
    
    // Initial connection attempt
    mqttConnect();
}

void mqttLoop() {
    if (!mqttClient.connected()) {
        // Attempt reconnection with rate limiting
        unsigned long now = millis();
        if (now - lastReconnectAttempt > RECONNECT_INTERVAL) {
            lastReconnectAttempt = now;
            if (mqttConnect()) {
                lastReconnectAttempt = 0;
            }
        }
    } else {
        // Process incoming messages and maintain connection
        mqttClient.loop();
    }
}

bool mqttIsConnected() {
    return mqttClient.connected();
}

void mqttPublishSensors(float temperature, float humidity) {
    if (!mqttClient.connected()) {
        DEBUG_PRINTLN("[MQTT] Cannot publish - not connected");
        return;
    }
    
    JsonDocument doc;
    doc["temperature"] = temperature;
    doc["humidity"] = humidity;
    doc["timestamp"] = millis(); // Will be replaced with NTP time in production
    
    char buffer[128];
    serializeJson(doc, buffer);
    
    if (mqttClient.publish(MQTT_TOPIC_SENSORS, buffer)) {
        DEBUG_PRINTF("[MQTT] Published sensors: %s\n", buffer);
    } else {
        DEBUG_PRINTLN("[MQTT] Failed to publish sensors");
    }
}

void mqttPublishStatus(bool pumpState, int wifiRssi, unsigned long uptimeMs) {
    if (!mqttClient.connected()) {
        return;
    }
    
    JsonDocument doc;
    doc["pump_state"] = pumpState ? "on" : "off";
    doc["wifi_rssi"] = wifiRssi;
    doc["uptime_sec"] = uptimeMs / 1000;
    doc["free_heap"] = ESP.getFreeHeap();
    
    char buffer[192];
    serializeJson(doc, buffer);
    
    if (mqttClient.publish(MQTT_TOPIC_STATUS, buffer)) {
        DEBUG_PRINTF("[MQTT] Published status: %s\n", buffer);
    }
}

void mqttOnPumpControl(void (*callback)(const char* action)) {
    pumpControlCallback = callback;
}

void mqttOnScheduleConfig(void (*callback)(JsonArray& schedules)) {
    scheduleConfigCallback = callback;
}

void mqttOnSensorConfig(void (*callback)(int intervalSec)) {
    sensorConfigCallback = callback;
}
