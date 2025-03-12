/**
 * @file TimeManager.cpp
 * @brief Implementation of the TimeManager class
 */

 #include "TimeManager.h"
 #include "../core/AppCore.h"
 #include "../system/LogManager.h"
 
 TimeManager::TimeManager() :
     _timezone("GMT0"),
     _ntpServer1("pool.ntp.org"),
     _ntpServer2("time.nist.gov"),
     _ntpServer3(""),
     _syncInterval(86400),  // Default: sync once per day
     _isTimeSet(false),
     _lastSyncTime(0),
     _timeMutex(nullptr),
     _timeTaskHandle(nullptr)
 {
 }
 
 TimeManager::~TimeManager() {
     // Clean up RTOS resources
     if (_timeMutex != nullptr) {
         vSemaphoreDelete(_timeMutex);
     }
     
     if (_timeTaskHandle != nullptr) {
         vTaskDelete(_timeTaskHandle);
     }
 }
 
 bool TimeManager::begin() {
     // Create mutex for thread-safe operations
     _timeMutex = xSemaphoreCreateMutex();
     if (_timeMutex == nullptr) {
         Serial.println("Failed to create time mutex!");
         return false;
     }
     
     // Load timezone from NVS if available
     nvs_handle_t nvsHandle;
     esp_err_t err = nvs_open(Constants::NVS_CONFIG_NAMESPACE, NVS_READONLY, &nvsHandle);
     if (err == ESP_OK) {
         size_t len = 0;
         err = nvs_get_str(nvsHandle, "timezone", nullptr, &len);
         if (err == ESP_OK && len > 0) {
             char* timezoneBuffer = new char[len];
             nvs_get_str(nvsHandle, "timezone", timezoneBuffer, &len);
             _timezone = String(timezoneBuffer);
             delete[] timezoneBuffer;
         }
         nvs_close(nvsHandle);
     }
     
     // Configure NTP servers and timezone
     configTime(0, 0, _ntpServer1.c_str(), _ntpServer2.c_str(), _ntpServer3.length() > 0 ? _ntpServer3.c_str() : nullptr);
     setenv("TZ", _timezone.c_str(), 1);
     tzset();
     
     // Perform initial time sync
     syncTime();
     
     getAppCore()->getLogManager()->log(LogLevel::INFO, "Time", 
         "Time manager initialized with timezone: " + _timezone);
     
     return true;
 }
 
 bool TimeManager::syncTime() {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_timeMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Time", "Synchronizing time with NTP servers...");
         
         // Check if WiFi is connected
         if (WiFi.status() != WL_CONNECTED) {
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "Time", "Cannot sync time: WiFi not connected");
             xSemaphoreGive(_timeMutex);
             return false;
         }
         
         // Configure NTP and timezone
         configTime(0, 0, _ntpServer1.c_str(), _ntpServer2.c_str(), _ntpServer3.length() > 0 ? _ntpServer3.c_str() : nullptr);
         setenv("TZ", _timezone.c_str(), 1);
         tzset();
         
         // Wait for time to sync (with timeout)
         uint32_t startTime = millis();
         while (!_isTimeSet && millis() - startTime < 10000) {  // 10 second timeout
             time_t now = time(nullptr);
             if (now > 1609459200) {  // January 1, 2021
                 _isTimeSet = true;
                 _lastSyncTime = now;
                 break;
             }
             delay(100);
         }
         
         xSemaphoreGive(_timeMutex);
         
         if (_isTimeSet) {
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Time", 
                 "Time synchronized: " + getTimeString());
         } else {
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "Time", 
                 "Failed to synchronize time");
         }
         
         return _isTimeSet;
     }
     
     return false;
 }
 
 bool TimeManager::isTimeSet() {
     bool timeSet = false;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_timeMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         timeSet = _isTimeSet;
         
         // If we think the time is set, perform a sanity check
         if (timeSet) {
             time_t now = time(nullptr);
             if (now < 1609459200) {  // Before January 1, 2021
                 timeSet = false;
                 _isTimeSet = false;
             }
         }
         
         xSemaphoreGive(_timeMutex);
     }
     
     return timeSet;
 }
 
 String TimeManager::getTimeString(const char* format) {
     char buffer[64];
     time_t now;
     struct tm timeinfo;
     
     time(&now);
     localtime_r(&now, &timeinfo);
     
     strftime(buffer, sizeof(buffer), format, &timeinfo);
     return String(buffer);
 }
 
 time_t TimeManager::getTimestamp() {
     return time(nullptr);
 }
 
 bool TimeManager::setTimezone(const String& timezone) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_timeMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         _timezone = timezone;
         
         // Apply timezone
         setenv("TZ", _timezone.c_str(), 1);
         tzset();
         
         // Save to NVS
         nvs_handle_t nvsHandle;
         esp_err_t err = nvs_open(Constants::NVS_CONFIG_NAMESPACE, NVS_READWRITE, &nvsHandle);
         if (err == ESP_OK) {
             nvs_set_str(nvsHandle, "timezone", _timezone.c_str());
             nvs_commit(nvsHandle);
             nvs_close(nvsHandle);
         }
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Time", 
             "Timezone set to: " + _timezone);
         
         xSemaphoreGive(_timeMutex);
         return true;
     }
     
     return false;
 }
 
 String TimeManager::getTimezone() {
     String timezone;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_timeMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         timezone = _timezone;
         xSemaphoreGive(_timeMutex);
     }
     
     return timezone;
 }
 
 bool TimeManager::setNtpServers(const String& server1, const String& server2, const String& server3) {
     // At least one server must be provided
     if (server1.isEmpty()) {
         return false;
     }
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_timeMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         _ntpServer1 = server1;
         _ntpServer2 = server2;
         _ntpServer3 = server3;
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Time", 
             "NTP servers updated. Primary: " + _ntpServer1);
         
         // Reconfigure NTP servers
         configTime(0, 0, _ntpServer1.c_str(), _ntpServer2.length() > 0 ? _ntpServer2.c_str() : nullptr, 
                  _ntpServer3.length() > 0 ? _ntpServer3.c_str() : nullptr);
         
         xSemaphoreGive(_timeMutex);
         return true;
     }
     
     return false;
 }
 
 void TimeManager::getNtpServers(String& server1, String& server2, String& server3) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_timeMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         server1 = _ntpServer1;
         server2 = _ntpServer2;
         server3 = _ntpServer3;
         
         xSemaphoreGive(_timeMutex);
     }
 }
 
 String TimeManager::timestampToString(time_t timestamp, const char* format) {
     char buffer[64];
     struct tm timeinfo;
     
     localtime_r(&timestamp, &timeinfo);
     strftime(buffer, sizeof(buffer), format, &timeinfo);
     
     return String(buffer);
 }
 
 bool TimeManager::isTimeInRange(uint8_t startHour, uint8_t startMinute, uint8_t endHour, uint8_t endMinute) {
     // Get current time
     time_t now;
     struct tm timeinfo;
     
     time(&now);
     localtime_r(&now, &timeinfo);
     
     // Convert to minutes since midnight
     uint16_t currentMinutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;
     uint16_t startMinutes = startHour * 60 + startMinute;
     uint16_t endMinutes = endHour * 60 + endMinute;
     
     // Check if the time range spans midnight
     if (endMinutes < startMinutes) {
         // Time range spans midnight
         return currentMinutes >= startMinutes || currentMinutes <= endMinutes;
     } else {
         // Normal time range
         return currentMinutes >= startMinutes && currentMinutes <= endMinutes;
     }
 }
 
 bool TimeManager::getDate(int& year, int& month, int& day) {
     if (!_isTimeSet) {
         return false;
     }
     
     time_t now;
     struct tm timeinfo;
     
     time(&now);
     localtime_r(&now, &timeinfo);
     
     year = timeinfo.tm_year + 1900;
     month = timeinfo.tm_mon + 1;
     day = timeinfo.tm_mday;
     
     return true;
 }
 
 bool TimeManager::getTime(int& hour, int& minute, int& second) {
     if (!_isTimeSet) {
         return false;
     }
     
     time_t now;
     struct tm timeinfo;
     
     time(&now);
     localtime_r(&now, &timeinfo);
     
     hour = timeinfo.tm_hour;
     minute = timeinfo.tm_min;
     second = timeinfo.tm_sec;
     
     return true;
 }
 
 int TimeManager::getDayOfWeek() {
     time_t now;
     struct tm timeinfo;
     
     time(&now);
     localtime_r(&now, &timeinfo);
     
     return timeinfo.tm_wday;  // 0 = Sunday, 6 = Saturday
 }
 
 void TimeManager::createTasks() {
     // Create time sync task
     BaseType_t result = xTaskCreatePinnedToCore(
         timeTask,                   // Task function
         "TimeTask",                 // Task name
         2048,                       // Stack size (words)
         this,                       // Task parameters
         1,                          // Priority (low)
         &_timeTaskHandle,           // Task handle
         0                           // Core ID (0 - protocol core)
     );
     
     if (result != pdPASS) {
         getAppCore()->getLogManager()->log(LogLevel::ERROR, "Time", 
             "Failed to create time sync task");
     }
 }
 
 void TimeManager::timeTask(void* parameter) {
     TimeManager* timeManager = static_cast<TimeManager*>(parameter);
     TickType_t lastWakeTime = xTaskGetTickCount();
     
     // Initial delay to allow WiFi to connect first
     vTaskDelay(pdMS_TO_TICKS(10000));
     
     while (true) {
         // Check if it's time to sync again
         time_t now = time(nullptr);
         if (now - timeManager->_lastSyncTime >= timeManager->_syncInterval) {
             timeManager->syncTime();
         }
         
         // Sleep for an hour before checking again
         vTaskDelay(pdMS_TO_TICKS(3600000));  // Check once per hour
     }
 }