#!/bin/bash

# ESP-IDF setup
export IDF_PATH=${HOME}/esp/esp-idf
source $IDF_PATH/export.sh

# Build configurations
MODULES=("bubble_machine_module" "greenhouse_module" "injection_module")
TARGET="esp32"
SERIAL_PORT="/dev/ttyUSB0"

# Build all modules
for module in "${MODULES[@]}"; do
    echo "=============================="
    echo "Building $module firmware..."
    echo "=============================="
    
    # Set build directory
    BUILD_DIR="build/$module"
    
    # Configure and build
    idf.py set-target $TARGET
    idf.py -B $BUILD_DIR build
    
    if [ $? -eq 0 ]; then
        echo "✅ $module built successfully!"
        
        # Flash firmware (uncomment to enable)
        # echo "Flashing $module to $SERIAL_PORT..."
        # idf.py -B $BUILD_DIR -p $SERIAL_PORT flash
        
        # Start monitor (uncomment to enable)
        # echo "Starting serial monitor..."
        # idf.py -B $BUILD_DIR -p $SERIAL_PORT monitor
    else
        echo "❌ Error building $module"
        exit 1
    fi
done

echo "================================="
echo "All firmware modules built successfully!"
echo "To flash a specific module:"
echo "  idf.py -B build/<module> -p $SERIAL_PORT flash"
echo "To monitor:"
echo "  idf.py -p $SERIAL_PORT monitor"
echo "================================="