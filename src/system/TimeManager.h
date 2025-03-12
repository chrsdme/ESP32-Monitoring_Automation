/**
 * @file TimeManager.h
 * @brief Manages system time synchronization and time-related functions
 */

 #ifndef TIME_MANAGER_H
 #define TIME_MANAGER_H
 
 #include <Arduino.h>
 #include <time.h>
 #include <freertos/FreeRTOS.h>
 #include <freertos/task.h>
 #include <freertos/semphr.h>
 #include "../utils/Constants.h"
 
 /**
  * @class TimeManager
  * @brief Manages system time synchronization with NTP servers and provides time-related functions
  */
 class TimeManager {
 public:
     TimeManager();
     ~TimeManager();
     
     /**
      * @brief Initialize the time manager
      * @return True if initialized successfully
      */
     bool begin();
     
     /**
      * @brief Synchronize time with NTP servers
      * @return True if time synchronized successfully
      */
     bool syncTime();
     
     /**
      * @brief Check if time is synchronized
      * @return True if time is synchronized
      */
     bool isTimeSet();
     
     /**
      * @brief Get current time as a string
      * @param format Time format string (strftime format)
      * @return Formatted time string
      */
     String getTimeString(const char* format = "%Y-%m-%d %H:%M:%S");
     
     /**
      * @brief Get current timestamp
      * @return Current timestamp (seconds since epoch)
      */
     time_t getTimestamp();
     
     /**
      * @brief Set time zone
      * @param timezone Time zone string (e.g., "EST5EDT,M3.2.0,M11.1.0")
      * @return True if time zone set successfully
      */
     bool setTimezone(const String& timezone);
     
     /**
      * @brief Get current time zone
      * @return Time zone string
      */
     String getTimezone();
     
     /**
      * @brief Set NTP servers
      * @param server1 Primary NTP server
      * @param server2 Secondary NTP server
      * @param server3 Tertiary NTP server
      * @return True if NTP servers set successfully
      */
     bool setNtpServers(const String& server1, const String& server2 = "", const String& server3 = "");
     
     /**
      * @brief Get NTP servers
      * @param server1 Output parameter for primary NTP server
      * @param server2 Output parameter for secondary NTP server
      * @param server3 Output parameter for tertiary NTP server
      */
     void getNtpServers(String& server1, String& server2, String& server3);
     
     /**
      * @brief Convert timestamp to local time string
      * @param timestamp Timestamp (seconds since epoch)
      * @param format Time format string (strftime format)
      * @return Formatted time string
      */
     String timestampToString(time_t timestamp, const char* format = "%Y-%m-%d %H:%M:%S");
     
     /**
      * @brief Check if current time is in a specified range
      * @param startHour Start hour (0-23)
      * @param startMinute Start minute (0-59)
      * @param endHour End hour (0-23)
      * @param endMinute End minute (0-59)
      * @return True if current time is in the specified range
      */
     bool isTimeInRange(uint8_t startHour, uint8_t startMinute, uint8_t endHour, uint8_t endMinute);
     
     /**
      * @brief Get current date parts
      * @param year Output parameter for year
      * @param month Output parameter for month (1-12)
      * @param day Output parameter for day (1-31)
      * @return True if date parts retrieved successfully
      */
     bool getDate(int& year, int& month, int& day);
     
     /**
      * @brief Get current time parts
      * @param hour Output parameter for hour (0-23)
      * @param minute Output parameter for minute (0-59)
      * @param second Output parameter for second (0-59)
      * @return True if time parts retrieved successfully
      */
     bool getTime(int& hour, int& minute, int& second);
     
     /**
      * @brief Get day of week
      * @return Day of week (0 = Sunday, 6 = Saturday)
      */
     int getDayOfWeek();
     
     /**
      * @brief Create RTOS tasks for time-related operations
      */
     void createTasks();
     
 private:
     // Configuration
     String _timezone;
     String _ntpServer1;
     String _ntpServer2;
     String _ntpServer3;
     uint32_t _syncInterval;
     
     // Status tracking
     bool _isTimeSet;
     time_t _lastSyncTime;
     
     // RTOS resources
     SemaphoreHandle_t _timeMutex;
     TaskHandle_t _timeTaskHandle;
     
     // Task function
     static void timeTask(void* parameter);
 };
 
 #endif // TIME_MANAGER_H