#include "pump_controller.h"
#include "config.h"
#include <Preferences.h>
#include <time.h>

// Preferences instance for NVS storage
static Preferences prefs;

// Current pump state
static bool pumpState = false;
static PumpMode currentMode = PUMP_MODE_AUTO;

// Manual mode timeout
static unsigned long manualOffTime = 0;

// Schedules array
static PumpSchedule schedules[MAX_SCHEDULES];
static int scheduleCount = 0;

// Last schedule check time
static unsigned long lastScheduleCheck = 0;
static const unsigned long SCHEDULE_CHECK_INTERVAL = 30000; // Check every 30 seconds

// Time synced flag
static bool timeSynced = false;

/**
 * Set relay state (handles active low/high logic)
 */
void setRelayState(bool on) {
    pumpState = on;
    
    #if RELAY_ACTIVE_LOW
        digitalWrite(RELAY_PIN, on ? LOW : HIGH);
    #else
        digitalWrite(RELAY_PIN, on ? HIGH : LOW);
    #endif
    
    DEBUG_PRINTF("[Pump] Relay set to %s\n", on ? "ON" : "OFF");
}

/**
 * Load schedules from NVS
 */
void loadSchedules() {
    prefs.begin(PREF_NAMESPACE, true); // Read-only
    scheduleCount = prefs.getInt(PREF_KEY_SCHEDULE_COUNT, 0);
    
    for (int i = 0; i < scheduleCount && i < MAX_SCHEDULES; i++) {
        char key[16];
        snprintf(key, sizeof(key), "%s%d", PREF_KEY_SCHEDULE_PREFIX, i);
        
        // Store schedule as packed uint32: enabled(1) | duration(8) | minute(8) | hour(8)
        uint32_t packed = prefs.getUInt(key, 0);
        schedules[i].hour = packed & 0xFF;
        schedules[i].minute = (packed >> 8) & 0xFF;
        schedules[i].durationMin = (packed >> 16) & 0xFF;
        schedules[i].enabled = (packed >> 24) & 0x01;
    }
    
    prefs.end();
    DEBUG_PRINTF("[Pump] Loaded %d schedules from NVS\n", scheduleCount);
}

/**
 * Save schedules to NVS
 */
void saveSchedules() {
    prefs.begin(PREF_NAMESPACE, false); // Read-write
    prefs.putInt(PREF_KEY_SCHEDULE_COUNT, scheduleCount);
    
    for (int i = 0; i < scheduleCount; i++) {
        char key[16];
        snprintf(key, sizeof(key), "%s%d", PREF_KEY_SCHEDULE_PREFIX, i);
        
        // Pack schedule into uint32
        uint32_t packed = schedules[i].hour |
                         (schedules[i].minute << 8) |
                         (schedules[i].durationMin << 16) |
                         ((schedules[i].enabled ? 1 : 0) << 24);
        prefs.putUInt(key, packed);
    }
    
    prefs.end();
    DEBUG_PRINTF("[Pump] Saved %d schedules to NVS\n", scheduleCount);
}

/**
 * Get current time from NTP
 */
bool getCurrentTime(struct tm* timeinfo) {
    if (!timeSynced) {
        return false;
    }
    
    if (!getLocalTime(timeinfo)) {
        DEBUG_PRINTLN("[Pump] Failed to get local time");
        return false;
    }
    
    return true;
}

/**
 * Check if current time matches a schedule
 */
bool checkScheduleMatch(struct tm* timeinfo, PumpSchedule* schedule) {
    if (!schedule->enabled) {
        return false;
    }
    
    return (timeinfo->tm_hour == schedule->hour && 
            timeinfo->tm_min == schedule->minute);
}

void pumpInit() {
    DEBUG_PRINTLN("[Pump] Initializing...");
    
    // Configure relay pin
    pinMode(RELAY_PIN, OUTPUT);
    setRelayState(false); // Start with pump off
    
    // Load saved schedules
    loadSchedules();
    
    DEBUG_PRINTLN("[Pump] Initialized");
}

void pumpLoop() {
    unsigned long now = millis();
    
    // Check for manual mode timeout
    if (currentMode == PUMP_MODE_MANUAL_ON && manualOffTime > 0 && now >= manualOffTime) {
        DEBUG_PRINTLN("[Pump] Manual duration expired, switching to auto");
        pumpSetAuto();
    }
    
    // Only check schedule in auto mode
    if (currentMode != PUMP_MODE_AUTO) {
        return;
    }
    
    // Rate-limit schedule checking
    if (now - lastScheduleCheck < SCHEDULE_CHECK_INTERVAL) {
        return;
    }
    lastScheduleCheck = now;
    
    // Get current time
    struct tm timeinfo;
    if (!getCurrentTime(&timeinfo)) {
        return;
    }
    
    // Check if any schedule matches
    for (int i = 0; i < scheduleCount; i++) {
        if (checkScheduleMatch(&timeinfo, &schedules[i])) {
            if (!pumpState) {
                DEBUG_PRINTF("[Pump] Schedule %d triggered at %02d:%02d\n", 
                           i, timeinfo.tm_hour, timeinfo.tm_min);
                
                // Turn on pump for specified duration
                pumpTurnOn(schedules[i].durationMin);
                currentMode = PUMP_MODE_AUTO; // Stay in auto mode
            }
            break;
        }
    }
    
    // Check if pump should turn off (duration expired in auto mode)
    if (pumpState && manualOffTime > 0 && now >= manualOffTime) {
        DEBUG_PRINTLN("[Pump] Scheduled duration complete");
        setRelayState(false);
        manualOffTime = 0;
    }
}

void pumpTurnOn(int durationMin) {
    setRelayState(true);
    
    if (durationMin > 0) {
        manualOffTime = millis() + (durationMin * 60 * 1000UL);
        DEBUG_PRINTF("[Pump] Will turn off in %d minutes\n", durationMin);
    } else {
        manualOffTime = 0;
    }
    
    if (currentMode != PUMP_MODE_AUTO) {
        currentMode = PUMP_MODE_MANUAL_ON;
    }
}

void pumpTurnOff() {
    setRelayState(false);
    manualOffTime = 0;
    currentMode = PUMP_MODE_MANUAL_OFF;
}

void pumpSetAuto() {
    currentMode = PUMP_MODE_AUTO;
    manualOffTime = 0;
    DEBUG_PRINTLN("[Pump] Switched to AUTO mode");
}

bool pumpIsOn() {
    return pumpState;
}

PumpMode pumpGetMode() {
    return currentMode;
}

bool pumpSetSchedule(int index, PumpSchedule schedule) {
    if (index < 0 || index >= MAX_SCHEDULES) {
        return false;
    }
    
    schedules[index] = schedule;
    if (index >= scheduleCount) {
        scheduleCount = index + 1;
    }
    
    saveSchedules();
    return true;
}

PumpSchedule pumpGetSchedule(int index) {
    if (index >= 0 && index < scheduleCount) {
        return schedules[index];
    }
    return {0, 0, 0, false};
}

int pumpGetScheduleCount() {
    return scheduleCount;
}

void pumpClearSchedules() {
    scheduleCount = 0;
    saveSchedules();
    DEBUG_PRINTLN("[Pump] All schedules cleared");
}

void pumpUpdateSchedulesFromJson(JsonArray& jsonSchedules) {
    scheduleCount = 0;
    
    for (JsonObject sched : jsonSchedules) {
        if (scheduleCount >= MAX_SCHEDULES) break;
        
        schedules[scheduleCount].hour = sched["hour"] | 0;
        schedules[scheduleCount].minute = sched["minute"] | 0;
        schedules[scheduleCount].durationMin = sched["duration_min"] | DEFAULT_PUMP_DURATION_MIN;
        schedules[scheduleCount].enabled = sched["enabled"] | true;
        
        DEBUG_PRINTF("[Pump] Schedule %d: %02d:%02d for %d min (%s)\n",
                    scheduleCount,
                    schedules[scheduleCount].hour,
                    schedules[scheduleCount].minute,
                    schedules[scheduleCount].durationMin,
                    schedules[scheduleCount].enabled ? "enabled" : "disabled");
        
        scheduleCount++;
    }
    
    saveSchedules();
    DEBUG_PRINTF("[Pump] Updated %d schedules from JSON\n", scheduleCount);
}

void pumpSyncTime() {
    DEBUG_PRINTLN("[Pump] Syncing time with NTP...");
    
    configTime(NTP_GMT_OFFSET_SEC, NTP_DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 10000)) { // 10 second timeout
        timeSynced = true;
        DEBUG_PRINTF("[Pump] Time synced: %s", asctime(&timeinfo));
    } else {
        DEBUG_PRINTLN("[Pump] Failed to sync time");
    }
}

String pumpGetTimeString() {
    struct tm timeinfo;
    if (getCurrentTime(&timeinfo)) {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", 
                timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        return String(buffer);
    }
    return "00:00:00";
}
