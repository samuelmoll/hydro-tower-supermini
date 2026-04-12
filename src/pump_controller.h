#ifndef PUMP_CONTROLLER_H
#define PUMP_CONTROLLER_H

#include <Arduino.h>
#include <ArduinoJson.h>

/**
 * Pump Controller for ESP32 Hydroponic System
 * Manages pump relay with scheduling and manual control
 */

// Schedule entry structure
struct PumpSchedule {
    uint8_t hour;           // 0-23
    uint8_t minute;         // 0-59
    uint8_t durationMin;    // Duration in minutes
    bool enabled;
};

// Pump mode
enum PumpMode {
    PUMP_MODE_AUTO,     // Follow schedule
    PUMP_MODE_MANUAL_ON,  // Forced on
    PUMP_MODE_MANUAL_OFF  // Forced off
};

/**
 * Initialize pump controller
 * Sets up GPIO and loads schedule from NVS
 */
void pumpInit();

/**
 * Main pump control loop
 * Call regularly to check schedule and control pump
 */
void pumpLoop();

/**
 * Turn pump on manually
 * @param durationMin Duration in minutes (0 = indefinite until manual off)
 */
void pumpTurnOn(int durationMin = 0);

/**
 * Turn pump off
 */
void pumpTurnOff();

/**
 * Set pump to auto mode (follow schedule)
 */
void pumpSetAuto();

/**
 * Check if pump is currently on
 */
bool pumpIsOn();

/**
 * Get current pump mode
 */
PumpMode pumpGetMode();

/**
 * Add or update a schedule entry
 * @param index Schedule index (0 to MAX_SCHEDULES-1)
 * @param schedule Schedule entry
 * @return true if successful
 */
bool pumpSetSchedule(int index, PumpSchedule schedule);

/**
 * Get a schedule entry
 */
PumpSchedule pumpGetSchedule(int index);

/**
 * Get number of active schedules
 */
int pumpGetScheduleCount();

/**
 * Clear all schedules
 */
void pumpClearSchedules();

/**
 * Update schedules from JSON array
 * Format: [{"hour": 8, "minute": 0, "duration_min": 5, "enabled": true}, ...]
 */
void pumpUpdateSchedulesFromJson(JsonArray& schedules);

/**
 * Sync time with NTP server
 * Must be called after WiFi is connected
 */
void pumpSyncTime();

/**
 * Update timezone configuration
 * @param gmtOffsetSec GMT offset in seconds
 * @param dstOffsetSec Daylight saving offset in seconds
 */
void pumpSetTimezone(int gmtOffsetSec, int dstOffsetSec);

/**
 * Get current GMT offset in seconds
 */
int pumpGetGmtOffset();

/**
 * Get current time as string (HH:MM:SS)
 */
String pumpGetTimeString();

#endif // PUMP_CONTROLLER_H
