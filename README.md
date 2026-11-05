
# LandslideGuard_ESP32
### IoT-Based Landslide Detection and Early Warning System using ESP32 and TinyML

LandslideGuard is a low-cost IoT-based prototype designed to monitor soil and slope conditions and provide early warning indications using sensor measurements and embedded machine learning.

## Project Overview

The system uses an ESP32 microcontroller to collect sensor data, process measurements locally, perform TinyML inference and activate visual and audible alerts.

## Key Features

- Three-point soil moisture monitoring
- Slope tilt and vibration monitoring
- Surface displacement measurement
- Real-time OLED monitoring
- Embedded TinyML risk classification
- RGB LED risk indication
- Buzzer-based warning system
- High-moisture override when average moisture exceeds 95%

## Hardware Components

- ESP32 DevKit V1
- ADS1115 16-bit ADC
- 3 × Capacitive Soil Moisture Sensors
- MPU6050 Accelerometer and Gyroscope
- VL53L0X Time-of-Flight Distance Sensor
- OLED Display
- RGB LED and Buzzer

## Technology Stack

- Embedded C++ / Arduino IDE
- ESP32
- TinyML
- Python (model development)
- Sensor data acquisition and processing

## Repository Structure

```text
LandslideGuard_ESP32/
├── firmware/
│   ├── LandslideGuard_ESP32.ino
│   ├── landslide_model.h
│   └── README.md
├── calibration/
│   └── moisture_calibration.md
├── hardware/
│   └── connections.md
├── README.md
└── LICENSE
```

## Current Development Status

- Core electronics assembled and tested.
- Sensor integration completed.
- Soil moisture calibration recorded.
- OLED and alert mechanisms functional.
- TinyML inference integrated into firmware.
- Physical landslide simulation and experimental validation are ongoing.

## Important Note

This is an experimental academic prototype. Its classification performance and warning thresholds require further validation through controlled physical experiments. The current firmware retains a placeholder rainfall feature for compatibility with the existing model; rainfall is not measured by a dedicated sensor.

## Project Objective

To develop an affordable embedded IoT system for monitoring soil and slope conditions and demonstrating early warning capabilities for landslide-risk scenarios.
