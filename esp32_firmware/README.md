# ESP32 Exoskeleton Control Modules

## Overview
This repository contains three independent ESP32 modules for controlling components of a robotic exoskeleton system:
1. Greenhouse Module - Deploys/retracts foldable greenhouse structures
2. Injection Module - Controls soil injection mechanisms
3. Bubble Machine Module - Manages repair solution spraying

Each module implements a dedicated functionality and communicates via MQTT protocol with a central control system.

## Module Specifications

### 1. Greenhouse Module
**Purpose**: Controls deployment and retraction of foldable greenhouse structures with environmental monitoring.

**Key Features**:
- Dual-state actuator control (deploy/retract)
- Position feedback sensors
- Temperature and humidity monitoring
- MQTT-based command interface
- Status reporting system

**Hardware Requirements**:
- 2x Digital output pins (deploy/retract control)
- 2x Digital input pins (position feedback)
- 2x Analog input pins (temperature/humidity sensors)
- WiFi connectivity

### 2. Injection Module
**Purpose**: Precise control of soil injection mechanism with depth and pressure monitoring.

**Key Features**:
- PWM-controlled injection motor
- Depth and pressure sensors
- Needle position feedback
- Parameterized injection commands
- Real-time sensor reporting

**Hardware Requirements**:
- 1x PWM output pin (motor control)
- 2x Analog input pins (depth/pressure sensors)
- 1x Digital input pin (needle position)
- WiFi connectivity

### 3. Bubble Machine Module
**Purpose**: Controlled spraying of repair solution with flow monitoring.

**Key Features**:
- Adjustable intensity spraying (PWM)
- Flow rate monitoring
- Tank level sensing
- System pressure monitoring
- Timed spraying operations

**Hardware Requirements**:
- 1x PWM output pin (nozzle control)
- 3x Analog input pins (flow/level/pressure)
- WiFi connectivity

## Communication Protocol

### MQTT Topics Architecture
| Module       | Command Topic                | Status Topic                 | Sensor Topic               |
|--------------|------------------------------|------------------------------|----------------------------|
| Greenhouse   | exoskeleton/greenhouse/cmd   | exoskeleton/greenhouse/stat  | exoskeleton/greenhouse/sen |
| Injection    | exoskeleton/injection/cmd    | exoskeleton/injection/stat   | exoskeleton/injection/sen  |
| Bubble       | exoskeleton/bubble/cmd       | exoskeleton/bubble/stat      | exoskeleton/bubble/sen     |

### Command Format
```json
{
  "action": "<command_name>",
  "params": {
    "<parameter1>": <value1>,
    "<parameter2>": <value2>
  }
}
```

### Status Message Format
```json
{
  "module": "<module_name>",
  "state": "<current_state>",
  "message": "<status_message>",
  "timestamp": <millis_since_start>
}
```

## Installation & Configuration

1. **Hardware Setup**:
   - Connect all sensors and actuators according to pin definitions
   - Ensure proper power supply for all components

2. **Software Configuration**:
   - Update WiFi credentials in each module
   - Set correct MQTT broker address
   - Adjust sensor calibration if needed

3. **Network Requirements**:
   - Stable WiFi network
   - MQTT broker accessible to all modules
   - Unique client IDs for each module

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