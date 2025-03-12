/**
 * @file LogManager.h
 * @brief Manages system logging with different log levels and destinations
 */

 #ifndef LOG_MANAGER_H
 #define LOG_MANAGER_H
 
 #include <Arduino.h>
 #include <SPIFFS.h>
 #include <freertos/FreeRTOS.h>
 #include <freertos/task.h>
 #include <freertos/semphr.h>
 #include <freertos/queue.h>
 #include <vector>
 #include <WiFiClient.h>
 #include "../utils/Constants.h"
 
 /**
  * @struct LogEntry
  * @brief Structure to hold a log entry
  */
 struct LogEntry {
     LogLevel level;
     String module;
     String message;
     uint32_t timestamp;
 };
 
 /**
  * @class LogManager
  * @brief Manages system logging with support for different log levels and destinations
  */
 class LogManager {
 public:
     LogManager();
     ~LogManager();
     
     /**
      * @brief Initialize the log manager
      * @return True if initialized successfully
      */
     bool begin();
     
     /**
      * @brief Log a message
      * @param level Log level
      * @param module Module name
      * @param message Log message
      */
     void log(LogLevel level, const String& module, const String& message);
     
     /**
      * @brief Set the log level
      * @param level Minimum log level to record
      */
     void setLogLevel(LogLevel level);
     
     /**
      * @brief Get the current log level
      * @return Current log level
      */
     LogLevel getLogLevel();
     
     /**
      * @brief Set the maximum log file size
      * @param sizeKB Maximum size in kilobytes
      */
     void setMaxLogSize(size_t sizeKB);
     
     /**
      * @brief Set the remote log server
      * @param server Server address (IP:port)
      */
     void setRemoteLogServer(const String& server);
     
     /**
      * @brief Set the log flush interval
      * @param seconds Interval in seconds
      */
     void setFlushInterval(uint32_t seconds);
     
     /**
      * @brief Get recent log entries
      * @param maxEntries Maximum number of entries to return
      * @return Vector of log entries
      */
     std::vector<LogEntry> getRecentLogs(size_t maxEntries = 100);
     
     /**
      * @brief Clear the log file
      */
     void clearLogs();
     
     /**
      * @brief Create RTOS tasks for logging operations
      */
     void createTasks();
     
 private:
     // Configuration
     LogLevel _logLevel;
     size_t _maxLogSize;
     String _remoteLogServer;
     uint32_t _flushInterval;
     
     // Internal state
     File _logFile;
     std::vector<LogEntry> _memoryBuffer;
     size_t _bufferMaxSize;
     
     // RTOS resources
     SemaphoreHandle_t _logMutex;
     TaskHandle_t _logTaskHandle;
     QueueHandle_t _logQueue;
     
     // Private methods
     bool openLogFile();
     void writeToFile(const LogEntry& entry);
     void sendToRemoteServer(const LogEntry& entry);
     bool rotateLogFile();
     String formatLogEntry(const LogEntry& entry);
     String getLogLevelString(LogLevel level);
     
     // Task function
     static void logProcessingTask(void* parameter);
 };
 
 #endif // LOG_MANAGER_H