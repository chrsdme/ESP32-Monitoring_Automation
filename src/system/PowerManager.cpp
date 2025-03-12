/**
 * @file PowerManager.cpp
 * @brief Implementation of the PowerManager class
 */

 #include "PowerManager.h"
 #include "../core/AppCore.h"
 #include "../system/LogManager.h"
 #include "../system/TimeManager.h"
 
 PowerManager::PowerManager() :
     _currentMode(PowerMode::NO_SLEEP),
     _isWiFiEnabled(true),
     _isBluetoothEnabled(false),
     _isScheduleActive(false),
     _powerMutex(nullptr),
     _powerTaskHandle(nullptr)
 {
 }
 
 PowerManager::~PowerManager() {
     // Clean up RTOS resources
     if (_powerMutex != nullptr) {
         vSemaphoreDelete(_powerMutex);
     }
     
     if (_powerTaskHandle != nullptr) {
         vTaskDelete(_powerTaskHandle);
     }
 }
 
 bool PowerManager::begin() {
     // Create mutex for thread-safe operations
     _powerMutex = xSemaphoreCreateMutex();
     if (_powerMutex == nullptr) {
         Serial.println("Failed to create power mutex!");
         return false;
     }
     
     // Load power schedule from NVS if available
     nvs_handle_t nvsHandle;
     esp_err_t err = nvs_open(Constants::NVS_CONFIG_NAMESPACE, NVS_READONLY, &nvsHandle);
     if (err == ESP_OK) {
         uint8_t enabled;
         if (nvs_get_u8(nvsHandle, "power_enabled", &enabled) == ESP_OK) {
             _powerSchedule.enabled = (enabled == 1);
         }
         
         uint8_t mode;
         if (nvs_get_u8(nvsHandle, "power_mode", &mode) == ESP_OK) {
             _powerSchedule.mode = static_cast<PowerMode>(mode);
         }
         
         uint8_t startHour, startMinute, endHour, endMinute;
         if (nvs_get_u8(nvsHandle, "power_start_hour", &startHour) == ESP_OK) {
             _powerSchedule.startHour = startHour;
         }
         
         if (nvs_get_u8(nvsHandle, "power_start_minute", &startMinute) == ESP_OK) {
             _powerSchedule.startMinute = startMinute;
         }
         
         if (nvs_get_u8(nvsHandle, "power_end_hour", &endHour) == ESP_OK) {
             _powerSchedule.endHour = endHour;
         }
         
         if (nvs_get_u8(nvsHandle, "power_end_minute", &endMinute) == ESP_OK) {
             _powerSchedule.endMinute = endMinute;
         }
         
         nvs_close(nvsHandle);
     }
     
     // Bluetooth is off by default to save power
     _isBluetoothEnabled = false;
     esp_bt_controller_disable();
     
     getAppCore()->getLogManager()->log(LogLevel::INFO, "Power", 
         "Power manager initialized");
     
     return true;
 }
 
 bool PowerManager::enterPowerSavingMode(PowerMode mode) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_powerMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Already in this mode
         if (_currentMode == mode) {
             xSemaphoreGive(_powerMutex);
             return true;
         }
         
         bool success = false;
         
         switch (mode) {
             case PowerMode::NO_SLEEP:
                 // Exit any power-saving mode
                 success = exitPowerSavingMode();
                 break;
                 
             case PowerMode::MODEM_SLEEP:
                 success = enterModemSleep();
                 break;
                 
             case PowerMode::LIGHT_SLEEP:
                 success = enterLightSleep();
                 break;
                 
             case PowerMode::DEEP_SLEEP:
                 success = enterDeepSleep();
                 break;
                 
             case PowerMode::HIBERNATION:
                 success = enterHibernation();
                 break;
                 
             default:
                 success = false;
                 break;
         }
         
         if (success) {
             _currentMode = mode;
             
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Power", 
                 "Entered power-saving mode: " + String(static_cast<int>(mode)));
         } else {
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "Power", 
                 "Failed to enter power-saving mode: " + String(static_cast<int>(mode)));
         }
         
         xSemaphoreGive(_powerMutex);
         return success;
     }
     
     return false;
 }
 
 bool PowerManager::exitPowerSavingMode() {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_powerMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         bool success = true;
         
         // Actions depend on current mode
         switch (_currentMode) {
             case PowerMode::NO_SLEEP:
                 // Already not sleeping
                 break;
                 
             case PowerMode::MODEM_SLEEP:
                 // Re-enable WiFi if it was enabled before
                 if (_isWiFiEnabled) {
                     success = setWiFiEnabled(true);
                 }
                 break;
                 
             case PowerMode::LIGHT_SLEEP:
             case PowerMode::DEEP_SLEEP:
             case PowerMode::HIBERNATION:
                 // These modes should have already exited on wake
                 // Just re-enable peripherals as needed
                 if (_isWiFiEnabled) {
                     success = setWiFiEnabled(true);
                 }
                 if (_isBluetoothEnabled) {
                     success = success && setBluetoothEnabled(true);
                 }
                 break;
         }
         
         if (success) {
             _currentMode = PowerMode::NO_SLEEP;
             
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Power", 
                 "Exited power-saving mode");
         } else {
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "Power", 
                 "Failed to exit power-saving mode");
         }
         
         xSemaphoreGive(_powerMutex);
         return success;
     }
     
     return false;
 }
 
 PowerMode PowerManager::getCurrentPowerMode() {
     PowerMode mode;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_powerMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         mode = _currentMode;
         xSemaphoreGive(_powerMutex);
     } else {
         mode = PowerMode::NO_SLEEP;  // Default if mutex fails
     }
     
     return mode;
 }
 
 bool PowerManager::setPowerSchedule(const PowerSchedule& schedule) {
     // Validate schedule times
     if (schedule.startHour > 23 || schedule.startMinute > 59 || 
         schedule.endHour > 23 || schedule.endMinute > 59) {
         return false;
     }
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_powerMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         _powerSchedule = schedule;
         
         // Save to NVS
         nvs_handle_t nvsHandle;
         esp_err_t err = nvs_open(Constants::NVS_CONFIG_NAMESPACE, NVS_READWRITE, &nvsHandle);
         if (err == ESP_OK) {
             nvs_set_u8(nvsHandle, "power_enabled", schedule.enabled ? 1 : 0);
             nvs_set_u8(nvsHandle, "power_mode", static_cast<uint8_t>(schedule.mode));
             nvs_set_u8(nvsHandle, "power_start_hour", schedule.startHour);
             nvs_set_u8(nvsHandle, "power_start_minute", schedule.startMinute);
             nvs_set_u8(nvsHandle, "power_end_hour", schedule.endHour);
             nvs_set_u8(nvsHandle, "power_end_minute", schedule.endMinute);
             nvs_commit(nvsHandle);
             nvs_close(nvsHandle);
         }
         
         // Log the change
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Power", 
             "Power schedule " + String(schedule.enabled ? "enabled" : "disabled") + 
             ", mode " + String(static_cast<int>(schedule.mode)) + 
             ", time " + String(schedule.startHour) + ":" + String(schedule.startMinute) + 
             " to " + String(schedule.endHour) + ":" + String(schedule.endMinute));
         
         xSemaphoreGive(_powerMutex);
         return true;
     }
     
     return false;
 }
 
 PowerSchedule PowerManager::getPowerSchedule() {
     PowerSchedule schedule;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_powerMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         schedule = _powerSchedule;
         xSemaphoreGive(_powerMutex);
     }
     
     return schedule;
 }
 
 bool PowerManager::setWiFiEnabled(bool enable) {
     esp_err_t err;
     
     if (enable) {
         // Enable WiFi
         err = esp_wifi_start();
         if (err != ESP_OK) {
             return false;
         }
         
         // Wait for connection
         uint8_t attempts = 0;
         while (WiFi.status() != WL_CONNECTED && attempts < 10) {
             delay(500);
             attempts++;
         }
         
         _isWiFiEnabled = true;
         return (WiFi.status() == WL_CONNECTED);
     } else {
         // Disable WiFi
         err = esp_wifi_stop();
         if (err != ESP_OK) {
             return false;
         }
         
         _isWiFiEnabled = false;
         return true;
     }
 }
 
 bool PowerManager::setBluetoothEnabled(bool enable) {
     esp_err_t err;
     
     if (enable) {
         // Enable BT controller
         err = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
         if (err != ESP_OK) {
             return false;
         }
         
         _isBluetoothEnabled = true;
         return true;
     } else {
         // Disable BT controller
         err = esp_bt_controller_disable();
         if (err != ESP_OK) {
             return false;
         }
         
         _isBluetoothEnabled = false;
         return true;
     }
 }
 
 bool PowerManager::checkPowerSchedule() {
     if (!_powerSchedule.enabled) {
         return false;
     }
     
     // Check if time manager is available and time is set
     if (!getAppCore()->getTimeManager()->isTimeSet()) {
         return false;
     }
     
     // Check if we're in the scheduled time range
     bool inScheduledRange = getAppCore()->getTimeManager()->isTimeInRange(
         _powerSchedule.startHour, 
         _powerSchedule.startMinute, 
         _powerSchedule.endHour, 
         _powerSchedule.endMinute
     );
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_powerMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // If we're in the scheduled range and not already in the target mode
         if (inScheduledRange && !_isScheduleActive) {
             _isScheduleActive = true;
             xSemaphoreGive(_powerMutex);
             
             // Enter the scheduled power mode
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Power", 
                 "Entering scheduled power-saving mode");
             
             return enterPowerSavingMode(_powerSchedule.mode);
         }
         // If we're outside the scheduled range but were previously in the scheduled mode
         else if (!inScheduledRange && _isScheduleActive) {
             _isScheduleActive = false;
             xSemaphoreGive(_powerMutex);
             
             // Exit power-saving mode
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Power", 
                 "Exiting scheduled power-saving mode");
             
             return exitPowerSavingMode();
         } else {
             xSemaphoreGive(_powerMutex);
             return false;
         }
     }
     
     return false;
 }
 
 void PowerManager::createTasks() {
     // Create power management task
     BaseType_t result = xTaskCreatePinnedToCore(
         powerTask,                   // Task function
         "PowerTask",                 // Task name
         2048,                        // Stack size (words)
         this,                        // Task parameters
         1,                           // Priority (low)
         &_powerTaskHandle,           // Task handle
         0                            // Core ID (0 - protocol core)
     );
     
     if (result != pdPASS) {
         getAppCore()->getLogManager()->log(LogLevel::ERROR, "Power", 
             "Failed to create power management task");
     }
 }
 
 bool PowerManager::enterModemSleep() {
     // In modem sleep, WiFi and BT radios are disabled
     // but CPU and other peripherals remain active
     
     // Remember current WiFi and BT state
     _isWiFiEnabled = (WiFi.status() == WL_CONNECTED);
     
     // Disable WiFi
     if (_isWiFiEnabled) {
         if (!setWiFiEnabled(false)) {
             return false;
         }
     }
     
     // Disable Bluetooth
     if (_isBluetoothEnabled) {
         if (!setBluetoothEnabled(false)) {
             // Try to restore WiFi
             if (_isWiFiEnabled) {
                 setWiFiEnabled(true);
             }
             return false;
         }
     }
     
     return true;
 }
 
 bool PowerManager::enterLightSleep() {
     // In light sleep, CPU is suspended, but WiFi, BT, and peripherals
     // maintain their state (though not operating)
     
     // Configure wake-up sources
     esp_sleep_enable_timer_wakeup(15000000);  // Wake up after 15 seconds
     
     // Enter light sleep
     esp_err_t err = esp_light_sleep_start();
     
     // Return true if sleep was successful
     return (err == ESP_OK);
 }
 
 bool PowerManager::enterDeepSleep(uint64_t sleepTimeUs) {
     // In deep sleep, CPU, WiFi, BT are all powered down
     // Only the RTC remains active
     
     // Save state for when we wake up
     _isWiFiEnabled = (WiFi.status() == WL_CONNECTED);
     
     // Configure wake-up time (defaulting to 1 hour if not specified)
     if (sleepTimeUs == 0) {
         sleepTimeUs = 3600000000;  // 1 hour in microseconds
     }
     
     // Configure wake-up sources
     esp_sleep_enable_timer_wakeup(sleepTimeUs);
     
     // Prepare a message
     getAppCore()->getLogManager()->log(LogLevel::INFO, "Power", 
         "Entering deep sleep for " + String(sleepTimeUs / 1000000) + " seconds");
     
     // Give some time for log to be saved
     delay(500);
     
     // Enter deep sleep
     esp_deep_sleep_start();
     
     // Code will not reach here, as deep sleep restarts the device
     return true;
 }
 
 bool PowerManager::enterHibernation(uint64_t sleepTimeUs) {
     // Hibernation is similar to deep sleep but with even more power savings
     // by disabling the RTC slow memory
     
     // Save state for when we wake up
     _isWiFiEnabled = (WiFi.status() == WL_CONNECTED);
     
     // Configure wake-up time (defaulting to 1 hour if not specified)
     if (sleepTimeUs == 0) {
         sleepTimeUs = 3600000000;  // 1 hour in microseconds
     }
     
     // Configure wake-up sources
     esp_sleep_enable_timer_wakeup(sleepTimeUs);
     
     // Disable RTC slow memory
     esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
     
     // Prepare a message
     getAppCore()->getLogManager()->log(LogLevel::INFO, "Power", 
         "Entering hibernation for " + String(sleepTimeUs / 1000000) + " seconds");
     
     // Give some time for log to be saved
     delay(500);
     
     // Enter deep sleep (with hibernation configuration)
     esp_deep_sleep_start();
     
     // Code will not reach here, as deep sleep restarts the device
     return true;
 }
 
 void PowerManager::powerTask(void* parameter) {
     PowerManager* powerManager = static_cast<PowerManager*>(parameter);
     TickType_t lastWakeTime = xTaskGetTickCount();
     
     // Initial delay to allow system to stabilize
     vTaskDelay(pdMS_TO_TICKS(30000));  // 30 seconds
     
     while (true) {
         // Check if a power mode change is needed
         powerManager->checkPowerSchedule();
         
         // Sleep until next check (every minute)
         vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(60000));
     }
 }