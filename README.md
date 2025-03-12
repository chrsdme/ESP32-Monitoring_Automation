# Mushroom Tent Controller

## Overview

The Mushroom Tent Controller is a comprehensive ESP32-based system designed to automate and monitor mushroom cultivation environments. This project provides precise control over environmental factors essential for successful mushroom growing, including temperature, humidity, CO2 levels, and lighting schedules.

## Features

- **Environmental Monitoring**: Real-time tracking of temperature, humidity, and CO2 levels using DHT22 and SCD40 sensors
- **Automated Control**: Intelligent management of heating, humidification, lighting, and ventilation systems
- **Multiple Operation Modes**: Pre-configured profiles for different growing stages (Colonization, Fruiting)
- **User-Friendly Interface**: Responsive web interface accessible from any device with a browser
- **WiFi Connectivity**: Reliable wireless connection with failover options and network quality monitoring
- **Data Visualization**: Real-time graphs for environmental metrics
- **Smart Device Integration**: Compatible with Tapo P100 smart plugs for enhanced control
- **Profile Management**: Save, load, and customize growing profiles for different mushroom varieties
- **OTA Updates**: Remote firmware and file system updates

## Hardware Requirements

- ESP32-Dev-Module
- 2× DHT22 temperature and humidity sensors
- SCD40 CO2 sensor
- 8-channel relay module
- Optional: Tapo P100 smart plugs (can replace physical relays)

## Software Architecture

The Mushroom Tent Controller is built using a modular, RTOS-based approach:

- **Core Modules**: System orchestration, task management, and security
- **Network Management**: WiFi connectivity, MQTT communication, and fallback mechanisms
- **Sensor Interface**: Temperature, humidity, and CO2 monitoring with error recovery
- **Relay Control**: Intelligent automation based on environmental readings and schedules
- **Web Interface**: Responsive dashboard and configuration pages

## Documentation

- [User Manual](UserManual.md): Setup and operating instructions
- [System Documentation](SystemDocs.md): Architecture and implementation details
- [API Documentation](API.md): REST API endpoints and usage

## Getting Started

1. Follow the [User Manual](UserManual.md) for hardware setup instructions
2. Flash the firmware to your ESP32 using Platform IO
3. The device will start in AP mode for initial configuration
4. Connect to the WiFi network "MushroomController" with password "mushroomsetup"
5. Navigate to http://192.168.4.1 to complete the setup

## License

This project is licensed under the Apache License Version 2.0 - see the LICENSE file for details.

## Acknowledgements

This project was built using various open-source libraries and tools:
- ESP32 Arduino Core
- FreeRTOS
- ESPAsyncWebServer
- ArduinoJson
- Chart.js
- Bootstrap