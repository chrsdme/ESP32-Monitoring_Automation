# Mushroom Tent Controller - System Documentation

## System Architecture

The Mushroom Tent Controller is built on a modular architecture utilizing FreeRTOS for efficient task management and resource sharing. The system is designed with high reliability, real-time responsiveness, and extensibility in mind.

### Core Components

![System Architecture Diagram](https://via.placeholder.com/800x600.png?text=Mushroom+Tent+Controller+Architecture)

#### 1. Core Application Layer

- **AppCore**: Central orchestrator that initializes and manages all other modules
- **Security Manager**: Handles authentication, encryption, and secure storage
- **RTOS Configuration**: Sets up tasks, priorities, and resource management

#### 2. Network Layer

- **Network Manager**: Handles WiFi connections, fallback mechanisms, and network quality monitoring
- **MQTT Client**: Enables communication with MQTT brokers for IoT integration
- **mDNS Service**: Provides local network discovery via hostname.local addressing

#### 3. System Management Layer

- **Storage Manager**: Manages file system operations, configuration persistence, and data storage
- **Time Manager**: Handles NTP synchronization, scheduling, and time-based operations
- **Log Manager**: Controls logging with different verbosity levels and remote logging capabilities
- **Maintenance Manager**: Implements diagnostics, self-healing routines, and system maintenance
- **Power Manager**: Controls power states, sleep modes, and energy optimization
- **Notification Manager**: Handles alerts and notifications across different channels
- **Profile Manager**: Manages growing profiles with different environmental settings

#### 4. Web Interface Layer

- **Web Server**: Serves the user interface and API endpoints
- **API Endpoints**: Provides programmatic access to system functionality

#### 5. Automation Layer

- **Sensor Manager**: Interfaces with physical sensors and processes readings
- **Relay Manager**: Controls physical relays based on schedules and sensor data
- **Tapo Device Manager**: Interfaces with TP-Link Tapo smart plugs

#### 6. OTA Update Layer

- **OTA Manager**: Handles over-the-air firmware and file system updates
- **Version Manager**: Tracks firmware versions and manages update rollbacks

### Hardware Integration

The system is designed to interface with the following hardware components:

- **ESP32 Microcontroller**: Dual-core processor with built-in WiFi and Bluetooth
- **DHT22 Sensors**: Temperature and humidity monitoring (two units for spatial distribution)
- **SCD40 Sensor**: CO2, temperature, and humidity monitoring
- **Relay Module**: 8-channel relay control for various actuators
- **Tapo P100 Smart Plugs**: Optional replacement for physical relays 1 and 6

## RTOS Implementation

The system utilizes FreeRTOS to manage concurrent tasks and resource sharing, ensuring responsive operation while maintaining system stability.

### Task Structure

| Task Name | Priority | Core | Stack Size | Description |
|-----------|----------|------|------------|-------------|
| InitTask | 10 | 1 | 4096 | System initialization |
| WiFiWatchdog | 3 | 0 | 2048 | Monitors WiFi connection quality |
| SensorTask | 5 | 0 | 3072 | Reads and processes sensor data |
| RelayControl | 4 | 0 | 2048 | Manages relay states based on conditions |
| WebServer | 2 | 1 | 4096 | Handles HTTP requests |
| LogTask | 1 | 0 | 2048 | Processes and stores log entries |
| MQTTClient | 2 | 0 | 3072 | Handles MQTT communication |

### Synchronization Mechanisms

To prevent race conditions and ensure data integrity, the system employs several synchronization primitives:

- **Mutexes**: Protect shared resources from concurrent access
- **Semaphores**: Coordinate task execution and resource allocation
- **Queues**: Facilitate safe inter-task communication
- **Task Notifications**: Lightweight signaling between tasks

### Memory Management

The system implements a careful memory management strategy to prevent leaks and fragmentation:

- **Static Allocation**: Used for critical system components
- **Dynamic Allocation**: Used for temporary buffers and web requests
- **Heap Management**: FreeRTOS heap_4 implementation for efficient memory allocation

## Data Flow

### Sensor Reading Process

1. Sensor tasks read data at configurable intervals
2. Raw readings are processed and validated
3. Valid readings are stored in a circular buffer for trend analysis
4. Current readings are made available for relay control decisions
5. Data is published to MQTT topics if configured
6. Readings are added to graph data points at the configured interval

### Relay Control Logic

Relays are controlled based on a priority system:

1. Manual override (user-initiated) - highest priority
2. Safety conditions (critical thresholds) - high priority
3. Environmental triggers (sensor thresholds) - medium priority
4. Scheduled operations (time-based) - base priority

Each relay has specific control logic:

- **Relay 1 (Main PSU)**: Activated when any dependent relay needs power, auto-shutdown when all dependents are off
- **Relay 2 (UV Light)**: Follows time schedule, alternates with Relay 3
- **Relay 3 (Grow Light)**: Follows time schedule, alternates with Relay 2
- **Relay 4 (Tub Fans)**: Runs on cycle schedule or when CO2 levels are low
- **Relay 5 (Humidifiers)**: Activates when humidity is below threshold until target is reached
- **Relay 6 (Heater)**: Activates when temperature is below threshold until target is reached
- **Relay 7 (IN/OUT Fans)**: Runs on cycle schedule or when CO2 levels are high
- **Relay 8 (Reserved)**: No default automation

## Security Implementation

### Authentication

- HTTP Basic Authentication for web interface and API
- Password hashing using SHA-256
- Token-based authentication for extended sessions

### Data Protection

- HTTPS support for encrypted web traffic (when certificates are provided)
- Secure storage of sensitive information in NVS
- Encrypted credentials for network and service connections

### Access Control

- Role-based permissions (not implemented in v1.0.1)
- IP-based access restrictions (not implemented in v1.0.1)

## Networking

### WiFi Management

The system incorporates robust WiFi management:

- Support for multiple WiFi networks with automatic failover
- Signal quality monitoring with threshold-based network switching
- Automatic reconnection with exponential backoff
- AP mode fallback for configuration when no networks are available

### MQTT Integration

MQTT support allows integration with home automation systems:

- Configurable broker, port, and credentials
- Automatic reconnection to broker
- Topic structure: `<base_topic>/<subtopic>`
- QoS levels configurable per topic
- Last Will and Testament for offline detection

## Storage System

### File System Layout

The ESP32's SPIFFS is organized as follows:

- `/config/`: Configuration files (profiles.json, network.json, etc.)
- `/logs/`: System logs
- `/data/`: User data (if applicable)
- `/`: Web interface files (HTML, CSS, JS)

### Configuration Persistence

System configuration is stored in several locations:

- **Network Credentials**: Stored in WiFi NVS namespace
- **HTTP Auth**: Stored in config NVS namespace
- **System Settings**: Stored in JSON files on SPIFFS
- **Profiles**: Stored in profiles.json on SPIFFS

## Error Handling and Recovery

The system implements comprehensive error handling and recovery mechanisms:

### Sensor Failures

1. Sensors are monitored for consecutive read failures
2. After a configurable number of failures, a sensor reset is attempted
3. If reset fails, the system falls back to readings from other sensors
4. Critical failures are logged and reported via notification system

### Network Failures

1. Connection failures trigger reconnection attempts with backoff
2. If primary network fails, system tries alternative networks
3. After exhausting all options, system can enter low-power mode or continue operating with local control only

### System Crashes

1. Watchdog timers monitor task execution
2. Crash logs are preserved for diagnostic purposes
3. Auto-reboot capability restores operation after critical failures
4. Scheduled reboots can be configured for preventative maintenance

## Extensibility

The modular design allows for easy extension of system capabilities:

### Adding New Sensors

1. Create a new sensor class implementing the ISensor interface
2. Register the sensor with the SensorManager
3. Add corresponding UI elements for displaying readings

### Adding New Actuators

1. Define relay mapping in the RelayManager
2. Implement control logic in the appropriate module
3. Add UI elements for manual control and configuration

### Integrating New Smart Devices

1. Implement a device-specific client class
2. Register with the appropriate manager
3. Extend the Tapo interface to accommodate the new device type

## Performance Considerations

### Resource Utilization

- Flash Usage: ~1.5MB (firmware) + ~1MB (SPIFFS)
- RAM Usage: ~200KB peak usage
- CPU Usage: ~30% average, peaks during WiFi activities and web serving

### Optimization Techniques

- Task-appropriate core assignment for balanced dual-core utilization
- Strategic use of sleep modes during inactive periods
- Sensor reading coalescing to minimize I2C bus traffic
- Responsive UI through efficient JSON handling and minimal DOM operations

## Build System

The project uses PlatformIO as the build system, with the following key configurations:

### Partition Scheme

Custom partition table to optimize space allocation:

- **Factory**: 1.5MB for firmware
- **OTA_0**: 1.5MB for OTA updates
- **SPIFFS**: 2MB for file system

### Build Flags

```
build_flags = 
  -D ESP32
  -D CORE_DEBUG_LEVEL=0
  -D CONFIG_ARDUHAL_LOG_COLORS=1
  -D ASYNC_TCP_SSL_ENABLED=1
  -D DEBUG_ESP_PORT=Serial
```

### Dependencies

The primary libraries used in the project include:

- ESPAsyncWebServer
- ArduinoJson
- AsyncMqttClient
- SensirionI2cScd4x
- DHT sensor library
- ESP32 AnalogWrite

## Known Limitations

- CO2 sensor warm-up period can result in inaccurate initial readings
- WiFi performance may degrade in environments with many 2.4GHz devices
- Web interface responsiveness may be reduced during OTA updates
- Deep sleep mode disables all monitoring functionality
- Limited to 3 WiFi network configurations
- No offline data logging to external storage

## Future Enhancements

Planned features for future versions:

- SD card support for extended data logging
- Bluetooth connectivity for direct mobile control
- Enhanced security with user roles and permissions
- Energy usage monitoring for connected devices
- Integration with additional smart device platforms
- Advanced scheduling with dawn/dusk simulation
- Machine learning for optimized growing conditions