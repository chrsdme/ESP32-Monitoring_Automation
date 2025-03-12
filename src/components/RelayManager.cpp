/**
 * @file RelayManager.cpp
 * @brief Implementation of the RelayManager class
 */

 #include "RelayManager.h"
 #include "../core/AppCore.h"
 #include "../system/LogManager.h"
 #include "../components/SensorManager.h"
 #include <time.h>
 
 RelayManager::RelayManager() :
     _overrideDurationMinutes(Constants::DEFAULT_USER_OVERRIDE_TIME_MIN),
     _relayMutex(nullptr),
     _relayControlTaskHandle(nullptr),
     _isInitialized(false)
 {
 }
 
 RelayManager::~RelayManager() {
     // Clean up RTOS resources
     if (_relayMutex != nullptr) {
         vSemaphoreDelete(_relayMutex);
     }
     
     if (_relayControlTaskHandle != nullptr) {
         vTaskDelete(_relayControlTaskHandle);
     }
     
     // Ensure all relays are off
     for (const auto& entry : _relayConfigs) {
         pinMode(entry.second.pin, OUTPUT);
         digitalWrite(entry.second.pin, LOW);
     }
 }
 
 bool RelayManager::begin() {
     // Create mutex for thread-safe operations
     _relayMutex = xSemaphoreCreateMutex();
     if (_relayMutex == nullptr) {
         Serial.println("Failed to create relay mutex!");
         return false;
     }
     
     _isInitialized = true;
     return true;
 }
 
 bool RelayManager::initRelays() {
     getAppCore()->getLogManager()->log(LogLevel::INFO, "Relays", "Initializing relays");
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_relayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Initialize default relay configurations
         
         // Relay 1 - Main PSU
         RelayConfig relay1;
         relay1.relayId = 1;
         relay1.name = "Main PSU";
         relay1.pin = Constants::DEFAULT_RELAY1_PIN;
         relay1.visible = false;
         relay1.hasDependency = false;
         _relayConfigs[1] = relay1;
         
         // Relay 2 - UV Light
         RelayConfig relay2;
         relay2.relayId = 2;
         relay2.name = "UV Light";
         relay2.pin = Constants::DEFAULT_RELAY2_PIN;
         relay2.visible = true;
         relay2.hasDependency = true;
         relay2.dependsOnRelay = 1;
         _relayConfigs[2] = relay2;
         
         // Relay 3 - Grow Light
         RelayConfig relay3;
         relay3.relayId = 3;
         relay3.name = "Grow Light";
         relay3.pin = Constants::DEFAULT_RELAY3_PIN;
         relay3.visible = true;
         relay3.hasDependency = false;
         _relayConfigs[3] = relay3;
         
         // Relay 4 - Tub Fans
         RelayConfig relay4;
         relay4.relayId = 4;
         relay4.name = "Tub Fans";
         relay4.pin = Constants::DEFAULT_RELAY4_PIN;
         relay4.visible = true;
         relay4.hasDependency = true;
         relay4.dependsOnRelay = 1;
         _relayConfigs[4] = relay4;
         
         // Relay 5 - Humidifier
         RelayConfig relay5;
         relay5.relayId = 5;
         relay5.name = "Humidifier";
         relay5.pin = Constants::DEFAULT_RELAY5_PIN;
         relay5.visible = true;
         relay5.hasDependency = true;
         relay5.dependsOnRelay = 1;
         _relayConfigs[5] = relay5;
         
         // Relay 6 - Heater
         RelayConfig relay6;
         relay6.relayId = 6;
         relay6.name = "Heater";
         relay6.pin = Constants::DEFAULT_RELAY6_PIN;
         relay6.visible = true;
         relay6.hasDependency = false;
         _relayConfigs[6] = relay6;
         
         // Relay 7 - IN/OUT Fans
         RelayConfig relay7;
         relay7.relayId = 7;
         relay7.name = "IN/OUT Fans";
         relay7.pin = Constants::DEFAULT_RELAY7_PIN;
         relay7.visible = true;
         relay7.hasDependency = true;
         relay7.dependsOnRelay = 1;
         _relayConfigs[7] = relay7;
         
         // Relay 8 - Reserved
         RelayConfig relay8;
         relay8.relayId = 8;
         relay8.name = "Reserved";
         relay8.pin = Constants::DEFAULT_RELAY8_PIN;
         relay8.visible = false;
         relay8.hasDependency = false;
         _relayConfigs[8] = relay8;
         
         // Initialize all relay pins
         for (auto& entry : _relayConfigs) {
             pinMode(entry.second.pin, OUTPUT);
             digitalWrite(entry.second.pin, LOW);
             entry.second.isOn = false;
             entry.second.state = RelayState::AUTO;
             entry.second.lastTrigger = RelayTrigger::MANUAL;
             
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Relays", 
                 "Initialized relay " + String(entry.first) + " (" + entry.second.name + ") on pin " + String(entry.second.pin));
         }
         
         // Release mutex
         xSemaphoreGive(_relayMutex);
         
         return true;
     }
     
     return false;
 }
 
 bool RelayManager::setRelayPin(uint8_t relayId, uint8_t pin) {
     if (relayId < 1 || relayId > 8) {
         return false;
     }
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_relayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Get current pin
         uint8_t currentPin = _relayConfigs[relayId].pin;
         
         // Set pin to LOW before changing
         if (currentPin != pin) {
             digitalWrite(currentPin, LOW);
             
             // Configure new pin
             _relayConfigs[relayId].pin = pin;
             pinMode(pin, OUTPUT);
             digitalWrite(pin, _relayConfigs[relayId].isOn ? HIGH : LOW);
             
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Relays", 
                 "Changed relay " + String(relayId) + " (" + _relayConfigs[relayId].name + ") from pin " + 
                 String(currentPin) + " to pin " + String(pin));
         }
         
         // Release mutex
         xSemaphoreGive(_relayMutex);
         return true;
     }
     
     return false;
 }
 
 uint8_t RelayManager::getRelayPin(uint8_t relayId) {
     if (relayId < 1 || relayId > 8) {
         return 0;
     }
     
     uint8_t pin = 0;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_relayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         pin = _relayConfigs[relayId].pin;
         
         // Release mutex
         xSemaphoreGive(_relayMutex);
     }
     
     return pin;
 }
 
 bool RelayManager::setRelayName(uint8_t relayId, const String& name) {
     if (relayId < 1 || relayId > 8) {
         return false;
     }
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_relayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Update name
         _relayConfigs[relayId].name = name;
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Relays", 
             "Renamed relay " + String(relayId) + " to \"" + name + "\"");
         
         // Release mutex
         xSemaphoreGive(_relayMutex);
         return true;
     }
     
     return false;
 }
 
 String RelayManager::getRelayName(uint8_t relayId) {
     if (relayId < 1 || relayId > 8) {
         return "";
     }
     
     String name = "";
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_relayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         name = _relayConfigs[relayId].name;
         
         // Release mutex
         xSemaphoreGive(_relayMutex);
     }
     
     return name;
 }
 
 bool RelayManager::setRelayOperatingTime(uint8_t relayId, uint8_t startHour, uint8_t startMinute, 
                                         uint8_t endHour, uint8_t endMinute) {
     if (relayId < 1 || relayId > 8) {
         return false;
     }
     
     // Validate time values
     if (startHour > 23 || startMinute > 59 || endHour > 23 || endMinute > 59) {
         return false;
     }
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_relayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Update operating time
         _relayConfigs[relayId].operatingTime.startHour = startHour;
         _relayConfigs[relayId].operatingTime.startMinute = startMinute;
         _relayConfigs[relayId].operatingTime.endHour = endHour;
         _relayConfigs[relayId].operatingTime.endMinute = endMinute;
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Relays", 
             "Set operating time for relay " + String(relayId) + " (" + _relayConfigs[relayId].name + 
             ") to " + _relayConfigs[relayId].operatingTime.toString());
         
         // Release mutex
         xSemaphoreGive(_relayMutex);
         return true;
     }
     
     return false;
 }
 
 TimeRange RelayManager::getRelayOperatingTime(uint8_t relayId) {
     TimeRange timeRange;
     
     if (relayId < 1 || relayId > 8) {
         return timeRange;
     }
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_relayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         timeRange = _relayConfigs[relayId].operatingTime;
         
         // Release mutex
         xSemaphoreGive(_relayMutex);
     }
     
     return timeRange;
 }
 
 bool RelayManager::setRelayVisibility(uint8_t relayId, bool visible) {
     if (relayId < 1 || relayId > 8) {
         return false;
     }
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_relayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Update visibility
         _relayConfigs[relayId].visible = visible;
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Relays", 
             "Set relay " + String(relayId) + " (" + _relayConfigs[relayId].name + 
             ") visibility to " + (visible ? "visible" : "hidden"));
         
         // Release mutex
         xSemaphoreGive(_relayMutex);
         return true;
     }
     
     return false;
 }
 
 bool RelayManager::getRelayVisibility(uint8_t relayId) {
     if (relayId < 1 || relayId > 8) {
         return false;
     }
     
     bool visible = false;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_relayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         visible = _relayConfigs[relayId].visible;
         
         // Release mutex
         xSemaphoreGive(_relayMutex);
     }
     
     return visible;
 }
 
 bool RelayManager::setRelayDependency(uint8_t relayId, bool hasDependency, uint8_t dependsOnRelay) {
     if (relayId < 1 || relayId > 8) {
         return false;
     }
     
     if (hasDependency && (dependsOnRelay < 1 || dependsOnRelay > 8 || dependsOnRelay == relayId)) {
         return false;
     }
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_relayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Update dependency
         _relayConfigs[relayId].hasDependency = hasDependency;
         _relayConfigs[relayId].dependsOnRelay = dependsOnRelay;
         
         if (hasDependency) {
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Relays", 
                 "Set relay " + String(relayId) + " (" + _relayConfigs[relayId].name + 
                 ") to depend on relay " + String(dependsOnRelay) + 
                 " (" + _relayConfigs[dependsOnRelay].name + ")");
         } else {
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Relays", 
                 "Set relay " + String(relayId) + " (" + _relayConfigs[relayId].name + 
                 ") to have no dependencies");
         }
         
         // Release mutex
         xSemaphoreGive(_relayMutex);
         return true;
     }
     
     return false;
 }
 
 bool RelayManager::getRelayDependency(uint8_t relayId, bool& hasDependency, uint8_t& dependsOnRelay) {
     if (relayId < 1 || relayId > 8) {
         return false;
     }
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_relayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         hasDependency = _relayConfigs[relayId].hasDependency;
         dependsOnRelay = _relayConfigs[relayId].dependsOnRelay;
         
         // Release mutex
         xSemaphoreGive(_relayMutex);
         return true;
     }
     
     return false;
 }
 
 bool RelayManager::setCycleConfig(uint16_t onDurationMinutes, uint16_t intervalMinutes) {
     if (onDurationMinutes >= intervalMinutes) {
         return false;
     }
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_relayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Update cycle configuration
         _cycleConfig.onDurationMinutes = onDurationMinutes;
         _cycleConfig.intervalMinutes = intervalMinutes;
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Relays", 
             "Set cycle configuration to " + String(onDurationMinutes) + 
             " minutes ON every " + String(intervalMinutes) + " minutes");
         
         // Release mutex
         xSemaphoreGive(_relayMutex);
         return true;
     }
     
     return false;
 }
 
 bool RelayManager::getCycleConfig(uint16_t& onDurationMinutes, uint16_t& intervalMinutes) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_relayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         onDurationMinutes = _cycleConfig.onDurationMinutes;
         intervalMinutes = _cycleConfig.intervalMinutes;
         
         // Release mutex
         xSemaphoreGive(_relayMutex);
         return true;
     }
     
     return false;
 }
 
 bool RelayManager::setEnvironmentalThresholds(float humidityLow, float humidityHigh,
                                              float temperatureLow, float temperatureHigh,
                                              float co2Low, float co2High) {
     // Validate thresholds
     if (humidityLow >= humidityHigh || temperatureLow >= temperatureHigh || co2Low >= co2High) {
         return false;
     }
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_relayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Update thresholds
         _thresholds.humidityLow = humidityLow;
         _thresholds.humidityHigh = humidityHigh;
         _thresholds.temperatureLow = temperatureLow;
         _thresholds.temperatureHigh = temperatureHigh;
         _thresholds.co2Low = co2Low;
         _thresholds.co2High = co2High;
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Relays", 
             "Set environmental thresholds: Humidity=" + String(humidityLow, 1) + 
             "%-" + String(humidityHigh, 1) + "%, Temperature=" + String(temperatureLow, 1) + 
             "°C-" + String(temperatureHigh, 1) + "°C, CO2=" + String(co2Low, 0) + 
             "-" + String(co2High, 0) + "ppm");
         
         // Release mutex
         xSemaphoreGive(_relayMutex);
         return true;
     }
     
     return false;
 }
 
 bool RelayManager::getEnvironmentalThresholds(float& humidityLow, float& humidityHigh,
                                              float& temperatureLow, float& temperatureHigh,
                                              float& co2Low, float& co2High) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_relayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         humidityLow = _thresholds.humidityLow;
         humidityHigh = _thresholds.humidityHigh;
         temperatureLow = _thresholds.temperatureLow;
         temperatureHigh = _thresholds.temperatureHigh;
         co2Low = _thresholds.co2Low;
         co2High = _thresholds.co2High;
         
         // Release mutex
         xSemaphoreGive(_relayMutex);
         return true;
     }
     
     return false;
 }
 
 bool RelayManager::setOverrideDuration(uint16_t minutes) {
     if (minutes == 0) {
         return false;
     }
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_relayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         _overrideDurationMinutes = minutes;
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Relays", 
             "Set user override duration to " + String(minutes) + " minutes");
         
         // Release mutex
         xSemaphoreGive(_relayMutex);
         return true;
     }
     
     return false;
 }
 
 uint16_t RelayManager::getOverrideDuration() {
     uint16_t duration = Constants::DEFAULT_USER_OVERRIDE_TIME_MIN;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_relayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         duration = _overrideDurationMinutes;
         
         // Release mutex
         xSemaphoreGive(_relayMutex);
     }
     
     return duration;
 }
 
 bool RelayManager::setRelayState(uint8_t relayId, RelayState state) {
     if (relayId < 1 || relayId > 8) {
         return false;
     }
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_relayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         _relayConfigs[relayId].state = state;
         
         // If manual override, set the expiration time
         if (state != RelayState::AUTO) {
             uint32_t overrideDurationMs = _overrideDurationMinutes * 60 * 1000;
             _relayConfigs[relayId].overrideUntil = millis() + overrideDurationMs;
             
             // Immediately apply the manual state
             bool turnOn = (state == RelayState::ON);
             
             // First check if this relay has dependencies
             if (turnOn && _relayConfigs[relayId].hasDependency) {
                 // Check if the dependency is on
                 uint8_t dependsOnRelay = _relayConfigs[relayId].dependsOnRelay;
                 if (!_relayConfigs[dependsOnRelay].isOn) {
                     // Turn on the dependency first
                     physicallyControlRelay(dependsOnRelay, true, RelayTrigger::DEPENDENT);
                 }
             }
             
             // Now control this relay
             physicallyControlRelay(relayId, turnOn, RelayTrigger::MANUAL);
             
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Relays", 
                 "Manual override for relay " + String(relayId) + " (" + _relayConfigs[relayId].name + 
                 "), set to " + (turnOn ? "ON" : "OFF") + " for " + String(_overrideDurationMinutes) + " minutes");
         } else {
             // Clear override, return to automatic control
             _relayConfigs[relayId].overrideUntil = 0;
             
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Relays", 
                 "Relay " + String(relayId) + " (" + _relayConfigs[relayId].name + 
                 ") set to AUTO mode");
         }
         
         // Release mutex
         xSemaphoreGive(_relayMutex);
         return true;
     }
     
     return false;
 }
 
 RelayState RelayManager::getRelayState(uint8_t relayId) {
     if (relayId < 1 || relayId > 8) {
         return RelayState::OFF;
     }
     
     RelayState state = RelayState::OFF;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_relayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         state = _relayConfigs[relayId].state;
         
         // Check if manual override has expired
         if (state != RelayState::AUTO && _relayConfigs[relayId].overrideUntil > 0) {
             if (millis() > _relayConfigs[relayId].overrideUntil) {
                 // Override expired, set back to auto
                 _relayConfigs[relayId].state = RelayState::AUTO;
                 _relayConfigs[relayId].overrideUntil = 0;
                 state = RelayState::AUTO;
                 
                 getAppCore()->getLogManager()->log(LogLevel::INFO, "Relays", 
                     "Manual override for relay " + String(relayId) + " (" + _relayConfigs[relayId].name + 
                     ") expired, returning to AUTO mode");
             }
         }
         
         // Release mutex
         xSemaphoreGive(_relayMutex);
     }
     
     return state;
 }
 
 bool RelayManager::isRelayOn(uint8_t relayId) {
     if (relayId < 1 || relayId > 8) {
         return false;
     }
     
     bool isOn = false;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_relayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         isOn = _relayConfigs[relayId].isOn;
         
         // Release mutex
         xSemaphoreGive(_relayMutex);
     }
     
     return isOn;
 }
 
 RelayTrigger RelayManager::getRelayLastTrigger(uint8_t relayId) {
     if (relayId < 1 || relayId > 8) {
         return RelayTrigger::MANUAL;
     }
     
     RelayTrigger trigger = RelayTrigger::MANUAL;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_relayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         trigger = _relayConfigs[relayId].lastTrigger;
         
         // Release mutex
         xSemaphoreGive(_relayMutex);
     }
     
     return trigger;
 }
 
 std::vector<RelayConfig> RelayManager::getAllRelayConfigs() {
     std::vector<RelayConfig> configs;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_relayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         for (const auto& entry : _relayConfigs) {
             configs.push_back(entry.second);
         }
         
         // Release mutex
         xSemaphoreGive(_relayMutex);
     }
     
     return configs;
 }
 
 void RelayManager::createTasks() {
     // Create relay control task
     BaseType_t result = xTaskCreatePinnedToCore(
         relayControlTask,                   // Task function
         "RelayControlTask",                 // Task name
         Constants::STACK_SIZE_RELAY_CONTROL,// Stack size (words)
         this,                               // Task parameters
         Constants::PRIORITY_RELAY_CONTROL,  // Priority
         &_relayControlTaskHandle,           // Task handle
         1                                   // Core ID (1 - application core)
     );
     
     if (result != pdPASS) {
         getAppCore()->getLogManager()->log(LogLevel::ERROR, "Relays", 
             "Failed to create relay control task");
     }
 }
 
 bool RelayManager::physicallyControlRelay(uint8_t relayId, bool turnOn, RelayTrigger trigger) {
     if (relayId < 1 || relayId > 8) {
         return false;
     }
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_relayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Get current state
         bool currentlyOn = _relayConfigs[relayId].isOn;
         
         // Only take action if state is changing
         if (currentlyOn != turnOn) {
             // Control the physical pin
             digitalWrite(_relayConfigs[relayId].pin, turnOn ? HIGH : LOW);
             
             // Update state
             _relayConfigs[relayId].isOn = turnOn;
             _relayConfigs[relayId].lastTrigger = trigger;
             
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Relays", 
                 "Relay " + String(relayId) + " (" + _relayConfigs[relayId].name + 
                 ") turned " + (turnOn ? "ON" : "OFF") + " by " + 
                 (trigger == RelayTrigger::MANUAL ? "manual control" : 
                  trigger == RelayTrigger::SCHEDULE ? "schedule" : 
                  trigger == RelayTrigger::ENVIRONMENTAL ? "environmental control" : "dependency"));
             
             // If turning off, check for dependent relays
             if (!turnOn) {
                 manageDependentRelays(relayId, turnOn);
             }
         }
         
         // Release mutex
         xSemaphoreGive(_relayMutex);
         return true;
     }
     
     return false;
 }
 
 bool RelayManager::checkDependencyChain(uint8_t relayId) {
     if (relayId < 1 || relayId > 8) {
         return false;
     }
     
     // If this relay doesn't have a dependency, it's always valid
     if (!_relayConfigs[relayId].hasDependency) {
         return true;
     }
     
     // Check if the relay it depends on is on
     uint8_t dependsOnRelay = _relayConfigs[relayId].dependsOnRelay;
     return _relayConfigs[dependsOnRelay].isOn;
 }
 
 bool RelayManager::isInOperatingTime(uint8_t relayId) {
     if (relayId < 1 || relayId > 8) {
         return false;
     }
     
     // Get current time
     time_t now;
     time(&now);
     struct tm timeInfo;
     localtime_r(&now, &timeInfo);
     
     // Check if current time is within operating time
     return _relayConfigs[relayId].operatingTime.isInRange(timeInfo.tm_hour, timeInfo.tm_min);
 }
 
 void RelayManager::manageDependentRelays(uint8_t relayId, bool turnOn) {
     // If turning on a relay, dependencies were already checked
     if (turnOn) {
         return;
     }
     
     // When turning off a relay, check if any other relays depend on it
     for (auto& entry : _relayConfigs) {
         if (entry.second.hasDependency && entry.second.dependsOnRelay == relayId && entry.second.isOn) {
             // This relay depends on the one being turned off, so turn it off too
             physicallyControlRelay(entry.first, false, RelayTrigger::DEPENDENT);
         }
     }
     
     // Special case for Relay 1 (Main PSU)
     if (relayId == 1) {
         // When turning off Relay 1, check if any other relays need it
         bool anyDependentOn = false;
         for (const auto& entry : _relayConfigs) {
             if (entry.first != 1 && entry.second.hasDependency && 
                 entry.second.dependsOnRelay == 1 && entry.second.isOn) {
                 anyDependentOn = true;
                 break;
             }
         }
         
         // If no dependent relays are on, it's safe to turn off Relay 1
         if (!anyDependentOn) {
             physicallyControlRelay(1, false, RelayTrigger::DEPENDENT);
         }
     }
 }
 
 void RelayManager::relayControlTask(void* parameter) {
     RelayManager* relayManager = static_cast<RelayManager*>(parameter);
     TickType_t lastWakeTime = xTaskGetTickCount();
     
     // Get reference to the sensor manager for environmental readings
     SensorManager* sensorManager = getAppCore()->getSensorManager();
     
     while (true) {
         // Get current time
         time_t now;
         time(&now);
         struct tm timeInfo;
         localtime_r(&now, &timeInfo);
         
         // Determine what minute of the day we're in (0-1439)
         uint16_t currentMinuteOfDay = timeInfo.tm_hour * 60 + timeInfo.tm_min;
         
         // Get sensor readings
         SensorReading upperDht, lowerDht, scd;
         bool hasReadings = sensorManager->getSensorReadings(upperDht, lowerDht, scd);
         
         // Calculate average temperature and humidity if we have readings
         float avgTemperature = 0.0f;
         float avgHumidity = 0.0f;
         uint16_t validSensors = 0;
         
         if (hasReadings) {
             if (upperDht.valid) {
                 avgTemperature += upperDht.temperature;
                 avgHumidity += upperDht.humidity;
                 validSensors++;
             }
             
             if (lowerDht.valid) {
                 avgTemperature += lowerDht.temperature;
                 avgHumidity += lowerDht.humidity;
                 validSensors++;
             }
             
             if (scd.valid) {
                 avgTemperature += scd.temperature;
                 avgHumidity += scd.humidity;
                 validSensors++;
             }
             
             if (validSensors > 0) {
                 avgTemperature /= validSensors;
                 avgHumidity /= validSensors;
             }
         }
         
         // Control relays based on their type and conditions
         for (uint8_t relayId = 1; relayId <= 8; relayId++) {
             // Take mutex to ensure thread safety
             if (xSemaphoreTake(relayManager->_relayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                 // Skip if not initialized
                 if (!relayManager->_isInitialized) {
                     xSemaphoreGive(relayManager->_relayMutex);
                     continue;
                 }
                 
                 // Check if relay is in manual mode
                 RelayState state = relayManager->getRelayState(relayId);
                 if (state != RelayState::AUTO) {
                     // Check for override expiration is done in getRelayState
                     xSemaphoreGive(relayManager->_relayMutex);
                     continue;
                 }
                 
                 // Get relay config
                 RelayConfig& config = relayManager->_relayConfigs[relayId];
                 
                 // Check if we're in operating time range
                 bool inOperatingTime = relayManager->isInOperatingTime(relayId);
                 
                 // Release mutex for reading sensors, etc.
                 xSemaphoreGive(relayManager->_relayMutex);
                 
                 // Control logic varies by relay ID
                 switch (relayId) {
                     case 1: // Main PSU
                         // Only turn on if needed by other relays
                         // This is handled by dependency management
                         break;
                         
                     case 2: // UV Light
                         // Alternate with Grow Light (Relay 3) in operating time
                         if (inOperatingTime) {
                             // Check if we're in the "on" phase of the cycle
                             uint16_t cycleMinutes = currentMinuteOfDay % relayManager->_cycleConfig.intervalMinutes;
                             bool shouldBeOn = (cycleMinutes < relayManager->_cycleConfig.onDurationMinutes);
                             
                             // Only turn on in the first half of the interval
                             if (shouldBeOn) {
                                 // Check dependency
                                 if (config.hasDependency) {
                                     uint8_t dependsOnRelay = config.dependsOnRelay;
                                     if (!relayManager->_relayConfigs[dependsOnRelay].isOn) {
                                         // Turn on the dependency first
                                         relayManager->physicallyControlRelay(dependsOnRelay, true, RelayTrigger::DEPENDENT);
                                     }
                                 }
                                 relayManager->physicallyControlRelay(relayId, true, RelayTrigger::SCHEDULE);
                             } else {
                                 relayManager->physicallyControlRelay(relayId, false, RelayTrigger::SCHEDULE);
                             }
                         } else {
                             relayManager->physicallyControlRelay(relayId, false, RelayTrigger::SCHEDULE);
                         }
                         break;
                         
                     case 3: // Grow Light
                         // Alternate with UV Light (Relay 2) in operating time
                         if (inOperatingTime) {
                             // Check if we're in the "on" phase of the cycle
                             uint16_t cycleMinutes = currentMinuteOfDay % relayManager->_cycleConfig.intervalMinutes;
                             bool shouldBeOn = (cycleMinutes >= relayManager->_cycleConfig.onDurationMinutes);
                             
                             // Only turn on in the second half of the interval
                             if (shouldBeOn) {
                                 relayManager->physicallyControlRelay(relayId, true, RelayTrigger::SCHEDULE);
                             } else {
                                 relayManager->physicallyControlRelay(relayId, false, RelayTrigger::SCHEDULE);
                             }
                         } else {
                             relayManager->physicallyControlRelay(relayId, false, RelayTrigger::SCHEDULE);
                         }
                         break;
                         
                     case 4: // Tub Fans
                         // Run based on CO2 level or cycle in operating time
                         if (inOperatingTime) {
                             bool shouldBeOn = false;
                             
                             // Check cycle timing
                             uint16_t cycleMinutes = currentMinuteOfDay % relayManager->_cycleConfig.intervalMinutes;
                             if (cycleMinutes < relayManager->_cycleConfig.onDurationMinutes) {
                                 shouldBeOn = true;
                             }
                             
                             // Also check CO2 level
                             if (scd.valid && scd.co2 < relayManager->_thresholds.co2Low) {
                                 shouldBeOn = true;
                             }
                             
                             // Run together with humidifier (Relay 5)
                             if (relayManager->_relayConfigs[5].isOn) {
                                 shouldBeOn = true;
                             }
                             
                             if (shouldBeOn) {
                                 // Check dependency
                                 if (config.hasDependency) {
                                     uint8_t dependsOnRelay = config.dependsOnRelay;
                                     if (!relayManager->_relayConfigs[dependsOnRelay].isOn) {
                                         // Turn on the dependency first
                                         relayManager->physicallyControlRelay(dependsOnRelay, true, RelayTrigger::DEPENDENT);
                                     }
                                 }
                                 relayManager->physicallyControlRelay(relayId, true, RelayTrigger::SCHEDULE);
                             } else {
                                 relayManager->physicallyControlRelay(relayId, false, RelayTrigger::SCHEDULE);
                             }
                         } else {
                             relayManager->physicallyControlRelay(relayId, false, RelayTrigger::SCHEDULE);
                         }
                         break;
                         
                     case 5: // Humidifier
                         // Run based on humidity level in operating time
                         if (inOperatingTime && validSensors > 0) {
                             bool shouldBeOn = false;
                             
                             // Check humidity level
                             if (avgHumidity < relayManager->_thresholds.humidityLow) {
                                 shouldBeOn = true;
                             } else if (avgHumidity >= relayManager->_thresholds.humidityHigh) {
                                 shouldBeOn = false;
                             } else {
                                 // Between thresholds, maintain current state
                                 shouldBeOn = config.isOn;
                             }
                             
                             // Run together with IN/OUT Fans (Relay 7)
                             if (relayManager->_relayConfigs[7].isOn) {
                                 shouldBeOn = true;
                             }
                             
                             if (shouldBeOn) {
                                 // Check dependency
                                 if (config.hasDependency) {
                                     uint8_t dependsOnRelay = config.dependsOnRelay;
                                     if (!relayManager->_relayConfigs[dependsOnRelay].isOn) {
                                         // Turn on the dependency first
                                         relayManager->physicallyControlRelay(dependsOnRelay, true, RelayTrigger::DEPENDENT);
                                     }
                                 }
                                 relayManager->physicallyControlRelay(relayId, true, RelayTrigger::ENVIRONMENTAL);
                             } else {
                                 relayManager->physicallyControlRelay(relayId, false, RelayTrigger::ENVIRONMENTAL);
                             }
                         } else {
                             relayManager->physicallyControlRelay(relayId, false, RelayTrigger::SCHEDULE);
                         }
                         break;
                         
                     case 6: // Heater
                         // Run based on temperature level
                         if (validSensors > 0) {
                             bool shouldBeOn = false;
                             
                             // Check temperature level
                             if (avgTemperature < relayManager->_thresholds.temperatureLow) {
                                 shouldBeOn = true;
                             } else if (avgTemperature >= relayManager->_thresholds.temperatureHigh) {
                                 shouldBeOn = false;
                             } else {
                                 // Between thresholds, maintain current state
                                 shouldBeOn = config.isOn;
                             }
                             
                             if (shouldBeOn) {
                                 relayManager->physicallyControlRelay(relayId, true, RelayTrigger::ENVIRONMENTAL);
                             } else {
                                 relayManager->physicallyControlRelay(relayId, false, RelayTrigger::ENVIRONMENTAL);
                             }
                         }
                         break;
                         
                     case 7: // IN/OUT Fans
                         // Run based on CO2 level or cycle in operating time
                         if (inOperatingTime) {
                             bool shouldBeOn = false;
                             
                             // Check cycle timing
                             uint16_t cycleMinutes = currentMinuteOfDay % relayManager->_cycleConfig.intervalMinutes;
                             if (cycleMinutes < relayManager->_cycleConfig.onDurationMinutes) {
                                 shouldBeOn = true;
                             }
                             
                             // Also check CO2 level
                             if (scd.valid) {
                                 if (scd.co2 > relayManager->_thresholds.co2High) {
                                     shouldBeOn = true;
                                 } else if (scd.co2 < relayManager->_thresholds.co2Low && shouldBeOn) {
                                     // Only turn off due to CO2 if it's already on due to cycling
                                     shouldBeOn = false;
                                 }
                             }
                             
                             if (shouldBeOn) {
                                 // Check dependency
                                 if (config.hasDependency) {
                                     uint8_t dependsOnRelay = config.dependsOnRelay;
                                     if (!relayManager->_relayConfigs[dependsOnRelay].isOn) {
                                         // Turn on the dependency first
                                         relayManager->physicallyControlRelay(dependsOnRelay, true, RelayTrigger::DEPENDENT);
                                     }
                                 }
                                 relayManager->physicallyControlRelay(relayId, true, RelayTrigger::ENVIRONMENTAL);
                             } else {
                                 relayManager->physicallyControlRelay(relayId, false, RelayTrigger::ENVIRONMENTAL);
                             }
                         } else {
                             relayManager->physicallyControlRelay(relayId, false, RelayTrigger::SCHEDULE);
                         }
                         break;
                         
                     case 8: // Reserved
                         // No automated control for reserved relay
                         break;
                 }
             }
         }
         
         // Wait for the next control period (check every 10 seconds)
         vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(10000));
     }
 }
                 /**
  * @file RelayManager.cpp
  * @brief Implementation of the RelayManager class
  */
 
 #include "RelayManager.h"
 #include "../core/AppCore.h"
 #include "../system/LogManager.h"
 #include "../components/SensorManager.h"
 #include <time.h>
 
 RelayManager::RelayManager() :
     _overrideDurationMinutes(Constants::DEFAULT_USER_OVERRIDE_TIME_MIN),
     _relayMutex(nullptr),
     _relayControlTaskHandle(nullptr),
     _isInitialized(false)
 {
 }
 
 RelayManager::~RelayManager() {
     // Clean up RTOS resources
     if (_relayMutex != nullptr) {
         vSemaphoreDelete(_relayMutex);
     }
     
     if (_relayControlTaskHandle != nullptr) {
         vTaskDelete(_relayControlTaskHandle);
     }
     
     // Ensure all relays are off
     for (const auto& entry : _relayConfigs) {
         pinMode(entry.second.pin, OUTPUT);
         digitalWrite(entry.second.pin, LOW);
     }
 }
 
 bool RelayManager::begin() {
     // Create mutex for thread-safe operations
     _relayMutex = xSemaphoreCreateMutex();
     if (_relayMutex == nullptr) {
         Serial.println("Failed to create relay mutex!");
         return false;
     }
     
     _isInitialized = true;
     return true;
 }
 
 bool RelayManager::initRelays() {
     getAppCore()->getLogManager()->log(LogLevel::INFO, "Relays", "Initializing relays");
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_relayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Initialize default relay configurations
         
         // Relay 1 - Main PSU
         RelayConfig relay1;
         relay1.relayId = 1;
         relay1.name = "Main PSU";
         relay1.pin = Constants::DEFAULT_RELAY1_PIN;
         relay1.visible = false;
         relay1.hasDependency = false;
         _relayConfigs[1] = relay1;
         
         // Relay 2 - UV Light
         RelayConfig relay2;
         relay2.relayId = 2;
         relay2.name = "UV Light";
         relay2.pin = Constants::DEFAULT_RELAY2_PIN;
         relay2.visible = true;
         relay2.hasDependency = true;
         relay2.dependsOnRelay = 1;
         _relayConfigs[2] = relay2;
         
         // Relay 3 - Grow Light
         RelayConfig relay3;
         relay3.relayId = 3;
         relay3.name = "Grow Light";
         relay3.pin = Constants::DEFAULT_RELAY3_PIN;
         relay3.visible = true;
         relay3.hasDependency = false;
         _relayConfigs[3] = relay3;
         
         // Relay 4 - Tub Fans
         RelayConfig relay4;
         relay4.relayId = 4;
         relay4.name = "Tub Fans";
         relay4.pin = Constants::DEFAULT_RELAY4_PIN;
         relay4.visible = true;
         relay4.hasDependency = true;
         relay4.dependsOnRelay = 1;
         _relayConfigs[4] = relay4;
         
         // Relay 5 - Humidifier
         RelayConfig relay5;
         relay5.relayId = 5;
         relay5.name = "Humidifier";
         relay5.pin = Constants::DEFAULT_RELAY5_PIN;
         relay5.visible = true;
         relay5.hasDependency = true;
         relay5.dependsOnRelay = 1;
         _relayConfigs[5] = relay5;
         
         // Relay 6 - Heater
         RelayConfig relay6;
         relay6.relayId = 6;
         relay6.name = "Heater";
         relay6.pin = Constants::DEFAULT_RELAY6_PIN;
         relay6.visible = true;
         relay6.hasDependency = false;
         _relayConfigs[6] = relay6;
         
         // Relay 7 - IN/OUT Fans
         RelayConfig relay7;
         relay7.relayId = 7;
         relay7.name = "IN/OUT Fans";
         relay7.pin = Constants::DEFAULT_RELAY7_PIN;
         relay7.visible = true;
         relay7.hasDependency = true;
         relay7.dependsOnRelay = 1;
         _relayConfigs[7] = relay7;
         
         // Relay 8 - Reserved
         RelayConfig relay8;
         relay8.relayId = 8;
         relay8.name = "Reserved";
         relay8.pin = Constants::DEFAULT_RELAY8_PIN;
         relay8.visible = false;
         relay8.hasDependency = false;
         _relayConfigs[8] = relay8;
         
         // Initialize all relay pins
         for (auto& entry : _relayConfigs) {
             pinMode(entry.second.pin, OUTPUT);
             digitalWrite(entry.second.pin, LOW);
             entry.second.isOn = false;
             entry.second.state = RelayState::AUTO;
             entry.second.lastTrigger = RelayTrigger::MANUAL;
             
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Relays", 
                 "Initialized relay " + String(entry.first) + " (" + entry.second.name + ") on pin " + String(entry.second.pin));
         }
         
         // Release mutex
         xSemaphoreGive(_relayMutex);
         
         return true;
     }
     
     return false;
 }
 
 bool RelayManager::setRelayPin(uint8_t relayId, uint8_t pin) {
     if (relayId < 1 || relayId > 8) {
         return false;
     }
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_relayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Get current pin
         uint8_t currentPin = _relayConfigs[relayId].pin;
         
         // Set pin to LOW before changing
         if (currentPin != pin) {
             digitalWrite(currentPin, LOW);
             
             // Configure new pin
             _relayConfigs[relayId].pin = pin;
             pinMode(pin, OUTPUT);
             digitalWrite(pin, _relayConfigs[relayId].isOn ? HIGH : LOW);
             
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Relays", 
                 "Changed relay " + String(