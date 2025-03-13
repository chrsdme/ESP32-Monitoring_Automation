/**
 * @file AppCore.h
 * @brief Central orchestrator of all modules
 */

 #ifndef APP_CORE_H
 #define APP_CORE_H
 
 #include <Arduino.h>
 #include <freertos/FreeRTOS.h>
 #include <freertos/task.h>
 #include <freertos/semphr.h>
 #include <freertos/queue.h>
 #include <nvs_flash.h>
 
 #include "../network/NetworkManager.h"
 #include "../network/MQTTClient.h"
 #include "../system/StorageManager.h"
 #include "../system/TimeManager.h"
 #include "../system/LogManager.h"
 #include "../system/MaintenanceManager.h"
 #include "../system/PowerManager.h"
 #include "../system/NotificationManager.h"
 #include "../system/ProfileManager.h"
 #include "../web/WebServer.h"       // Changed back to regular WebServer
 #include "../ota/OTAManager.h"
 #include "../components/SensorManager.h"
 #include "../components/RelayManager.h"
 #include "../core/SecurityManager.h"
 
 /**
  * @class AppCore
  * @brief Central orchestrator that manages all system modules and tasks
  */
 class AppCore {
 public:
     AppCore();
     ~AppCore();
     
     /**
      * @brief Initialize and start the application
      * @return True if started successfully
      */
     bool begin();
     
     /**
      * @brief Check if first-time setup is required
      * @return True if this is the first boot or factory reset requested
      */
     bool needsInitialSetup();
     
     /**
      * @brief Start the initial setup mode (AP mode)
      */
     void startInitialSetup();
     
     /**
      * @brief Start normal operation mode
      */
     void startNormalOperation();
     
     /**
      * @brief Perform a factory reset
      */
     void factoryReset();
     
     /**
      * @brief Reboot the device
      */
     void reboot();
     
     // Accessor methods for the various managers
     NetworkManager* getNetworkManager() { return &_networkManager; }
     MQTTClient* getMQTTClient() { return &_mqttClient; }
     StorageManager* getStorageManager() { return &_storageManager; }
     TimeManager* getTimeManager() { return &_timeManager; }
     LogManager* getLogManager() { return &_logManager; }
     MaintenanceManager* getMaintenanceManager() { return &_maintenanceManager; }
     PowerManager* getPowerManager() { return &_powerManager; }
     NotificationManager* getNotificationManager() { return &_notificationManager; }
     ProfileManager* getProfileManager() { return &_profileManager; }
     WebServer* getWebServer() { return &_webServer; }  // Changed back to WebServer
     OTAManager* getOTAManager() { return &_otaManager; }
     SensorManager* getSensorManager() { return &_sensorManager; }
     RelayManager* getRelayManager() { return &_relayManager; }
     SecurityManager* getSecurityManager() { return &_securityManager; }
     
     // Global event handlers
     void onWiFiConnected(const String& ip, const String& ssid);
     void onWiFiDisconnected();
     
 private:
     // System state
     bool _isFirstBoot;
     bool _isInSetupMode;
     bool _isInitialized;
     
     // Module instances
     NetworkManager _networkManager;
     MQTTClient _mqttClient;
     StorageManager _storageManager;
     TimeManager _timeManager;
     LogManager _logManager;
     MaintenanceManager _maintenanceManager;
     PowerManager _powerManager;
     NotificationManager _notificationManager;
     ProfileManager _profileManager;
     WebServer _webServer;            // Changed back to WebServer
     OTAManager _otaManager;
     SensorManager _sensorManager;
     RelayManager _relayManager;
     SecurityManager _securityManager;
     
     // RTOS resources
     SemaphoreHandle_t _systemMutex;
     
     // Task handles
     TaskHandle_t _initTaskHandle;
     
     // Private helper methods
     bool initNVS();
     void initManagers();
     void initRTOSTasks();
     
     // Task functions (static because they need to be passed to xTaskCreate)
     static void initTaskFunction(void* parameter);
 };
 
 // Global accessor
 extern AppCore* getAppCore();
 
 #endif // APP_CORE_H