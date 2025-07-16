#ifndef SENSOR_CALIBRATION_H
#define SENSOR_CALIBRATION_H

#include "debug_helper.h"

/**
 * @brief Calibrate temperature sensor readings
 * @param raw Raw ADC value
 * @return Calibrated temperature in °C
 */
float calibrate_temperature(float raw) {
    // Example calibration: linear formula y = mx + b
    const float m = 0.125;
    const float b = -12.5;
    float calibrated = (raw * m) + b;
    
    DebugHelper::logCalibration("Temperature", raw, calibrated);
    return calibrated;
}

/**
 * @brief Calibrate pressure sensor readings
 * @param raw Raw ADC value
 * @return Calibrated pressure in kPa
 */
float calibrate_pressure(float raw) {
    // Example calibration: quadratic formula
    const float a = 0.0015;
    const float b = 0.25;
    float calibrated = (a * raw * raw) + (b * raw);
    
    DebugHelper::logCalibration("Pressure", raw, calibrated);
    return calibrated;
}

/**
 * @brief Calibrate flow sensor readings
 * @param raw Raw ADC value
 * @return Calibrated flow rate in L/min
 */
float calibrate_flow(float raw) {
    // Example calibration: piecewise linear
    float calibrated;
    if (raw < 500) {
        calibrated = raw * 0.1;
    } else {
        calibrated = 50 + (raw - 500) * 0.08;
    }
    
    DebugHelper::logCalibration("Flow", raw, calibrated);
    return calibrated;
}

#endif // SENSOR_CALIBRATION_H