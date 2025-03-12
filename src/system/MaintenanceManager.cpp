/**
 * @file MaintenanceManager.cpp
 * @brief Implementation of the MaintenanceManager class
 */

 #include "MaintenanceManager.h"
 #include "../core/AppCore.h"
 #include "../system/LogManager.h"
 #include "../components/SensorManager.h"
 #include "../components/RelayManager.h"
 #include <esp_task_wdt.h>
 #include <ArduinoJson.h>
 
 MaintenanceManager::MaintenanceManager() :
     _watchdogEnabled(false),
     _watchdogTimeout(30),
     _lastRebootCheck(0),
     _isInitialized(false),
     _maintenanceMutex(nullptr),
     _maintenanceTaskHandle(nullptr)
 {
 }
 
 MaintenanceManager::~MaintenanceManager() {
     // Clean up RTOS resources
     if (_maintenanceMutex != nullptr) {
         vSemaphoreDelete(_maintenanceMutex);
     }
     
     if (_maintenanceTaskHandle != nullptr) {
         vTaskDelete(_maintenanceTaskHandle);
     }
     
     // Disable watchdog if it was enabled
     if (_watchdogEnabled) {
         esp_task_wdt_deinit();
     }
 }
 
 bool MaintenanceManager::begin() {
     // Create mutex for thread-safe operations
     _maintenanceMutex = xSemaphoreCreateMutex();
     if (_maintenanceMutex == nullptr) {
         Serial.println("Failed to create maintenance mutex!");
         return false;
     }
     
     // Load reboot schedule from NVS if available
     nvs_handle_t nvsHandle;
     esp_err_t err = nvs_open(Constants::NVS_CONFIG_NAMESPACE, NVS_READONLY, &nvsHandle);
     if (err == ESP_OK) {
         uint8_t enabled;
         if (nvs_get_u8(nvsHandle, "reboot_enabled", &enabled) == ESP_OK) {
             _rebootSchedule.enabled = (enabled == 1);
         }
         
         uint8_t day, hour, minute;
         if (nvs_get_u8(nvsHandle, "reboot_day", &day) == ESP_OK) {
             _rebootSchedule.dayOfWeek = day;
         }
         
         if (nvs_get_u8(nvsHandle, "reboot_hour", &hour) == ESP_OK) {
             _rebootSchedule.hour = hour;
         }
         
         if (nvs_get_u8(nvsHandle, "reboot_minute", &minute) == ESP_OK) {
             _rebootSchedule.minute = minute;
         }
         
         nvs_close(nvsHandle);
     }
     
     _isInitialized = true;
     
     getAppCore()->getLogManager()->log(LogLevel::INFO, "Maintenance", 
         "Maintenance manager initialized");
     
     return true;
 }
 
 String MaintenanceManager::runDiagnostics(bool fullTest) {
     DynamicJsonDocument doc(2048);
     
     // Basic diagnostics - always run
     doc["wifi"] = testWiFi();
     doc["sensors"] = testSensors();
     doc["relays"] = testRelays();
     doc["storage"] = testStorage();
     
     // System information
     JsonObject sysInfo = doc.createNestedObject("system_info");
     sysInfo["free_heap"] = ESP.getFreeHeap();
     sysInfo["min_free_heap"] = ESP.getMinFreeHeap();
     sysInfo["uptime_seconds"] = millis() / 1000;
     sysInfo["cpu_freq_mhz"] = ESP.getCpuFreqMHz();
     
     // Full diagnostics - more comprehensive tests
     if (fullTest) {
         // Additional tests could be added here
         doc["full_test"] = true;
         
         // Connection quality information
         if (WiFi.status() == WL_CONNECTED) {
             JsonObject wifi = doc.createNestedObject("wifi_detail");
             wifi["ssid"] = WiFi.SSID();
             wifi["rssi"] = WiFi.RSSI();
             wifi["ip"] = WiFi.localIP().toString();
             wifi["channel"] = WiFi.channel();
         }
         
         // Memory fragmentation
         size_t heapSize = ESP.getHeapSize();
         size_t freeHeap = ESP.getFreeHeap();
         size_t maxAlloc = ESP.getMaxAllocHeap();
         
         JsonObject memory = doc.createNestedObject("memory");
         memory["heap_size"] = heapSize;
         memory["free_heap"] = freeHeap;
         memory["max_alloc"] = maxAlloc;
         memory["fragmentation"] = 100.0f * (1.0f - ((float)maxAlloc / (float)freeHeap));
         
         // SPIFFS status
         JsonObject storage = doc.createNestedObject("storage_detail");
         storage["total"] = SPIFFS.totalBytes();
         storage["used"] = SPIFFS.usedBytes();
         storage["free"] = SPIFFS.totalBytes() - SPIFFS.usedBytes();
     }
     
     String result;
     serializeJson(doc, result);
     
     getAppCore()->getLogManager()->log(LogLevel::INFO, "Maintenance", 
         "Diagnostics completed" + String(fullTest ? " (full)" : ""));
     
     return result;
 }
 
 String MaintenanceManager::testComponent(uint8_t component) {
     DynamicJsonDocument doc(1024);
     bool success = false;
     String message = "Unknown component";
     
     switch (component) {
         case 1: // WiFi
             success = testWiFi();
             message = success ? "WiFi test passed" : "WiFi test failed";
             break;
             
         case 2: // Sensors
             success = testSensors();
             message = success ? "Sensors test passed" : "Sensors test failed";
             break;
             
         case 3: // Relays
             success = testRelays();
             message = success ? "Relays test passed" : "Relays test failed";
             break;
             
         case 4: // Storage
             success = testStorage();
             message = success ? "Storage test passed" : "Storage test failed";
             break;
             
         default:
             success = false;
             message = "Unknown component id: " + String(component);
             break;
     }
     
     doc["success"] = success;
     doc["message"] = message;
     doc["component_id"] = component;
     
     String result;
     serializeJson(doc, result);
     
     getAppCore()->getLogManager()->log(LogLevel::INFO, "Maintenance", 
         "Component test " + String(component) + ": " + message);
     
     return result;
 }
 
 String MaintenanceManager::getSystemHealth() {
     DynamicJsonDocument doc(1024);
     
     // Get system uptime
     uint32_t uptime = millis() / 1000; // seconds
     
     // Get memory stats
     size_t heapSize = ESP.getHeapSize();
     size_t freeHeap = ESP.getFreeHeap();
     size_t minFreeHeap = ESP.getMinFreeHeap();
     
     // Calculate memory health (0-100%)
     uint8_t memoryHealth = (uint8_t)((float)freeHeap / (float)heapSize * 100.0f);
     
     // Calculate system health score (0-100)
     // Simple algorithm that combines memory health and other factors
     uint8_t systemHealth = memoryHealth;
     
     // WiFi status
     bool wifiConnected = WiFi.status() == WL_CONNECTED;
     int8_t rssi = wifiConnected ? WiFi.RSSI() : 0;
     uint8_t wifiHealth = 0;
     
     if (wifiConnected) {
         // Convert RSSI to health percentage (typical values: -30 to -90)
         if (rssi >= -50) {
             wifiHealth = 100;
         } else if (rssi <= -90) {
             wifiHealth = 0;
         } else {
             wifiHealth = (uint8_t)((float)(rssi + 90) / 40.0f * 100.0f);
         }
         
         // Include WiFi health in overall score
         systemHealth = (systemHealth + wifiHealth) / 2;
     } else {
         // Penalize overall health if WiFi is disconnected
         systemHealth = (systemHealth * 2) / 3;
     }
     
     // Create health status
     doc["system_health"] = systemHealth;
     doc["memory_health"] = memoryHealth;
     doc["wifi_health"] = wifiHealth;
     doc["wifi_connected"] = wifiConnected;
     doc["uptime_seconds"] = uptime;
     doc["free_heap"] = freeHeap;
     doc["min_free_heap"] = minFreeHeap;
     doc["status"] = systemHealth > 70 ? "good" : (systemHealth > 40 ? "fair" : "poor");
     
     String result;
     serializeJson(doc, result);
     
     return result;
 }
 
 bool MaintenanceManager::setRebootSchedule(bool enabled, uint8_t dayOfWeek, uint8_t hour, uint8_t minute) {
     // Validate parameters
     if (dayOfWeek > 6 || hour > 23 || minute > 59) {
         return false;
     }
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_maintenanceMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         _rebootSchedule.enabled = enabled;
         _rebootSchedule.dayOfWeek = dayOfWeek;
         _rebootSchedule.hour = hour;
         _rebootSchedule.minute = minute;
         
         // Save to NVS
         nvs_handle_t nvsHandle;
         esp_err_t err = nvs_open(Constants::NVS_CONFIG_NAMESPACE, NVS_READWRITE, &nvsHandle);
         if (err == ESP_OK) {
             nvs_set_u8(nvsHandle, "reboot_enabled", enabled ? 1 : 0);
             nvs_set_u8(nvsHandle, "reboot_day", dayOfWeek);
             nvs_set_u8(nvsHandle, "reboot_hour", hour);
             nvs_set_u8(nvsHandle, "reboot_minute", minute);
             nvs_commit(nvsHandle);
             nvs_close(nvsHandle);
         }
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Maintenance", 
             "Reboot schedule " + String(enabled ? "enabled" : "disabled") + 
             ", day " + String(dayOfWeek) + 
             " at " + String(hour) + ":" + String(minute));
         
         xSemaphoreGive(_maintenanceMutex);
         return true;
     }
     
     return false;
 }
 
 RebootSchedule MaintenanceManager::getRebootSchedule() {
     RebootSchedule schedule;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_maintenanceMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         schedule = _rebootSchedule;
         xSemaphoreGive(_maintenanceMutex);
     }
     
     return schedule;
 }
 
 bool MaintenanceManager::checkScheduledReboot() {
     if (!_rebootSchedule.enabled) {
         return false;
     }
     
     // Get current time
     time_t now = time(nullptr);
     struct tm timeinfo;
     localtime_r(&now, &timeinfo);
     
     // Check if it's time to reboot
     if (timeinfo.tm_wday == _rebootSchedule.dayOfWeek &&
         timeinfo.tm_hour == _rebootSchedule.hour &&
         timeinfo.tm_min == _rebootSchedule.minute &&
         now - _lastRebootCheck > 30) {  // Ensure we don't check too frequently
         
         _lastRebootCheck = now;
         
         // It's reboot time!
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Maintenance", 
             "Scheduled reboot triggered");
         
         return true;
     }
     
     _lastRebootCheck = now;
     return false;
 }
 
 bool MaintenanceManager::setWatchdogEnabled(bool enabled, uint32_t timeoutSeconds) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_maintenanceMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // If current state matches desired state, nothing to do
         if (_watchdogEnabled == enabled) {
             xSemaphoreGive(_maintenanceMutex);
             return true;
         }
         
         if (enabled) {
             // Initialize watchdog
             esp_err_t err = esp_task_wdt_init(timeoutSeconds, true); // true = panic on timeout
             if (err != ESP_OK) {
                 getAppCore()->getLogManager()->log(LogLevel::ERROR, "Maintenance", 
                     "Failed to initialize watchdog timer");
                 xSemaphoreGive(_maintenanceMutex);
                 return false;
             }
             
             // Subscribe current task to watchdog
             err = esp_task_wdt_add(NULL);
             if (err != ESP_OK) {
                 getAppCore()->getLogManager()->log(LogLevel::ERROR, "Maintenance", 
                     "Failed to subscribe to watchdog timer");
                 esp_task_wdt_deinit();
                 xSemaphoreGive(_maintenanceMutex);
                 return false;
             }
             
             _watchdogEnabled = true;
             _watchdogTimeout = timeoutSeconds;
             
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Maintenance", 
                 "Watchdog timer enabled with timeout of " + String(timeoutSeconds) + " seconds");
         } else {
             // Unsubscribe current task
             esp_err_t err = esp_task_wdt_delete(NULL);
             
             // Deinitialize watchdog
             esp_task_wdt_deinit();
             
             _watchdogEnabled = false;
             
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Maintenance", 
                 "Watchdog timer disabled");
         }
         
         xSemaphoreGive(_maintenanceMutex);
         return true;
     }
     
     return false;
 }
 
 bool MaintenanceManager::isWatchdogEnabled() {
     bool enabled = false;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_maintenanceMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         enabled = _watchdogEnabled;
         xSemaphoreGive(_maintenanceMutex);
     }
     
     return enabled;
 }
 
 void MaintenanceManager::feedWatchdog() {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_maintenanceMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         if (_watchdogEnabled) {
             esp_task_wdt_reset();
         }
         xSemaphoreGive(_maintenanceMutex);
     }
 }
 
 void MaintenanceManager::createTasks() {
     // Create maintenance task
     BaseType_t result = xTaskCreatePinnedToCore(
         maintenanceTask,             // Task function
         "MaintenanceTask",           // Task name
         2048,                        // Stack size (words)
         this,                        // Task parameters
         1,                           // Priority (low)
         &_maintenanceTaskHandle,     // Task handle
         0                            // Core ID (0 - protocol core)
     );
     
     if (result != pdPASS) {
         getAppCore()->getLogManager()->log(LogLevel::ERROR, "Maintenance", 
             "Failed to create maintenance task");
     }
 }
 
 bool MaintenanceManager::testWiFi() {
     if (WiFi.status() != WL_CONNECTED) {
         return false;
     }
     
     // Basic connectivity test
     IPAddress ip = WiFi.localIP();
     if (ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0) {
         return false;
     }
     
     // Simple ping test to gateway
     IPAddress gateway = WiFi.gatewayIP();
     // Note: proper ping implementation would require icmp_echo
     // For simplicity, we just check if gateway is valid
     if (gateway[0] == 0 && gateway[1] == 0 && gateway[2] == 0 && gateway[3] == 0) {
         return false;
     }
     
     // Check signal strength
     int rssi = WiFi.RSSI();
     if (rssi < getAppCore()->getNetworkManager()->getMinRSSI()) {
         return false;
     }
     
     return true;
 }
 
 bool MaintenanceManager::testSensors() {
     SensorReading upperDht, lowerDht, scd;
     bool hasReadings = getAppCore()->getSensorManager()->getSensorReadings(upperDht, lowerDht, scd);
     
     // At least one sensor should work
     return hasReadings && (upperDht.valid || lowerDht.valid || scd.valid);
 }
 
 bool MaintenanceManager::testRelays() {
     // Get relay configurations
     std::vector<RelayConfig> relays = getAppCore()->getRelayManager()->getAllRelayConfigs();
     
     // Check that we have all expected relays
     if (relays.size() != 8) {
         return false;
     }
     
     // Perform a simple check that pins are initialized
     // In a real implementation, you might test toggling relays
     for (const auto& relay : relays) {
         if (relay.pin == 0) {
             return false;
         }
     }
     
     return true;
 }
 
 bool MaintenanceManager::testStorage() {
     // Check that SPIFFS is mounted
     if (!SPIFFS.begin(true)) {
         return false;
     }
     
     // Check available space
     size_t total = SPIFFS.totalBytes();
     size_t used = SPIFFS.usedBytes();
     
     // Make sure we have reasonable values and some free space
     if (total == 0 || used > total || (total - used) < 10240) {
         return false;
     }
     
     // NVS test
     nvs_handle_t nvsHandle;
     esp_err_t err = nvs_open(Constants::NVS_CONFIG_NAMESPACE, NVS_READWRITE, &nvsHandle);
     if (err != ESP_OK) {
         return false;
     }
     
     // Try to write and read a test value
     err = nvs_set_u32(nvsHandle, "test_key", 12345);
     if (err != ESP_OK) {
         nvs_close(nvsHandle);
         return false;
     }
     
     uint32_t testValue = 0;
     err = nvs_get_u32(nvsHandle, "test_key", &testValue);
     
     // Clean up test key
     nvs_erase_key(nvsHandle, "test_key");
     nvs_commit(nvsHandle);
     nvs_close(nvsHandle);
     
     return (err == ESP_OK && testValue == 12345);
 }
 
 void MaintenanceManager::maintenanceTask(void* parameter) {
     MaintenanceManager* maintenanceManager = static_cast<MaintenanceManager*>(parameter);
     TickType_t lastWakeTime = xTaskGetTickCount();
     
     // Initial delay to allow system to stabilize
     vTaskDelay(pdMS_TO_TICKS(30000));  // 30 seconds
     
     while (true) {
         // Feed watchdog if enabled
         maintenanceManager->feedWatchdog();
         
         // Check for scheduled reboot
         if (maintenanceManager->checkScheduledReboot()) {
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Maintenance", 
                 "Executing scheduled reboot...");
             
             // Perform reboot
             vTaskDelay(pdMS_TO_TICKS(1000));  // Give time for log to be saved
             getAppCore()->reboot();
         }
         
         // Perform periodic maintenance checks
         // This could include monitoring system health, checking for updates, etc.
         
         // Sleep until next maintenance period (every minute)
         vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(60000));
     }
 }