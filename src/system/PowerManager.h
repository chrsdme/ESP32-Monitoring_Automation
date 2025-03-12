/**
 * @file PowerManager.h
 * @brief Manages power-saving modes and power-related functions
 */

 #ifndef POWER_MANAGER_H
 #define POWER_MANAGER_H
 
 #include <Arduino.h>
 #include <freertos/FreeRTOS.h>
 #include <freertos/task.h>
 #include <freertos/semphr.h>
 #include <esp_sleep.h>
 #include <esp_wifi.h>
 #include <esp_bt.h>
 #include "../utils/Constants.h"
 
 // Forward declarations
 class AppCore;
 
 /**
  * @struct PowerSchedule
  * @brief Structure to hold power mode schedule
  */
 struct PowerSchedule {
     bool enabled;
     PowerMode mode;
     uint8_t startHour;
     uint8_t startMinute;
     uint8_t endHour;
     uint8_t endMinute;
     
     PowerSchedule() : 
         enabled(false),
         mode(PowerMode::NO_SLEEP),
         startHour(0), 
         startMinute(0), 
         endHour(0), 
         endMinute(0) {}
     
     PowerSchedule(bool enabled, PowerMode mode, uint8_t startHour, uint8_t startMinute, 
                  uint8_t endHour, uint8_t endMinute) :
         enabled(enabled),
         mode(mode),
         startHour(startHour),
         startMinute(startMinute),
         endHour(endHour),
         endMinute(endMinute) {}
 };
 
 /**
  * @class PowerManager
  * @brief Manages power-saving modes and power-related functions
  */
 class PowerManager {
 public:
     PowerManager();
     ~PowerManager();
     
     /**
      * @brief Initialize the power manager
      * @return True if initialized successfully
      */
     bool begin();
     
     /**
      * @brief Enter specified power-saving mode
      * @param mode Power-saving mode
      * @return True if mode entered successfully
      */
     bool enterPowerSavingMode(PowerMode mode);
     
     /**
      * @brief Exit current power-saving mode
      * @return True if exited successfully
      */
     bool exitPowerSavingMode();
     
     /**
      * @brief Get current power-saving mode
      * @return Current power mode
      */
     PowerMode getCurrentPowerMode();
     
     /**
      * @brief Set power mode schedule
      * @param schedule Power schedule configuration
      * @return True if schedule set successfully
      */
     bool setPowerSchedule(const PowerSchedule& schedule);
     
     /**
      * @brief Get power mode schedule
      * @return Current power schedule
      */
     PowerSchedule getPowerSchedule();
     
     /**
      * @brief Enable or disable WiFi
      * @param enable True to enable WiFi, false to disable
      * @return True if operation successful
      */
     bool setWiFiEnabled(bool enable);
     
     /**
      * @brief Enable or disable Bluetooth
      * @param enable True to enable Bluetooth, false to disable
      * @return True if operation successful
      */
     bool setBluetoothEnabled(bool enable);
     
     /**
      * @brief Check if a scheduled power mode change is needed
      * @return True if power mode should be changed
      */
     bool checkPowerSchedule();
     
     /**
      * @brief Create RTOS tasks for power management
      */
     void createTasks();
     
 private:
     // Configuration
     PowerSchedule _powerSchedule;
     PowerMode _currentMode;
     bool _isWiFiEnabled;
     bool _isBluetoothEnabled;
     bool _isScheduleActive;
     
     // RTOS resources
     SemaphoreHandle_t _powerMutex;
     TaskHandle_t _powerTaskHandle;
     
     // Helper methods
     bool enterModemSleep();
     bool enterLightSleep();
     bool enterDeepSleep(uint64_t sleepTimeUs = 0);
     bool enterHibernation(uint64_t sleepTimeUs = 0);
     
     // Task function
     static void powerTask(void* parameter);
 };
 
 #endif // POWER_MANAGER_H