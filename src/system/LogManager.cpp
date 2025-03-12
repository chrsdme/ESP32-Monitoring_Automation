/**
 * @file LogManager.cpp
 * @brief Implementation of the LogManager class
 */

 #include "LogManager.h"
 #include "../core/AppCore.h"
 #include <time.h>
 
 LogManager::LogManager() :
     _logLevel(LogLevel::INFO),
     _maxLogSize(Constants::MAX_LOG_FILE_SIZE / 1024),  // Convert to KB
     _remoteLogServer(""),
     _flushInterval(60),  // 60 seconds default
     _bufferMaxSize(100),  // Maximum entries in memory buffer
     _logMutex(nullptr),
     _logTaskHandle(nullptr),
     _logQueue(nullptr)
 {
 }
 
 LogManager::~LogManager() {
     // Close log file if open
     if (_logFile) {
         _logFile.close();
     }
     
     // Clean up RTOS resources
     if (_logMutex != nullptr) {
         vSemaphoreDelete(_logMutex);
     }
     
     if (_logTaskHandle != nullptr) {
         vTaskDelete(_logTaskHandle);
     }
     
     if (_logQueue != nullptr) {
         vQueueDelete(_logQueue);
     }
 }
 
 bool LogManager::begin() {
     // Create mutex for thread-safe operations
     _logMutex = xSemaphoreCreateMutex();
     if (_logMutex == nullptr) {
         Serial.println("Failed to create log mutex!");
         return false;
     }
     
     // Create queue for log messages
     _logQueue = xQueueCreate(50, sizeof(LogEntry));
     if (_logQueue == nullptr) {
         Serial.println("Failed to create log queue!");
         return false;
     }
     
     // Create necessary directory
     if (!SPIFFS.exists("/logs")) {
         SPIFFS.mkdir("/logs");
     }
     
     // Open log file
     if (!openLogFile()) {
         Serial.println("Failed to open log file!");
         return false;
     }
     
     // Log system start
     log(LogLevel::INFO, "LogManager", "Logging system initialized");
     
     return true;
 }
 
 void LogManager::log(LogLevel level, const String& module, const String& message) {
     // Ignore messages below current log level
     if (level < _logLevel) {
         return;
     }
     
     // Create log entry
     LogEntry entry;
     entry.level = level;
     entry.module = module;
     entry.message = message;
     entry.timestamp = millis();
     
     // Print to serial (for debugging)
     Serial.print(getLogLevelString(level));
     Serial.print(" [");
     Serial.print(module);
     Serial.print("] ");
     Serial.println(message);
     
     // Add to queue for processing
     if (_logQueue) {
         // Make a copy for the queue
         LogEntry* entryCopy = new LogEntry(entry);
         if (xQueueSend(_logQueue, entryCopy, 0) != pdPASS) {
             // Queue is full, just delete the copy
             delete entryCopy;
         }
     }
     
     // Also add to memory buffer (for recent logs API)
     if (xSemaphoreTake(_logMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
         _memoryBuffer.push_back(entry);
         
         // Limit buffer size
         if (_memoryBuffer.size() > _bufferMaxSize) {
             _memoryBuffer.erase(_memoryBuffer.begin());
         }
         
         xSemaphoreGive(_logMutex);
     }
 }
 
 void LogManager::setLogLevel(LogLevel level) {
     _logLevel = level;
     log(LogLevel::INFO, "LogManager", "Log level set to " + getLogLevelString(level));
 }
 
 LogLevel LogManager::getLogLevel() {
     return _logLevel;
 }
 
 void LogManager::setMaxLogSize(size_t sizeKB) {
     _maxLogSize = sizeKB;
     log(LogLevel::INFO, "LogManager", "Max log size set to " + String(sizeKB) + "KB");
 }
 
 void LogManager::setRemoteLogServer(const String& server) {
     _remoteLogServer = server;
     if (server.length() > 0) {
         log(LogLevel::INFO, "LogManager", "Remote log server set to " + server);
     } else {
         log(LogLevel::INFO, "LogManager", "Remote logging disabled");
     }
 }
 
 void LogManager::setFlushInterval(uint32_t seconds) {
     _flushInterval = seconds;
     log(LogLevel::INFO, "LogManager", "Log flush interval set to " + String(seconds) + "s");
 }
 
 std::vector<LogEntry> LogManager::getRecentLogs(size_t maxEntries) {
     std::vector<LogEntry> result;
     
     if (xSemaphoreTake(_logMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Calculate how many entries to return
         size_t count = min(_memoryBuffer.size(), maxEntries);
         size_t startIndex = _memoryBuffer.size() - count;
         
         // Copy the most recent logs
         for (size_t i = startIndex; i < _memoryBuffer.size(); i++) {
             result.push_back(_memoryBuffer[i]);
         }
         
         xSemaphoreGive(_logMutex);
     }
     
     return result;
 }
 
 void LogManager::clearLogs() {
     if (xSemaphoreTake(_logMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Close existing file
         if (_logFile) {
             _logFile.close();
         }
         
         // Delete and recreate log file
         SPIFFS.remove(Constants::LOG_FILE_PATH);
         openLogFile();
         
         // Clear memory buffer
         _memoryBuffer.clear();
         
         xSemaphoreGive(_logMutex);
     }
     
     log(LogLevel::INFO, "LogManager", "Logs cleared");
 }
 
 void LogManager::createTasks() {
     // Create log processing task
     BaseType_t result = xTaskCreatePinnedToCore(
         logProcessingTask,              // Task function
         "LogProcessingTask",            // Task name
         Constants::STACK_SIZE_LOGGING,  // Stack size (words)
         this,                           // Task parameters
         Constants::PRIORITY_LOGGING,    // Priority
         &_logTaskHandle,                // Task handle
         0                               // Core ID (0 - protocol core)
     );
     
     if (result != pdPASS) {
         Serial.println("Failed to create log processing task!");
     }
 }
 
 bool LogManager::openLogFile() {
     // Open log file in append mode
     _logFile = SPIFFS.open(Constants::LOG_FILE_PATH, FILE_APPEND);
     return _logFile;
 }
 
 void LogManager::writeToFile(const LogEntry& entry) {
     if (!_logFile) {
         if (!openLogFile()) {
             return;
         }
     }
     
     // Format and write log entry
     String formattedEntry = formatLogEntry(entry);
     _logFile.println(formattedEntry);
     _logFile.flush();
     
     // Check if log rotation is needed
     if (_logFile.size() > _maxLogSize * 1024) {
         rotateLogFile();
     }
 }
 
 void LogManager::sendToRemoteServer(const LogEntry& entry) {
     if (_remoteLogServer.length() == 0) {
         return;
     }
     
     // Parse server address
     int colonPos = _remoteLogServer.indexOf(':');
     if (colonPos <= 0) {
         return;
     }
     
     String host = _remoteLogServer.substring(0, colonPos);
     int port = _remoteLogServer.substring(colonPos + 1).toInt();
     
     if (port <= 0 || port > 65535) {
         return;
     }
     
     // Connect to server
     WiFiClient client;
     if (!client.connect(host.c_str(), port)) {
         return;
     }
     
     // Send log entry
     String formattedEntry = formatLogEntry(entry);
     client.println(formattedEntry);
     
     // Close connection
     client.stop();
 }
 
 bool LogManager::rotateLogFile() {
     if (xSemaphoreTake(_logMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Close existing file
         if (_logFile) {
             _logFile.close();
         }
         
         // Create backup file name
         String backupName = "/logs/system_backup.log";
         
         // Remove old backup if exists
         if (SPIFFS.exists(backupName)) {
             SPIFFS.remove(backupName);
         }
         
         // Rename current log to backup
         SPIFFS.rename(Constants::LOG_FILE_PATH, backupName);
         
         // Open new log file
         openLogFile();
         
         xSemaphoreGive(_logMutex);
         
         log(LogLevel::INFO, "LogManager", "Log file rotated");
         return true;
     }
     
     return false;
 }
 
 String LogManager::formatLogEntry(const LogEntry& entry) {
     // Get current timestamp
     time_t now;
     time(&now);
     struct tm timeinfo;
     localtime_r(&now, &timeinfo);
     
     char timeStr[20];
     strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
     
     // Format full log entry
     String formattedEntry = String(timeStr) + " " + 
                           getLogLevelString(entry.level) + " [" + 
                           entry.module + "] " + 
                           entry.message;
     
     return formattedEntry;
 }
 
 String LogManager::getLogLevelString(LogLevel level) {
     switch (level) {
         case LogLevel::DEBUG: return "DEBUG";
         case LogLevel::INFO:  return "INFO ";
         case LogLevel::WARN:  return "WARN ";
         case LogLevel::ERROR: return "ERROR";
         default:              return "UNKNOWN";
     }
 }
 
 void LogManager::logProcessingTask(void* parameter) {
     LogManager* logManager = static_cast<LogManager*>(parameter);
     TickType_t lastFlushTime = xTaskGetTickCount();
     
     // Buffer to store log entries for batch processing
     std::vector<LogEntry> logBuffer;
     
     while (true) {
         // Process any queued log entries
         LogEntry entry;
         while (xQueueReceive(logManager->_logQueue, &entry, 0) == pdPASS) {
             logBuffer.push_back(entry);
         }
         
         // Write accumulated logs to file/server at flush interval
         if (xTaskGetTickCount() - lastFlushTime >= pdMS_TO_TICKS(logManager->_flushInterval * 1000) ||
             logBuffer.size() > 20) {  // Also flush if buffer gets large
             
             if (xSemaphoreTake(logManager->_logMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                 for (const auto& entry : logBuffer) {
                     logManager->writeToFile(entry);
                     logManager->sendToRemoteServer(entry);
                 }
                 xSemaphoreGive(logManager->_logMutex);
                 
                 // Clear buffer after processing
                 logBuffer.clear();
                 
                 // Update last flush time
                 lastFlushTime = xTaskGetTickCount();
             }
         }
         
         // Check for shutdown request
         
         // Sleep to avoid hogging CPU
         vTaskDelay(pdMS_TO_TICKS(500));
     }
 }