/**
 * @file AppCore.cpp
 * @brief Implementation of the central application orchestrator
 */

 #include "AppCore.h"
 #include "../utils/Constants.h"
 #include <esp_system.h>
 #include <esp_task_wdt.h>
 
 // Global pointer to the AppCore instance
 static AppCore* _appCoreInstance = nullptr;
 
 // Global accessor function implementation
 AppCore* getAppCore() {
     return _appCoreInstance;
 }
 
 AppCore::AppCore() : 
     _isFirstBoot(false),
     _isInSetupMode(false),
     _isInitialized(false),
     _systemMutex(nullptr),
     _initTaskHandle(nullptr)
 {
     // Set the global pointer to this instance
     _appCoreInstance = this;
 }
 
 AppCore::~AppCore() {
     // Release RTOS resources
     if (_systemMutex != nullptr) {
         vSemaphoreDelete(_systemMutex);
     }
     
     // Delete tasks if they exist
     if (_initTaskHandle != nullptr) {
         vTaskDelete(_initTaskHandle);
     }
 }
 
 bool AppCore::begin() {
     // Start serial for debugging
     Serial.begin(115200);
     Serial.println("\n\n");
     Serial.println("===================================");
     Serial.println(Constants::APP_NAME);
     Serial.println("Version: " + String(Constants::APP_VERSION));
     Serial.println("===================================");
     
     // Initialize Non-Volatile Storage
     if (!initNVS()) {
         Serial.println("NVS initialization failed!");
         return false;
     }
     
     // Create system mutex for thread-safe operations
     _systemMutex = xSemaphoreCreateMutex();
     if (_systemMutex == nullptr) {
         Serial.println("Failed to create system mutex!");
         return false;
     }
     
     // Create the initialization task with higher priority
     BaseType_t result = xTaskCreatePinnedToCore(
         initTaskFunction,       // Task function
         "InitTask",             // Task name
         4096,                   // Stack size (words)
         this,                   // Task parameters
         10,                     // Priority (higher than others)
         &_initTaskHandle,       // Task handle
         1                       // Core ID (1 - application core)
     );
     
     if (result != pdPASS) {
         Serial.println("Failed to create initialization task!");
         return false;
     }
     
     // Task will handle the rest of initialization
     return true;
 }
 
 void AppCore::initTaskFunction(void* parameter) {
     AppCore* app = static_cast<AppCore*>(parameter);
     
     // Initialize managers
     app->initManagers();
     
     // Check if we need initial setup
     if (app->needsInitialSetup()) {
         app->startInitialSetup();
     } else {
         app->startNormalOperation();
     }
     
     // Once initialization is complete, we can delete this task
     app->_initTaskHandle = nullptr;
     vTaskDelete(nullptr);
 }
 
 bool AppCore::initNVS() {
     esp_err_t ret = nvs_flash_init();
     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
         // NVS partition was truncated or is a new version, erase and retry
         ESP_ERROR_CHECK(nvs_flash_erase());
         ret = nvs_flash_init();
     }
     ESP_ERROR_CHECK(ret);
     
     Serial.println("NVS initialized successfully");
     return true;
 }
 
 void AppCore::initManagers() {
     Serial.println("Initializing system managers...");
     
     // Initialize managers in the correct order
     _logManager.begin();
     _logManager.log(LogLevel::INFO, "System", "System startup complete, Running Version " + String(Constants::APP_VERSION));
 }
 
 void AppCore::factoryReset() {
     _logManager.log(LogLevel::INFO, "System", "Factory reset initiated");
     
     // Stop all tasks
     vTaskSuspendAll();
     
     // Set the factory reset flag
     _storageManager.setFactoryResetFlag(true);
     
     // Reset NVS
     nvs_flash_erase();
     
     // Reboot
     reboot();
 }
 
 void AppCore::reboot() {
     _logManager.log(LogLevel::INFO, "System", "System rebooting...");
     delay(1000);  // Allow log to be written
     esp_restart();
 }
 
 void AppCore::initRTOSTasks() {
     _logManager.log(LogLevel::INFO, "System", "Initializing RTOS tasks");
     
     // Create different task sets depending on mode
     if (_isInSetupMode) {
         // For setup mode, we only need a subset of tasks
         _networkManager.createTasks();
         _webServer.createTasks();
         _logManager.createTasks();
     } else {
         // For normal operation, create all tasks
         _networkManager.createTasks();
         _webServer.createTasks();
         _sensorManager.createTasks();
         _relayManager.createTasks();
         _logManager.createTasks();
         _maintenanceManager.createTasks();
         _timeManager.createTasks();
         
         if (_profileManager.isMQTTEnabled()) {
             _mqttClient.createTasks();
         }
     }
 }
 
 void AppCore::onWiFiConnected(const String& ip, const String& ssid) {
     String hostname = _networkManager.getHostname();
     uint16_t webPort = _webServer.getPort();
     
     _logManager.log(LogLevel::INFO, "System", "Start-up complete, Running Version " + String(Constants::APP_VERSION) + 
                    ", on IP: " + ip + 
                    ", Webserver on port: " + String(webPort) + 
                    ", SSID: " + ssid + 
                    ", hostname: " + hostname + 
                    "\nYou can access the device at http://" + ip + ":" + String(webPort) + "/index.html or http://" + hostname + ".local:" + String(webPort) + "/index.html");
 }
 
 void AppCore::onWiFiDisconnected() {
    _logManager.log(LogLevel::WARN, "System", "WiFi disconnected. Attempting to reconnect...");
    
    _logManager.log(LogLevel::INFO, "System", 
        "Configuration mode started. Connect to WiFi AP '" + 
        String(Constants::DEFAULT_AP_SSID) + 
        "' and visit http://192.168.4.1 to configure the device");
    
    // Initialize RTOS tasks for setup mode
    initRTOSTasks();
}
 
 void AppCore::startNormalOperation() {
     _logManager.log(LogLevel::INFO, "System", "Starting normal operation mode");
     _isInSetupMode = false;
     
     // Start in station mode with saved credentials
     if (_networkManager.startSTAMode()) {
         _logManager.log(LogLevel::INFO, "System", "Connecting to WiFi...");
     } else {
         _logManager.log(LogLevel::ERROR, "System", "Failed to start WiFi in station mode!");
         // Fall back to setup mode
         startInitialSetup();
         return;
     }
     
     // Initialize MQTT if configured
     if (_profileManager.isMQTTEnabled()) {
         _mqttClient.begin();
     }
     
     // Start the web server for normal operation
     _webServer.startNormalMode();
     
     // Enable OTA updates
     _otaManager.enableUpdates();
     
     // Initialize sensors fully
     _sensorManager.fullInitialization();
     
     // Initialize relays
     _relayManager.initRelays();
     
     // Initialize RTOS tasks for normal operation
     initRTOSTasks();
     
     _logManager.log(LogLevel::INFO, "System", "Starting " + String(Constants::APP_NAME) + " v" + String(Constants::APP_VERSION));
     
     _storageManager.begin();
     _securityManager.begin();
     _timeManager.begin();
     _maintenanceManager.begin();
     _powerManager.begin();
     _notificationManager.begin();
     _profileManager.begin();
     
     // These will be fully initialized in the appropriate mode
     _networkManager.begin();
     _webServer.begin();
     _otaManager.begin();
     _sensorManager.begin();
     _relayManager.begin();
     
     _logManager.log(LogLevel::INFO, "System", "All managers initialized");
     _isInitialized = true;
 }
 
 bool AppCore::needsInitialSetup() {
     // Check if WiFi credentials exist in NVS
     nvs_handle_t nvsHandle;
     esp_err_t err = nvs_open(Constants::NVS_WIFI_NAMESPACE, NVS_READONLY, &nvsHandle);
     
     if (err != ESP_OK) {
         _logManager.log(LogLevel::INFO, "System", "NVS namespace not found, first boot detected");
         _isFirstBoot = true;
         return true;
     }
     
     // Check for SSID1
     size_t ssidLen = 0;
     err = nvs_get_str(nvsHandle, Constants::NVS_WIFI_SSID1_KEY, nullptr, &ssidLen);
     nvs_close(nvsHandle);
     
     if (err != ESP_OK || ssidLen == 0) {
         _logManager.log(LogLevel::INFO, "System", "No WiFi credentials found, setup needed");
         _isFirstBoot = true;
         return true;
     }
     
     // Check if factory reset was explicitly requested
     bool resetRequested = _storageManager.getFactoryResetFlag();
     if (resetRequested) {
         _logManager.log(LogLevel::INFO, "System", "Factory reset requested");
         return true;
     }
     
     return false;
 }
 
 void AppCore::startInitialSetup() {
     _logManager.log(LogLevel::INFO, "System", "Starting initial setup mode");
     _isInSetupMode = true;
     
     // Start in AP mode
     _networkManager.startAPMode(
         Constants::DEFAULT_AP_SSID,
         Constants::DEFAULT_AP_PASSWORD,
         true  // Also start in STA mode to allow scanning
     );
     
     // Start the web server for configuration
     _webServer.startConfigurationMode();
     
     _logManager.log(LogLevel::INFO, "System");
    }