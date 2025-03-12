/**
 * @file NotificationManager.h
 * @brief Manages notifications and alerts via different channels
 */

 #ifndef NOTIFICATION_MANAGER_H
 #define NOTIFICATION_MANAGER_H
 
 #include <Arduino.h>
 #include <freertos/FreeRTOS.h>
 #include <freertos/task.h>
 #include <freertos/semphr.h>
 #include <freertos/queue.h>
 #include <vector>
 #include "../utils/Constants.h"
 
 // Forward declarations
 class AppCore;
 
 /**
  * @enum NotificationChannel
  * @brief Available notification channels
  */
 enum class NotificationChannel : uint8_t {
     NONE = 0,
     EMAIL,
     TELEGRAM,
     MQTT,
     HTTP_WEBHOOK,
     PUSH_NOTIFICATION
 };
 
 /**
  * @enum NotificationLevel
  * @brief Notification severity levels
  */
 enum class NotificationLevel : uint8_t {
     INFO = 0,
     WARNING,
     ALERT,
     CRITICAL
 };
 
 /**
  * @struct NotificationConfig
  * @brief Structure to hold notification channel configuration
  */
 struct NotificationConfig {
     NotificationChannel channel;
     bool enabled;
     String recipient;    // Email address, chat ID, topic, etc.
     String credentials;  // API key, token, etc.
     String endpoint;     // Server, URL, etc.
     
     NotificationConfig() : 
         channel(NotificationChannel::NONE),
         enabled(false),
         recipient(""),
         credentials(""),
         endpoint("") {}
     
     NotificationConfig(NotificationChannel channel, bool enabled, 
                        const String& recipient, const String& credentials, const String& endpoint) :
         channel(channel),
         enabled(enabled),
         recipient(recipient),
         credentials(credentials),
         endpoint(endpoint) {}
 };
 
 /**
  * @struct NotificationMessage
  * @brief Structure to hold a notification message
  */
 struct NotificationMessage {
     NotificationLevel level;
     String source;
     String title;
     String message;
     uint32_t timestamp;
     bool sent;
     
     NotificationMessage() : 
         level(NotificationLevel::INFO),
         source(""),
         title(""),
         message(""),
         timestamp(0),
         sent(false) {}
     
     NotificationMessage(NotificationLevel level, const String& source, 
                         const String& title, const String& message) :
         level(level),
         source(source),
         title(title),
         message(message),
         timestamp(millis()),
         sent(false) {}
 };
 
 /**
  * @class NotificationManager
  * @brief Manages notifications and alerts via different channels
  */
 class NotificationManager {
 public:
     NotificationManager();
     ~NotificationManager();
     
     /**
      * @brief Initialize the notification manager
      * @return True if initialized successfully
      */
     bool begin();
     
     /**
      * @brief Enable a notification channel
      * @param channel Notification channel
      * @param config Channel configuration
      * @return True if channel enabled successfully
      */
     bool enableChannel(NotificationChannel channel, const NotificationConfig& config);
     
     /**
      * @brief Disable a notification channel
      * @param channel Notification channel
      * @return True if channel disabled successfully
      */
     bool disableChannel(NotificationChannel channel);
     
     /**
      * @brief Test a notification channel
      * @param channel Notification channel
      * @return True if test message sent successfully
      */
     bool testChannel(NotificationChannel channel);
     
     /**
      * @brief Set minimum notification level
      * @param level Minimum notification level
      * @return True if level set successfully
      */
     bool setMinLevel(NotificationLevel level);
     
     /**
      * @brief Get minimum notification level
      * @return Minimum notification level
      */
     NotificationLevel getMinLevel();
     
     /**
      * @brief Send a notification
      * @param level Notification level
      * @param source Source of the notification
      * @param title Notification title
      * @param message Notification message
      * @return True if notification queued successfully
      */
     bool sendNotification(NotificationLevel level, const String& source, 
                           const String& title, const String& message);
     
     /**
      * @brief Get recent notifications
      * @param maxCount Maximum number of notifications to return
      * @return Vector of recent notifications
      */
     std::vector<NotificationMessage> getRecentNotifications(size_t maxCount = 10);
     
     /**
      * @brief Get channel config
      * @param channel Notification channel
      * @return Channel configuration
      */
     NotificationConfig getChannelConfig(NotificationChannel channel);
     
     /**
      * @brief Create RTOS tasks for notification operations
      */
     void createTasks();
     
 private:
     // Configuration
     std::map<NotificationChannel, NotificationConfig> _channelConfigs;
     NotificationLevel _minLevel;
     
     // Message history
     std::vector<NotificationMessage> _recentNotifications;
     size_t _maxHistorySize;
     
     // RTOS resources
     SemaphoreHandle_t _notificationMutex;
     TaskHandle_t _notificationTaskHandle;
     QueueHandle_t _notificationQueue;
     
     // Helper methods
     bool sendEmail(const NotificationMessage& notification);
     bool sendTelegram(const NotificationMessage& notification);
     bool sendMqtt(const NotificationMessage& notification);
     bool sendWebhook(const NotificationMessage& notification);
     bool sendPushNotification(const NotificationMessage& notification);
     
     // Task function
     static void notificationTask(void* parameter);
 };
 
 #endif // NOTIFICATION_MANAGER_H   