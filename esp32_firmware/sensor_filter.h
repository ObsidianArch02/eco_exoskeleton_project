#ifndef SENSOR_FILTER_H
#define SENSOR_FILTER_H

#define FILTER_WINDOW_SIZE 5

/**
 * @brief Moving average filter for sensor data
 */
class SensorFilter {
private:
    float values[FILTER_WINDOW_SIZE] = {0};
    int index = 0;
    int count = 0;
    
public:
    /**
     * @brief Add new value to filter
     * @param value New sensor reading
     */
    void addValue(float value) {
        values[index] = value;
        index = (index + 1) % FILTER_WINDOW_SIZE;
        if (count < FILTER_WINDOW_SIZE) count++;
    }
    
    /**
     * @brief Get filtered value
     * @return Filtered sensor value
     */
    float getFiltered() {
        if (count == 0) return 0.0f;
        
        float sum = 0;
        for (int i = 0; i < count; i++) {
            sum += values[i];
        }
        return sum / count;
    }
};

#endif // SENSOR_FILTER_H