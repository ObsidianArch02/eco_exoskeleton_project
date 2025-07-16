#ifndef DEBUG_HELPER_H
#define DEBUG_HELPER_H

#include <Arduino.h>
#include <stdarg.h>

// Debug levels
#define DEBUG_LEVEL_OFF     0
#define DEBUG_LEVEL_ERROR   1
#define DEBUG_LEVEL_WARNING 2
#define DEBUG_LEVEL_INFO    3
#define DEBUG_LEVEL_VERBOSE 4

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL DEBUG_LEVEL_INFO  // Default debug level
#endif

class DebugHelper {
public:
    static int debugLevel;
    static bool initialized;

    /**
     * @brief Initialize debug system
     */
    static void initialize();

    /**
     * @brief Set debug verbosity level
     * @param level Debug level (0-4)
     */
    static void setLevel(int level);

    // Logging functions
    static void error(const char* format, ...);
    static void warning(const char* format, ...);
    static void info(const char* format, ...);
    static void verbose(const char* format, ...);

    // Specialized logging
    static void logSensor(const char* name, float value, const char* unit = "");
    static void logCalibration(const char* sensor, float raw, float calibrated);
    static void hexDump(const uint8_t* data, size_t length, const char* label = nullptr);

private:
    static void print(const char* level, const char* format, va_list args);
};

#endif // DEBUG_HELPER_H