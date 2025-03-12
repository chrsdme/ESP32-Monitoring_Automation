/**
 * @file RelayManager.h
 * @brief Manages all relay operations with scheduling, dependencies, and automation
 */

 #ifndef RELAY_MANAGER_H
 #define RELAY_MANAGER_H
 
 #include <Arduino.h>
 #include <freertos/FreeRTOS.h>
 #include <freertos/task.h>
 #include <freertos/semphr.h>
 #include <freertos/queue.h>
 #include <vector>
 #include <map>
 #include <time.h>
 #include "../utils/Constants.h"
 
 // Forward declarations
 class AppCore;
 class SensorManager;
 
 /**
  * @struct TimeRange
  * @brief Structure to hold operating time range
  */
 struct TimeRange {
     uint8_t startHour;
     uint8_t startMinute;
     uint8_t endHour;
     uint8_t endMinute;
     
     TimeRange() : startHour(0), startMinute(0), endHour(23), endMinute(59) {}
     
     TimeRange(uint8_t sh, uint8_t sm, uint8_t eh, uint8_t em) 
         : startHour(sh), startMinute(sm), endHour(eh), endMinute(em) {}
     
     String toString() const {
         char buffer[12];
         sprintf(buffer, "%02d:%02d-%02d:%02d", startHour, startMinute, endHour, endMinute);
         return String(buffer);
     }
     
     void fromString(const String& str) {
         if (sscanf(str.c_str(), "%hhu:%hhu-%hhu:%hhu", &startHour, &startMinute, &endHour, &endMinute) != 4) {
             // Default to all day if parsing fails
             startHour = 0;
             startMinute = 0;
             endHour = 23;
             endMinute = 59;
         }
     }
     
     bool isInRange(uint8_t hour, uint8_t minute) const {
         uint16_t currentMinutes = hour * 60 + minute;
         uint16_t startMinutes = startHour * 60 + startMinute;
         uint16_t endMinutes = endHour * 60 + endMinute;
         
         if (endMinutes >= startMinutes) {
             // Normal range (e.g., 08:00-16:00)
             return currentMinutes >= startMinutes && currentMinutes <= endMinutes;
         } else {
             // Overnight range (e.g., 22:00-06:00)
             return currentMinutes >= startMinutes || currentMinutes <= endMinutes;
         }
     }
 };
 
 /**
  * @struct CycleConfig
  * @brief Structure to hold cycle configuration for relays
  */
 struct CycleConfig {
     uint16_t onDurationMinutes;
     uint16_t intervalMinutes;
     
     CycleConfig() : onDurationMinutes(5), intervalMinutes(60) {}
     
     CycleConfig(uint16_t on, uint16_t interval) 
         : onDurationMinutes(on), intervalMinutes(interval) {}
 };
 
 /**
  * @struct RelayConfig
  * @brief Structure to hold relay configuration
  */
 struct RelayConfig {
     uint8_t relayId;
     String name;
     uint8_t pin;
     TimeRange operatingTime;
     bool visible;
     bool hasDependency;
     uint8_t dependsOnRelay;
     bool isOn;
     RelayState state;
     RelayTrigger lastTrigger;
     uint32_t overrideUntil;  // Time in millis when override expires
     
     RelayConfig() : 
         relayId(0),
         name(""),
         pin(0),
         visible(true),
         hasDependency(false),
         dependsOnRelay(0),
         isOn(false),
         state(RelayState::OFF),
         lastTrigger(RelayTrigger::MANUAL),
         overrideUntil(0) {}
 };
 
 /**
  * @struct EnvironmentalThresholds
  * @brief Structure to hold environmental thresholds for automation
  */
 struct EnvironmentalThresholds {
     float humidityLow;
     float humidityHigh;
     float temperatureLow;
     float temperatureHigh;
     float co2Low;
     float co2High;
     
     EnvironmentalThresholds() :
         humidityLow(Constants::DEFAULT_HUMIDITY_LOW_THRESHOLD),
         humidityHigh(Constants::DEFAULT_HUMIDITY_HIGH_THRESHOLD),
         temperatureLow(Constants::DEFAULT_TEMPERATURE_LOW_THRESHOLD),
         temperatureHigh(Constants::DEFAULT_TEMPERATURE_HIGH_THRESHOLD),
         co2Low(Constants::DEFAULT_CO2_LOW_THRESHOLD),
         co2High(Constants::DEFAULT_CO2_HIGH_THRESHOLD) {}
 };
 
 /**
  * @class RelayManager
  * @brief Manages relay operations with scheduling, dependencies, and automation
  */
 class RelayManager {
 public:
     RelayManager();
     ~RelayManager();
     
     /**
      * @brief Initialize the relay manager
      * @return True if initialized successfully
      */
     bool begin();
     
     /**
      * @brief Initialize relay configurations
      * @return True if relays initialized successfully
      */
     bool initRelays();
     
     /**
      * @brief Set relay pin configuration
      * @param relayId Relay ID (1-8)
      * @param pin GPIO pin for the relay
      * @return True if configuration saved successfully
      */
     bool setRelayPin(uint8_t relayId, uint8_t pin);
     
     /**
      * @brief Get relay pin configuration
      * @param relayId Relay ID (1-8)
      * @return GPIO pin for the relay
      */
     uint8_t getRelayPin(uint8_t relayId);
     
     /**
      * @brief Set relay name
      * @param relayId Relay ID (1-8)
      * @param name Display name for the relay
      * @return True if name saved successfully
      */
     bool setRelayName(uint8_t relayId, const String& name);
     
     /**
      * @brief Get relay name
      * @param relayId Relay ID (1-8)
      * @return Display name for the relay
      */
     String getRelayName(uint8_t relayId);
     
     /**
      * @brief Set relay operating time
      * @param relayId Relay ID (1-8)
      * @param startHour Start hour (0-23)
      * @param startMinute Start minute (0-59)
      * @param endHour End hour (0-23)
      * @param endMinute End minute (0-59)
      * @return True if operating time saved successfully
      */
     bool setRelayOperatingTime(uint8_t relayId, uint8_t startHour, uint8_t startMinute, 
                               uint8_t endHour, uint8_t endMinute);
     
     /**
      * @brief Get relay operating time
      * @param relayId Relay ID (1-8)
      * @return Operating time range
      */
     TimeRange getRelayOperatingTime(uint8_t relayId);
     
     /**
      * @brief Set relay visibility
      * @param relayId Relay ID (1-8)
      * @param visible True if relay should be visible in the UI
      * @return True if visibility saved successfully
      */
     bool setRelayVisibility(uint8_t relayId, bool visible);
     
     /**
      * @brief Get relay visibility
      * @param relayId Relay ID (1-8)
      * @return True if relay is visible in the UI
      */
     bool getRelayVisibility(uint8_t relayId);
     
     /**
      * @brief Set relay dependency
      * @param relayId Relay ID (1-8)
      * @param hasDependency True if relay has a dependency
      * @param dependsOnRelay ID of the relay this one depends on
      * @return True if dependency saved successfully
      */
     bool setRelayDependency(uint8_t relayId, bool hasDependency, uint8_t dependsOnRelay = 0);
     
     /**
      * @brief Get relay dependency
      * @param relayId Relay ID (1-8)
      * @param hasDependency Output parameter for dependency status
      * @param dependsOnRelay Output parameter for dependency relay ID
      * @return True if dependency retrieved successfully
      */
     bool getRelayDependency(uint8_t relayId, bool& hasDependency, uint8_t& dependsOnRelay);
     
     /**
      * @brief Set cycle configuration
      * @param onDurationMinutes Duration of ON state in minutes
      * @param intervalMinutes Total cycle interval in minutes
      * @return True if configuration saved successfully
      */
     bool setCycleConfig(uint16_t onDurationMinutes, uint16_t intervalMinutes);
     
     /**
      * @brief Get cycle configuration
      * @param onDurationMinutes Output parameter for ON duration
      * @param intervalMinutes Output parameter for cycle interval
      * @return True if configuration retrieved successfully
      */
     bool getCycleConfig(uint16_t& onDurationMinutes, uint16_t& intervalMinutes);
     
     /**
      * @brief Set environmental thresholds
      * @param humidityLow Low humidity threshold (%)
      * @param humidityHigh High humidity threshold (%)
      * @param temperatureLow Low temperature threshold (°C)
      * @param temperatureHigh High temperature threshold (°C)
      * @param co2Low Low CO2 threshold (ppm)
      * @param co2High High CO2 threshold (ppm)
      * @return True if thresholds saved successfully
      */
     bool setEnvironmentalThresholds(float humidityLow, float humidityHigh,
                                    float temperatureLow, float temperatureHigh,
                                    float co2Low, float co2High);
     
     /**
      * @brief Get environmental thresholds
      * @param humidityLow Output parameter for low humidity threshold
      * @param humidityHigh Output parameter for high humidity threshold
      * @param temperatureLow Output parameter for low temperature threshold
      * @param temperatureHigh Output parameter for high temperature threshold
      * @param co2Low Output parameter for low CO2 threshold
      * @param co2High Output parameter for high CO2 threshold
      * @return True if thresholds retrieved successfully
      */
     bool getEnvironmentalThresholds(float& humidityLow, float& humidityHigh,
                                    float& temperatureLow, float& temperatureHigh,
                                    float& co2Low, float& co2High);
     
     /**
      * @brief Set user override duration
      * @param minutes Override duration in minutes
      * @return True if duration saved successfully
      */
     bool setOverrideDuration(uint16_t minutes);
     
     /**
      * @brief Get user override duration
      * @return Override duration in minutes
      */
     uint16_t getOverrideDuration();
     
     /**
      * @brief Set relay state (manual override)
      * @param relayId Relay ID (1-8)
      * @param state Desired relay state
      * @return True if state set successfully
      */
     bool setRelayState(uint8_t relayId, RelayState state);
     
     /**
      * @brief Get relay state
      * @param relayId Relay ID (1-8)
      * @return Current relay state
      */
     RelayState getRelayState(uint8_t relayId);
     
     /**
      * @brief Get relay power status
      * @param relayId Relay ID (1-8)
      * @return True if relay is currently ON
      */
     bool isRelayOn(uint8_t relayId);
     
     /**
      * @brief Get last trigger for relay
      * @param relayId Relay ID (1-8)
      * @return Last trigger type
      */
     RelayTrigger getRelayLastTrigger(uint8_t relayId);
     
     /**
      * @brief Get all relay configurations
      * @return Vector of relay configurations
      */
     std::vector<RelayConfig> getAllRelayConfigs();
     
     /**
      * @brief Create RTOS tasks for relay operations
      */
     void createTasks();
     
 private:
     // Relay configurations
     std::map<uint8_t, RelayConfig> _relayConfigs;
     
     // Environmental thresholds
     EnvironmentalThresholds _thresholds;
     
     // Cycle configuration
     CycleConfig _cycleConfig;
     
     // User override duration in minutes
     uint16_t _overrideDurationMinutes;
     
     // RTOS resources
     SemaphoreHandle_t _relayMutex;
     TaskHandle_t _relayControlTaskHandle;
     
     // Status tracking
     bool _isInitialized;
     
     // Private methods
     bool physicallyControlRelay(uint8_t relayId, bool turnOn, RelayTrigger trigger);
     bool checkDependencyChain(uint8_t relayId);
     bool isInOperatingTime(uint8_t relayId);
     void manageDependentRelays(uint8_t relayId, bool turnOn);
     
     // Task functions
     static void relayControlTask(void* parameter);
 };
 
 #endif // RELAY_MANAGER_H