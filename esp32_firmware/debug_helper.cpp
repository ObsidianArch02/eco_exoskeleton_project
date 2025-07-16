#include "debug_helper.h"
#include <stdio.h>
#include <string.h>

#include <Preferences.h>

Preferences debug_prefs;
const char* PREFS_NAMESPACE = "debug_settings";
const char* DEBUG_LEVEL_KEY = "debug_level";

int DebugHelper::debugLevel = DEBUG_LEVEL;

void DebugHelper::initialize() {
    if (!initialized) {
        Serial.begin(115200);
        while (!Serial); // Wait for serial port to connect
        
        // Load saved debug level from NVS
        debug_prefs.begin(PREFS_NAMESPACE, true); // Read-only
        int saved_level = debug_prefs.getInt(DEBUG_LEVEL_KEY, DEBUG_LEVEL);
        debug_prefs.end();
        
        if (saved_level != DEBUG_LEVEL) {
            debugLevel = saved_level;
        }
        
        info("调试系统已初始化. 级别: %d", debugLevel);
        initialized = true;
    }
}

void DebugHelper::setLevel(int level) {
    debugLevel = level;
    
    // Save to non-volatile storage
    debug_prefs.begin(PREFS_NAMESPACE, false); // Read-write
    debug_prefs.putInt(DEBUG_LEVEL_KEY, level);
    debug_prefs.end();
    
    info("调试级别已设置为: %d", level);
}
bool DebugHelper::initialized = false;

void DebugHelper::initialize() {
    if (!initialized) {
        Serial.begin(115200);
        while (!Serial); // Wait for serial port to connect
        info("Debug helper initialized. Level: %d", debugLevel);
        initialized = true;
    }
}

void DebugHelper::setLevel(int level) {
    debugLevel = level;
    info("Debug level set to: %d", level);
}

void DebugHelper::print(const char* level, const char* format, va_list args) {
    if (!initialized) return;
    
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    char final[300];
    snprintf(final, sizeof(final), "[%lu][%s] %s", millis(), level, buffer);
    
    Serial.println(final);
}

void DebugHelper::error(const char* format, ...) {
    if (debugLevel >= DEBUG_LEVEL_ERROR) {
        va_list args;
        va_start(args, format);
        print("ERROR", format, args);
        va_end(args);
    }
}

void DebugHelper::warning(const char* format, ...) {
    if (debugLevel >= DEBUG_LEVEL_WARNING) {
        va_list args;
        va_start(args, format);
        print("WARN", format, args);
        va_end(args);
    }
}

void DebugHelper::info(const char* format, ...) {
    if (debugLevel >= DEBUG_LEVEL_INFO) {
        va_list args;
        va_start(args, format);
        print("INFO", format, args);
        va_end(args);
    }
}

void DebugHelper::verbose(const char* format, ...) {
    if (debugLevel >= DEBUG_LEVEL_VERBOSE) {
        va_list args;
        va_start(args, format);
        print("VERBOSE", format, args);
        va_end(args);
    }
}

void DebugHelper::logSensor(const char* name, float value, const char* unit) {
    if (debugLevel >= DEBUG_LEVEL_INFO) {
        if (strlen(unit) > 0) {
            info("Sensor %s: %.2f %s", name, value, unit);
        } else {
            info("Sensor %s: %.2f", name, value);
        }
    }
}

void DebugHelper::logCalibration(const char* sensor, float raw, float calibrated) {
    if (debugLevel >= DEBUG_LEVEL_VERBOSE) {
        verbose("Calibration [%s]: raw=%.2f -> calibrated=%.2f", sensor, raw, calibrated);
    }
}

void DebugHelper::hexDump(const uint8_t* data, size_t length, const char* label) {
    if (debugLevel < DEBUG_LEVEL_VERBOSE) return;
    
    if (label) {
        Serial.print(label);
        Serial.print(": ");
    }
    
    for (size_t i = 0; i < length; i++) {
        if (i % 16 == 0) {
            if (i > 0) Serial.println();
            Serial.printf("%04X: ", i);
        }
        Serial.printf("%02X ", data[i]);
    }
    Serial.println();
}