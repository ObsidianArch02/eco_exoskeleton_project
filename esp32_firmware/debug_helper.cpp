#include "debug_helper.h"
#include <stdio.h>
#include <string.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char* TAG = "DebugHelper";
static const char* PREFS_NAMESPACE = "debug_settings";
static const char* DEBUG_LEVEL_KEY = "debug_level";

int DebugHelper::debugLevel = DEBUG_LEVEL;
bool DebugHelper::initialized = false;

void DebugHelper::initialize() {
    if (!initialized) {
        // Initialize NVS
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK(ret);
        
        // Load saved debug level from NVS
        nvs_handle_t nvs_handle;
        ret = nvs_open(PREFS_NAMESPACE, NVS_READONLY, &nvs_handle);
        if (ret == ESP_OK) {
            size_t required_size = sizeof(int);
            int saved_level = DEBUG_LEVEL;
            ret = nvs_get_blob(nvs_handle, DEBUG_LEVEL_KEY, &saved_level, &required_size);
            if (ret == ESP_OK && saved_level != DEBUG_LEVEL) {
                debugLevel = saved_level;
            }
            nvs_close(nvs_handle);
        }
        
        info("调试系统已初始化. 级别: %d", debugLevel);
        initialized = true;
    }
}

void DebugHelper::setLevel(int level) {
    debugLevel = level;
    
    // Save to non-volatile storage
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(PREFS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret == ESP_OK) {
        ret = nvs_set_blob(nvs_handle, DEBUG_LEVEL_KEY, &level, sizeof(level));
        if (ret == ESP_OK) {
            ret = nvs_commit(nvs_handle);
        }
        nvs_close(nvs_handle);
    }
    
    info("调试级别已设置为: %d", level);
}

void DebugHelper::print(const char* level, const char* format, va_list args) {
    if (!initialized) return;
    
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    // Get current time in milliseconds
    uint32_t time_ms = (uint32_t)(esp_timer_get_time() / 1000);
    
    printf("[%lu][%s] %s\n", time_ms, level, buffer);
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
        printf("%s: ", label);
    }
    
    for (size_t i = 0; i < length; i++) {
        if (i % 16 == 0) {
            if (i > 0) printf("\n");
            printf("%04X: ", (unsigned int)i);
        }
        printf("%02X ", data[i]);
    }
    printf("\n");
}