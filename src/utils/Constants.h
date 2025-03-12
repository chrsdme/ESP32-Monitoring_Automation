/**
 * @file Constants.h
 * @brief Global constants for the Mushroom Tent Controller
 */

 #ifndef CONSTANTS_H
 #define CONSTANTS_H
 
 #include <Arduino.h>
 
 namespace Constants {
     // Version information
     constexpr const char* APP_NAME = "Mushroom Tent Controller";
     constexpr const char* APP_VERSION = "1.0.0";
     constexpr const char* FS_VERSION = "1.0.0";
     
     // Default pin assignments for relays
     constexpr uint8_t DEFAULT_RELAY1_PIN = 16;  // Main PSU
     constexpr uint8_t DEFAULT_RELAY2_PIN = 17;  // UV Light (depends on RELAY1)
     constexpr uint8_t DEFAULT_RELAY3_PIN = 19;  // Grow Light
     constexpr uint8_t DEFAULT_RELAY4_PIN = 26;  // Tub Fans (depends on RELAY1)
     constexpr uint8_t DEFAULT_RELAY5_PIN = 33;  // Humidifier (depends on RELAY1)
     constexpr uint8_t DEFAULT_RELAY6_PIN = 23;  // Tube Heater
     constexpr uint8_t DEFAULT_RELAY7_PIN = 25;  // IN/OUT Fans (depends on RELAY1)
     constexpr uint8_t DEFAULT_RELAY8_PIN = 35;  // Reserved for future use
     
     // Default pin assignments for sensors
     constexpr uint8_t DEFAULT_DHT1_PIN = 13;    // Upper DHT
     constexpr uint8_t DEFAULT_DHT2_PIN = 14;    // Lower DHT
     constexpr uint8_t DEFAULT_SCD40_SDA_PIN = 21;
     constexpr uint8_t DEFAULT_SCD40_SCL_PIN = 22;
     
     // Default values for sensor reading intervals
     constexpr uint16_t DEFAULT_DHT_READ_INTERVAL_MS = 5000;      // 5 seconds
     constexpr uint16_t DEFAULT_SCD40_READ_INTERVAL_MS = 10000;   // 10 seconds
     constexpr uint16_t DEFAULT_GRAPH_UPDATE_INTERVAL_MS = 30000; // 30 seconds
     constexpr uint16_t DEFAULT_GRAPH_MAX_POINTS = 100;
     
     // Default environmental thresholds
     constexpr float DEFAULT_HUMIDITY_LOW_THRESHOLD = 50.0f;
     constexpr float DEFAULT_HUMIDITY_HIGH_THRESHOLD = 85.0f;
     constexpr float DEFAULT_TEMPERATURE_LOW_THRESHOLD = 20.0f;
     constexpr float DEFAULT_TEMPERATURE_HIGH_THRESHOLD = 24.0f;
     constexpr float DEFAULT_CO2_LOW_THRESHOLD = 1000.0f;
     constexpr float DEFAULT_CO2_HIGH_THRESHOLD = 1600.0f;
     
     // Default timing values for relay automation
     constexpr uint16_t DEFAULT_USER_OVERRIDE_TIME_MIN = 5;      // 5 minutes
     constexpr uint16_t DEFAULT_FANS_ON_DURATION_MIN = 5;        // 5 minutes
     constexpr uint16_t DEFAULT_FANS_CYCLE_INTERVAL_MIN = 60;    // 60 minutes
     
     // Web server related constants
     constexpr uint16_t DEFAULT_WEB_SERVER_PORT = 80;
     constexpr uint16_t DEFAULT_DEBUG_PORT = 23;
     constexpr uint16_t DEFAULT_OTA_PORT = 3232;
     constexpr const char* DEFAULT_AP_SSID = "MushroomTent-Setup";
     constexpr const char* DEFAULT_AP_PASSWORD = "mushroom";     // Empty for open network
     constexpr const char* DEFAULT_HOSTNAME = "mushroom";
     
     // Security constants
     constexpr const char* DEFAULT_HTTP_USERNAME = "admin";
     constexpr const char* DEFAULT_HTTP_PASSWORD = "admin";
     
     // MQTT related constants
     constexpr const char* DEFAULT_MQTT_BROKER = "192.168.1.100";
     constexpr uint16_t DEFAULT_MQTT_PORT = 1883;
     constexpr const char* DEFAULT_MQTT_TOPIC = "mushroom/tent";
     constexpr const char* DEFAULT_MQTT_USERNAME = "mqtt";
     constexpr const char* DEFAULT_MQTT_PASSWORD = "mqtt";
     
     // File system constants
     constexpr const char* DEFAULT_CONFIG_FILE = "/config/default_config.json";
     constexpr const char* PROFILES_FILE = "/config/profiles.json";
     constexpr const char* NETWORK_CONFIG_FILE = "/config/network.json";
     
     // SPIFFS and NVS constants
     constexpr size_t MAX_LOG_FILE_SIZE = 50 * 1024;  // 50KB max log file size
     constexpr const char* LOG_FILE_PATH = "/logs/system.log";
     
     // NVS namespaces
     constexpr const char* NVS_WIFI_NAMESPACE = "bootwifi";
     constexpr const char* NVS_CONFIG_NAMESPACE = "config";
     
     // NVS keys
     constexpr const char* NVS_WIFI_SSID1_KEY = "wifi_ssid1";
     constexpr const char* NVS_WIFI_PASS1_KEY = "wifi_pass1";
     constexpr const char* NVS_WIFI_SSID2_KEY = "wifi_ssid2";
     constexpr const char* NVS_WIFI_PASS2_KEY = "wifi_pass2";
     constexpr const char* NVS_WIFI_SSID3_KEY = "wifi_ssid3";
     constexpr const char* NVS_WIFI_PASS3_KEY = "wifi_pass3";
     constexpr const char* NVS_HOSTNAME_KEY = "hostname";
     constexpr const char* NVS_HTTP_USER_KEY = "http_user";
     constexpr const char* NVS_HTTP_PASS_KEY = "http_pass";
     
     // WiFi constants
     constexpr int32_t DEFAULT_MIN_RSSI = -80;         // Minimum acceptable RSSI value
     constexpr uint32_t WIFI_CONNECT_TIMEOUT = 10000;  // 10 second timeout for connection
     constexpr uint32_t WIFI_CHECK_INTERVAL = 30000;   // Check WiFi every 30 seconds
     
     // RTOS task priorities
     constexpr UBaseType_t PRIORITY_WIFI = 5;
     constexpr UBaseType_t PRIORITY_WEBSERVER = 4;
     constexpr UBaseType_t PRIORITY_SENSORS = 3;
     constexpr UBaseType_t PRIORITY_RELAY_CONTROL = 3;
     constexpr UBaseType_t PRIORITY_MQTT = 2;
     constexpr UBaseType_t PRIORITY_LOGGING = 1;
     
     // RTOS task stack sizes (in words)
     constexpr uint32_t STACK_SIZE_WIFI = 4096;
     constexpr uint32_t STACK_SIZE_WEBSERVER = 8192;
     constexpr uint32_t STACK_SIZE_SENSORS = 4096;
     constexpr uint32_t STACK_SIZE_RELAY_CONTROL = 2048;
     constexpr uint32_t STACK_SIZE_MQTT = 4096;
     constexpr uint32_t STACK_SIZE_LOGGING = 2048;
 }
 
 // Enum definitions
 enum class LogLevel : uint8_t {
     DEBUG = 0,
     INFO,
     WARN,
     ERROR
 };
 
 enum class PowerMode : uint8_t {
     NO_SLEEP = 0,
     MODEM_SLEEP,
     LIGHT_SLEEP,
     DEEP_SLEEP,
     HIBERNATION
 };
 
 enum class RelayState : uint8_t {
     OFF = 0,
     ON = 1,
     AUTO = 2
 };
 
 enum class RelayTrigger : uint8_t {
     MANUAL = 0,
     SCHEDULE,
     ENVIRONMENTAL,
     DEPENDENT
 };
 
 #endif // CONSTANTS_H