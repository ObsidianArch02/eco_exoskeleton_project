#ifndef SENSOR_FILTER_C_H
#define SENSOR_FILTER_C_H

#include <string.h>

#define FILTER_WINDOW_SIZE 5

/**
 * @brief C-compatible moving average filter for sensor data
 */
typedef struct {
    float values[FILTER_WINDOW_SIZE];
    int index;
    int count;
} sensor_filter_t;

/**
 * @brief Initialize sensor filter
 * @param filter Pointer to filter structure
 */
static inline void sensor_filter_init(sensor_filter_t* filter) {
    memset(filter->values, 0, sizeof(filter->values));
    filter->index = 0;
    filter->count = 0;
}

/**
 * @brief Add new value to filter
 * @param filter Pointer to filter structure
 * @param value New sensor reading
 */
static inline void sensor_filter_add_value(sensor_filter_t* filter, float value) {
    filter->values[filter->index] = value;
    filter->index = (filter->index + 1) % FILTER_WINDOW_SIZE;
    if (filter->count < FILTER_WINDOW_SIZE) {
        filter->count++;
    }
}

/**
 * @brief Get filtered value
 * @param filter Pointer to filter structure
 * @return Filtered sensor value
 */
static inline float sensor_filter_get_filtered(sensor_filter_t* filter) {
    if (filter->count == 0) return 0.0f;
    
    float sum = 0;
    for (int i = 0; i < filter->count; i++) {
        sum += filter->values[i];
    }
    return sum / filter->count;
}

#endif // SENSOR_FILTER_C_H 