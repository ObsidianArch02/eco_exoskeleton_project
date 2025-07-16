#ifndef DEBUG_HELPER_H
#define DEBUG_HELPER_H

#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>

/**
 * @file debug_helper.h
 * @brief Debug logging utility for ESP32 firmware modules
 * 
 * This helper provides multi-level debugging capabilities with support for
 * sensor data logging, calibration logging, and hex dump functionality.
 * Debug levels can be configured and persisted to NVS storage.
 */

// Debug levels - Control verbosity of debug output
#define DEBUG_LEVEL_OFF     0    // No debug output
#define DEBUG_LEVEL_ERROR   1    // Only error messages
#define DEBUG_LEVEL_WARNING 2    // Errors and warnings
#define DEBUG_LEVEL_INFO    3    // Errors, warnings, and info
#define DEBUG_LEVEL_VERBOSE 4    // All debug messages

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL DEBUG_LEVEL_INFO  // Default debug level
#endif

/**
 * @brief Debug helper utility class
 * 
 * Provides static methods for logging at different verbosity levels.
 * The debug level is configurable and can be persisted to NVS storage.
 */
class DebugHelper {
public:
    static int debugLevel;      // Current debug verbosity level
    static bool initialized;    // Initialization status

    /**
     * @brief Initialize debug system and load saved settings from NVS
     * @note This function should be called once during system startup
     */
    static void initialize();

    /**
     * @brief Set debug verbosity level and save to NVS
     * @param level Debug level (0-4, see DEBUG_LEVEL_* defines)
     */
    static void setLevel(int level);

    // Logging functions for different severity levels
    
    /**
     * @brief Log error message (always shown unless OFF)
     * @param format Printf-style format string
     * @param ... Variable arguments for format string
     */
    static void error(const char* format, ...);
    
    /**
     * @brief Log warning message (shown at WARNING level and above)
     * @param format Printf-style format string
     * @param ... Variable arguments for format string
     */
    static void warning(const char* format, ...);
    
    /**
     * @brief Log informational message (shown at INFO level and above)
     * @param format Printf-style format string
     * @param ... Variable arguments for format string
     */
    static void info(const char* format, ...);
    
    /**
     * @brief Log verbose debug message (shown only at VERBOSE level)
     * @param format Printf-style format string
     * @param ... Variable arguments for format string
     */
    static void verbose(const char* format, ...);

    // Specialized logging functions for sensor data and diagnostics
    
    /**
     * @brief Log sensor reading with optional unit
     * @param name Sensor name/identifier
     * @param value Sensor reading value
     * @param unit Unit of measurement (optional, can be empty string)
     */
    static void logSensor(const char* name, float value, const char* unit = "");
    
    /**
     * @brief Log sensor calibration data (raw -> calibrated conversion)
     * @param sensor Sensor name/identifier
     * @param raw Raw sensor value before calibration
     * @param calibrated Calibrated sensor value after processing
     */
    static void logCalibration(const char* sensor, float raw, float calibrated);
    
    /**
     * @brief Dump binary data in hexadecimal format for debugging
     * @param data Pointer to data buffer to dump
     * @param length Number of bytes to dump
     * @param label Optional label to prefix the hex dump (can be nullptr)
     */
    static void hexDump(const uint8_t* data, size_t length, const char* label = nullptr);

private:
    /**
     * @brief Internal print function with timestamp and level formatting
     * @param level Debug level string (ERROR, WARN, INFO, VERBOSE)
     * @param format Printf-style format string
     * @param args Variable argument list
     */
    static void print(const char* level, const char* format, va_list args);
};

#endif // DEBUG_HELPER_H