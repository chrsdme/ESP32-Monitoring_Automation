/**
 * @file MaintenanceManager.h
 * @brief Manages system maintenance, diagnostics, and recovery
 */

 #ifndef MAINTENANCE_MANAGER_H
 #define MAINTENANCE_MANAGER_H
 
 #include <Arduino.h>
 #include <freertos/FreeRTOS.h>
 #include <freertos/task.h>
 #include <freertos/semphr.h>
 #include "../utils/Constants.h"
 
 // Forward declarations
 class AppCore;
 
 /**
  * @struct RebootSchedule
  * @brief Structure to hold scheduled reboot information
  */
 struct RebootSchedule {
     bool enabled;
     uint8_t dayOfWeek;  // 0 = Sunday, 6 = Saturday
     uint8_t hour;
     uint8_t minute;
     
     RebootSchedule() : enabled(false), dayOfWeek(0), hour(3), minute(0) {}
     
     RebootSchedule(bool enabled, uint8_t dayOfWeek, uint8_t hour, uint8_t minute)
         : enabled(enabled), dayOfWeek(dayOfWeek), hour(hour), minute(minute) {}
 };
 
 /**
  * @class MaintenanceManager
  * @brief Manages system maintenance, diagnostics, and recovery operations
  */
 class MaintenanceManager {
 public:
     MaintenanceManager();
     ~MaintenanceManager();
     
     /**
      * @brief Initialize the maintenance manager
      * @return True if initialized successfully
      */
     bool begin();
     
     /**
      * @brief Run a system diagnostic test
      * @param fullTest True for comprehensive test, false for basic test
      * @return Test result as JSON string
      */
     String runDiagnostics(bool fullTest = false);
     
     /**
      * @brief Test a specific system component
      * @param component Component identifier (1 = WiFi, 2 = Sensors, 3 = Relays, etc.)
      * @return Test result as JSON string
      */
     String testComponent(uint8_t component);
     
     /**
      * @brief Get system health status
      * @return Health status as JSON string
      */
     String getSystemHealth();
     
     /**
      * @brief Set scheduled reboot configuration
      * @param enabled Whether scheduled reboots are enabled
      * @param dayOfWeek Day of week (0 = Sunday, 6 = Saturday)
      * @param hour Hour (0-23)
      * @param minute Minute (0-59)
      * @return True if schedule set successfully
      */
     bool setRebootSchedule(bool enabled, uint8_t dayOfWeek, uint8_t hour, uint8_t minute);
     
     /**
      * @brief Get scheduled reboot configuration
      * @return Reboot schedule
      */
     RebootSchedule getRebootSchedule();
     
     /**
      * @brief Check if a scheduled reboot is needed
      * @return True if reboot should be performed
      */
     bool checkScheduledReboot();
     
     /**
      * @brief Enable or disable the watchdog timer
      * @param enabled Whether watchdog is enabled
      * @param timeoutSeconds Timeout in seconds
      * @return True if watchdog configured successfully
      */
     bool setWatchdogEnabled(bool enabled, uint32_t timeoutSeconds = 30);
     
     /**
      * @brief Check if watchdog is enabled
      * @return True if watchdog is enabled
      */
     bool isWatchdogEnabled();
     
     /**
      * @brief Feed the watchdog timer
      */
     void feedWatchdog();
     
     /**
      * @brief Create RTOS tasks for maintenance operations
      */
     void createTasks();
     
 private:
     // Configuration
     RebootSchedule _rebootSchedule;
     bool _watchdogEnabled;
     uint32_t _watchdogTimeout;
     time_t _lastRebootCheck;
     
     // Status tracking
     bool _isInitialized;
     
     // RTOS resources
     SemaphoreHandle_t _maintenanceMutex;
     TaskHandle_t _maintenanceTaskHandle;
     
     // Helper methods
     bool testWiFi();
     bool testSensors();
     bool testRelays();
     bool testStorage();
     
     // Task function
     static void maintenanceTask(void* parameter);
 };
 
 #endif // MAINTENANCE_MANAGER_H