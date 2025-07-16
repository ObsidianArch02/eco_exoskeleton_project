# ESP32 Exoskeleton Firmware

This repository contains firmware for various modules of the mechanical exoskeleton system.

## Modules
1. Bubble Machine Module - Spray control system
2. Greenhouse Module - Deployment control system
3. Injection Module - Nutrient injection system

## Building the Firmware

### Prerequisites
- [ESP-IDF v4.4+](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
- CMake v3.16+
- Python 3.8+

### Setup Environment
```bash
# Install ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh

# Set up ESP-IDF
source $IDF_PATH/export.sh
```

### Build Options
| Option | Description |
|--------|-------------|
| `BUILD_MODULE` | Module to build (`bubble`, `greenhouse`, `injection`) |
| `SERIAL_PORT` | Serial port for flashing (default: `/dev/ttyUSB0`) |
| `DEBUG_LEVEL` | Debug verbosity (0-4, default=3) |

### Build Commands
```bash
# Build all modules
./build.sh

# Build specific module
idf.py set-target esp32
idf.py -B build/bubble_machine_module build

# Flash module
idf.py -B build/bubble_machine_module -p /dev/ttyUSB0 flash

# Monitor serial output
idf.py -p /dev/ttyUSB0 monitor

# Clean build
rm -rf build/
```

### Using the Debug Helper
```cpp
#include "debug_helper.h"

void setup() {
  DebugHelper::initialize();
  DebugHelper::setLevel(DEBUG_LEVEL_VERBOSE); // 0=OFF, 1=ERROR, 2=WARN, 3=INFO, 4=VERBOSE
}

void loop() {
  // Basic logging
  DebugHelper::info("System started");
  DebugHelper::error("Sensor failure: %d", error_code);
  
  // Sensor logging
  float temp = read_temperature();
  DebugHelper::logSensor("Temperature", temp, "°C");
  
  // Calibration logging
  DebugHelper::logCalibration("Pressure", raw_value, calibrated_value);
  
  // Binary data inspection
  uint8_t data[32];
  // ... fill data ...
  DebugHelper::hexDump(data, sizeof(data), "Sensor Data");
}
```

## File Structure
```
esp32_firmware/
├── CMakeLists.txt          # Main build configuration
├── build.sh                # Build automation script
├── README.md               # This documentation
├── debug_helper.h          # Debug logging library
├── debug_helper.cpp        # Debug implementation
├── mqtt_helper.h           # Shared MQTT library
├── mqtt_helper.cpp         # MQTT implementation
├── bubble_machine_module.c # Bubble machine firmware
├── greenhouse_module.c     # Greenhouse firmware
└── injection_module.c      # Injection system firmware
```

## Debugging Tips
1. Set `DEBUG_LEVEL=4` in code for maximum verbosity
2. Use `logSensor()` for critical sensor readings
3. Monitor MQTT connections with `DEBUG_LEVEL_VERBOSE`
4. Use hexDump() for inspecting binary protocols
5. Calibration logs show raw→calibrated values

## Calibration Notes
Sensor readings should be calibrated using:
```cpp
// Example calibration function
float calibrate_temperature(float raw) {
  // Custom calibration formula
  float calibrated = (raw * 0.123) + 1.456;
  
  // Log calibration
  DebugHelper::logCalibration("Temperature", raw, calibrated);
  return calibrated;
}
```

## Troubleshooting
**Flashing issues:**
- Check serial port permissions
- Reset ESP32 into bootloader mode
- Verify USB cable supports data transfer

**WiFi/MQTT connection problems:**
- Enable DEBUG_LEVEL_VERBOSE in mqtt_helper.cpp
- Verify WiFi credentials
- Check MQTT broker availability

**Sensor inaccuracies:**
- Verify reference voltages
- Check wiring connections
- Implement calibration curves