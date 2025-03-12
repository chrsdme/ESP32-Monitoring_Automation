/**
 * @file NotificationManager.cpp
 * @brief Implementation of the NotificationManager class
 */

 #include "NotificationManager.h"
 #include "../core/AppCore.h"
 #include "../system/LogManager.h"
 #include "../network/MQTTClient.h"
 #include <HTTPClient.h>
 #include <ArduinoJson.h>
 
 NotificationManager::NotificationManager() :
     _minLevel(NotificationLevel::WARNING),
     _maxHistorySize(20),
     _notificationMutex(nullptr),
     _notificationTaskHandle(nullptr),
     _notificationQueue(nullptr)
 {
 }
 
 NotificationManager::~NotificationManager() {
     // Clean up RTOS resources
     if (_notificationMutex != nullptr) {
         vSemaphoreDelete(_notificationMutex);
     }
     
     if (_notificationTaskHandle != nullptr) {
         vTaskDelete(_notificationTaskHandle);
     }
     
     if (_notificationQueue != nullptr) {
         vQueueDelete(_notificationQueue);
     }
 }
 
 bool NotificationManager::begin() {
     // Create mutex for thread-safe operations
     _notificationMutex = xSemaphoreCreateMutex();
     if (_notificationMutex == nullptr) {
         Serial.println("Failed to create notification mutex!");
         return false;
     }
     
     // Create queue for notification messages
     _notificationQueue = xQueueCreate(10, sizeof(NotificationMessage));
     if (_notificationQueue == nullptr) {
         Serial.println("Failed to create notification queue!");
         return false;
     }
     
     // Load notification configurations from NVS if available
     nvs_handle_t nvsHandle;
     esp_err_t err = nvs_open(Constants::NVS_CONFIG_NAMESPACE, NVS_READONLY, &nvsHandle);
     if (err == ESP_OK) {
         uint8_t minLevel;
         if (nvs_get_u8(nvsHandle, "notification_level", &minLevel) == ESP_OK) {
             _minLevel = static_cast<NotificationLevel>(minLevel);
         }
         
         // Load configurations for each channel if they exist
         for (uint8_t i = 1; i <= 5; i++) {
             NotificationChannel channel = static_cast<NotificationChannel>(i);
             String prefix = "notify_" + String(i) + "_";
             
             uint8_t enabled;
             if (nvs_get_u8(nvsHandle, (prefix + "enabled").c_str(), &enabled) == ESP_OK && enabled == 1) {
                 NotificationConfig config;
                 config.channel = channel;
                 config.enabled = true;
                 
                 // Get recipient
                 size_t recipientLen = 0;
                 if (nvs_get_str(nvsHandle, (prefix + "recipient").c_str(), nullptr, &recipientLen) == ESP_OK && recipientLen > 0) {
                     char* recipientBuf = new char[recipientLen];
                     nvs_get_str(nvsHandle, (prefix + "recipient").c_str(), recipientBuf, &recipientLen);
                     config.recipient = String(recipientBuf);
                     delete[] recipientBuf;
                 }
                 
                 // Get credentials
                 size_t credentialsLen = 0;
                 if (nvs_get_str(nvsHandle, (prefix + "credentials").c_str(), nullptr, &credentialsLen) == ESP_OK && credentialsLen > 0) {
                     char* credentialsBuf = new char[credentialsLen];
                     nvs_get_str(nvsHandle, (prefix + "credentials").c_str(), credentialsBuf, &credentialsLen);
                     config.credentials = String(credentialsBuf);
                     delete[] credentialsBuf;
                 }
                 
                 // Get endpoint
                 size_t endpointLen = 0;
                 if (nvs_get_str(nvsHandle, (prefix + "endpoint").c_str(), nullptr, &endpointLen) == ESP_OK && endpointLen > 0) {
                     char* endpointBuf = new char[endpointLen];
                     nvs_get_str(nvsHandle, (prefix + "endpoint").c_str(), endpointBuf, &endpointLen);
                     config.endpoint = String(endpointBuf);
                     delete[] endpointBuf;
                 }
                 
                 // Add to channel configs
                 _channelConfigs[channel] = config;
             }
         }
         
         nvs_close(nvsHandle);
     }
     
     getAppCore()->getLogManager()->log(LogLevel::INFO, "Notification", 
         "Notification manager initialized");
     
     return true;
 }
 
 bool NotificationManager::enableChannel(NotificationChannel channel, const NotificationConfig& config) {
     if (channel == NotificationChannel::NONE) {
         return false;
     }
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_notificationMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Store configuration
         _channelConfigs[channel] = config;
         _channelConfigs[channel].enabled = true;
         
         // Save to NVS
         nvs_handle_t nvsHandle;
         esp_err_t err = nvs_open(Constants::NVS_CONFIG_NAMESPACE, NVS_READWRITE, &nvsHandle);
         if (err == ESP_OK) {
             String prefix = "notify_" + String(static_cast<uint8_t>(channel)) + "_";
             
             nvs_set_u8(nvsHandle, (prefix + "enabled").c_str(), 1);
             nvs_set_str(nvsHandle, (prefix + "recipient").c_str(), config.recipient.c_str());
             nvs_set_str(nvsHandle, (prefix + "credentials").c_str(), config.credentials.c_str());
             nvs_set_str(nvsHandle, (prefix + "endpoint").c_str(), config.endpoint.c_str());
             
             nvs_commit(nvsHandle);
             nvs_close(nvsHandle);
         }
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Notification", 
             "Notification channel " + String(static_cast<uint8_t>(channel)) + " enabled");
         
         xSemaphoreGive(_notificationMutex);
         return true;
     }
     
     return false;
 }
 
 bool NotificationManager::disableChannel(NotificationChannel channel) {
     if (channel == NotificationChannel::NONE) {
         return false;
     }
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_notificationMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Find channel config if it exists
         auto it = _channelConfigs.find(channel);
         if (it != _channelConfigs.end()) {
             // Disable channel
             it->second.enabled = false;
             
             // Update NVS
             nvs_handle_t nvsHandle;
             esp_err_t err = nvs_open(Constants::NVS_CONFIG_NAMESPACE, NVS_READWRITE, &nvsHandle);
             if (err == ESP_OK) {
                 String prefix = "notify_" + String(static_cast<uint8_t>(channel)) + "_";
                 nvs_set_u8(nvsHandle, (prefix + "enabled").c_str(), 0);
                 nvs_commit(nvsHandle);
                 nvs_close(nvsHandle);
             }
             
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Notification", 
                 "Notification channel " + String(static_cast<uint8_t>(channel)) + " disabled");
             
             xSemaphoreGive(_notificationMutex);
             return true;
         }
         
         xSemaphoreGive(_notificationMutex);
     }
     
     return false;
 }
 
 bool NotificationManager::testChannel(NotificationChannel channel) {
     // Create a test notification
     NotificationMessage testMsg(
         NotificationLevel::INFO,
         "System",
         "Test Notification",
         "This is a test notification from " + String(Constants::APP_NAME)
     );
     
     bool result = false;
     
     // Send test message via the specified channel
     switch (channel) {
         case NotificationChannel::EMAIL:
             result = sendEmail(testMsg);
             break;
             
         case NotificationChannel::TELEGRAM:
             result = sendTelegram(testMsg);
             break;
             
         case NotificationChannel::MQTT:
             result = sendMqtt(testMsg);
             break;
             
         case NotificationChannel::HTTP_WEBHOOK:
             result = sendWebhook(testMsg);
             break;
             
         case NotificationChannel::PUSH_NOTIFICATION:
             result = sendPushNotification(testMsg);
             break;
             
         default:
             result = false;
             break;
     }
     
     if (result) {
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Notification", 
             "Test notification sent via channel " + String(static_cast<uint8_t>(channel)));
     } else {
         getAppCore()->getLogManager()->log(LogLevel::ERROR, "Notification", 
             "Failed to send test notification via channel " + String(static_cast<uint8_t>(channel)));
     }
     
     return result;
 }
 
 bool NotificationManager::setMinLevel(NotificationLevel level) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_notificationMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         _minLevel = level;
         
         // Save to NVS
         nvs_handle_t nvsHandle;
         esp_err_t err = nvs_open(Constants::NVS_CONFIG_NAMESPACE, NVS_READWRITE, &nvsHandle);
         if (err == ESP_OK) {
             nvs_set_u8(nvsHandle, "notification_level", static_cast<uint8_t>(level));
             nvs_commit(nvsHandle);
             nvs_close(nvsHandle);
         }
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Notification", 
             "Minimum notification level set to " + String(static_cast<uint8_t>(level)));
         
         xSemaphoreGive(_notificationMutex);
         return true;
     }
     
     return false;
 }
 
 NotificationLevel NotificationManager::getMinLevel() {
     NotificationLevel level;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_notificationMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         level = _minLevel;
         xSemaphoreGive(_notificationMutex);
     } else {
         level = NotificationLevel::WARNING;  // Default if mutex fails
     }
     
     return level;
 }
 
 bool NotificationManager::sendNotification(NotificationLevel level, const String& source, 
                                         const String& title, const String& message) {
     // Check if level meets minimum threshold
     if (level < _minLevel) {
         return false;
     }
     
     // Create notification message
     NotificationMessage notification(level, source, title, message);
     
     // Add to message history
     if (xSemaphoreTake(_notificationMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         _recentNotifications.push_back(notification);
         
         // Limit history size
         while (_recentNotifications.size() > _maxHistorySize) {
             _recentNotifications.erase(_recentNotifications.begin());
         }
         
         xSemaphoreGive(_notificationMutex);
     }
     
     // Log the notification
     LogLevel logLevel;
     switch (level) {
         case NotificationLevel::INFO:
             logLevel = LogLevel::INFO;
             break;
         case NotificationLevel::WARNING:
             logLevel = LogLevel::WARN;
             break;
         case NotificationLevel::ALERT:
         case NotificationLevel::CRITICAL:
             logLevel = LogLevel::ERROR;
             break;
         default:
             logLevel = LogLevel::INFO;
             break;
     }
     
     getAppCore()->getLogManager()->log(logLevel, "Notification", 
         "[" + source + "] " + title + ": " + message);
     
     // Add to notification queue for async processing
     if (_notificationQueue && notification.level >= _minLevel) {
         return xQueueSend(_notificationQueue, &notification, 0) == pdTRUE;
     }
     
     return false;
 }
 
 std::vector<NotificationMessage> NotificationManager::getRecentNotifications(size_t maxCount) {
     std::vector<NotificationMessage> result;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_notificationMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Calculate how many notifications to return
         size_t count = std::min(_recentNotifications.size(), maxCount);
         size_t startIndex = _recentNotifications.size() - count;
         
         // Copy the most recent notifications
         for (size_t i = startIndex; i < _recentNotifications.size(); i++) {
             result.push_back(_recentNotifications[i]);
         }
         
         xSemaphoreGive(_notificationMutex);
     }
     
     return result;
 }
 
 NotificationConfig NotificationManager::getChannelConfig(NotificationChannel channel) {
     NotificationConfig config;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_notificationMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Find channel config if it exists
         auto it = _channelConfigs.find(channel);
         if (it != _channelConfigs.end()) {
             config = it->second;
         }
         
         xSemaphoreGive(_notificationMutex);
     }
     
     return config;
 }
 
 void NotificationManager::createTasks() {
     // Create notification processing task
     BaseType_t result = xTaskCreatePinnedToCore(
         notificationTask,             // Task function
         "NotificationTask",           // Task name
         4096,                         // Stack size (words)
         this,                         // Task parameters
         1,                            // Priority (low)
         &_notificationTaskHandle,     // Task handle
         0                             // Core ID (0 - protocol core)
     );
     
     if (result != pdPASS) {
         getAppCore()->getLogManager()->log(LogLevel::ERROR, "Notification", 
             "Failed to create notification task");
     }
 }
 
 bool NotificationManager::sendEmail(const NotificationMessage& notification) {
     // Get email configuration
     NotificationConfig config = getChannelConfig(NotificationChannel::EMAIL);
     if (!config.enabled || config.recipient.isEmpty() || config.endpoint.isEmpty()) {
         return false;
     }
     
     // This is a simplified implementation. In a real application, you'd use
     // a proper SMTP library or email service API. For now, we'll just log it.
     getAppCore()->getLogManager()->log(LogLevel::INFO, "Notification", 
         "Email would be sent to " + config.recipient + 
         " with title: " + notification.title);
     
     // Placeholder for actual email sending code
     return true;
 }
 
 bool NotificationManager::sendTelegram(const NotificationMessage& notification) {
     // Get Telegram configuration
     NotificationConfig config = getChannelConfig(NotificationChannel::TELEGRAM);
     if (!config.enabled || config.recipient.isEmpty() || config.credentials.isEmpty()) {
         return false;
     }
     
     // Format message
     String messageText = "*" + notification.title + "*\n" + notification.message;
     
     // Create HTTP client
     HTTPClient http;
     
     // Format URL with bot token and chat ID
     String url = "https://api.telegram.org/bot" + config.credentials + 
                  "/sendMessage?chat_id=" + config.recipient + 
                  "&parse_mode=Markdown&text=" + urlEncode(messageText);
     
     // Send request
     http.begin(url);
     int httpCode = http.GET();
     
     // Check result
     bool success = (httpCode == 200);
     
     http.end();
     return success;
 }
 
 bool NotificationManager::sendMqtt(const NotificationMessage& notification) {
     // Get MQTT configuration
     NotificationConfig config = getChannelConfig(NotificationChannel::MQTT);
     if (!config.enabled || config.recipient.isEmpty()) {
         return false;
     }
     
     // Create JSON message
     DynamicJsonDocument doc(512);
     doc["level"] = static_cast<uint8_t>(notification.level);
     doc["source"] = notification.source;
     doc["title"] = notification.title;
     doc["message"] = notification.message;
     doc["timestamp"] = notification.timestamp;
     
     String payload;
     serializeJson(doc, payload);
     
     // Send via MQTT
     MQTTClient* mqttClient = getAppCore()->getMQTTClient();
     if (mqttClient->isConnected()) {
         return mqttClient->publish(config.recipient, payload);
     }
     
     return false;
 }
 
 bool NotificationManager::sendWebhook(const NotificationMessage& notification) {
     // Get webhook configuration
     NotificationConfig config = getChannelConfig(NotificationChannel::HTTP_WEBHOOK);
     if (!config.enabled || config.endpoint.isEmpty()) {
         return false;
     }
     
     // Create JSON payload
     DynamicJsonDocument doc(512);
     doc["level"] = static_cast<uint8_t>(notification.level);
     doc["source"] = notification.source;
     doc["title"] = notification.title;
     doc["message"] = notification.message;
     doc["timestamp"] = notification.timestamp;
     
     String payload;
     serializeJson(doc, payload);
     
     // Create HTTP client
     HTTPClient http;
     
     // Set up request
     http.begin(config.endpoint);
     http.addHeader("Content-Type", "application/json");
     
     // Add authentication if provided
     if (!config.credentials.isEmpty()) {
         http.addHeader("Authorization", "Bearer " + config.credentials);
     }
     
     // Send POST request
     int httpCode = http.POST(payload);
     
     // Check result
     bool success = (httpCode >= 200 && httpCode < 300);
     
     http.end();
     return success;
 }
 
 bool NotificationManager::sendPushNotification(const NotificationMessage& notification) {
     // Get push notification configuration
     NotificationConfig config = getChannelConfig(NotificationChannel::PUSH_NOTIFICATION);
     if (!config.enabled || config.endpoint.isEmpty() || config.credentials.isEmpty()) {
         return false;
     }
     
     // This is a simplified implementation. In a real application, you'd use
     // a service like Firebase Cloud Messaging, OneSignal, etc.
     getAppCore()->getLogManager()->log(LogLevel::INFO, "Notification", 
         "Push notification would be sent with title: " + notification.title);
     
     // Placeholder for actual push notification sending code
     return true;
 }
 
 // Helper function to URL-encode a string
 String NotificationManager::urlEncode(const String& text) {
    String encoded = "";
    char c;
    char code0;
    char code1;
    
    for (int i = 0; i < text.length(); i++) {
        c = text.charAt(i);
        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } 
        // Space should be encoded as '+'
        else if (c == ' ') {
            encoded += '+';
        } 
        // Any other characters are percent-encoded
        else {
            code1 = (c & 0xf) + '0';
            if ((c & 0xf) > 9) {
                code1 = (c & 0xf) - 10 + 'A';
            }
            c = (c >> 4) & 0xf;
            code0 = c + '0';
            if (c > 9) {
                code0 = c - 10 + 'A';
            }
            encoded += '%';
            encoded += code0;
            encoded += code1;
        }
    }
    
    return encoded;
}

 
 void NotificationManager::notificationTask(void* parameter) {
     NotificationManager* notificationManager = static_cast<NotificationManager*>(parameter);
     
     // Buffer for received notifications
     NotificationMessage notification;
     
     while (true) {
         // Wait for a notification to be available in the queue
         if (xQueueReceive(notificationManager->_notificationQueue, &notification, portMAX_DELAY) == pdTRUE) {
             // Process notification and send through configured channels
             bool anySent = false;
             
             // Check each enabled channel
             for (const auto& entry : notificationManager->_channelConfigs) {
                 if (entry.second.enabled) {
                     bool sent = false;
                     
                     switch (entry.first) {
                         case NotificationChannel::EMAIL:
                             sent = notificationManager->sendEmail(notification);
                             break;
                             
                         case NotificationChannel::TELEGRAM:
                             sent = notificationManager->sendTelegram(notification);
                             break;
                             
                         case NotificationChannel::MQTT:
                             sent = notificationManager->sendMqtt(notification);
                             break;
                             
                         case NotificationChannel::HTTP_WEBHOOK:
                             sent = notificationManager->sendWebhook(notification);
                             break;
                             
                         case NotificationChannel::PUSH_NOTIFICATION:
                             sent = notificationManager->sendPushNotification(notification);
                             break;
                             
                         default:
                             break;
                     }
                     
                     if (sent) {
                         anySent = true;
                         getAppCore()->getLogManager()->log(LogLevel::INFO, "Notification", 
                             "Notification sent via channel " + String(static_cast<uint8_t>(entry.first)));
                     } else {
                         getAppCore()->getLogManager()->log(LogLevel::WARN, "Notification", 
                             "Failed to send notification via channel " + String(static_cast<uint8_t>(entry.first)));
                     }
                 }
             }
             
             // Update notification status
             if (xSemaphoreTake(notificationManager->_notificationMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                 // Find the notification in history and mark as sent
                 for (auto& histNotification : notificationManager->_recentNotifications) {
                     if (histNotification.timestamp == notification.timestamp &&
                         histNotification.title == notification.title &&
                         histNotification.message == notification.message) {
                         histNotification.sent = anySent;
                         break;
                     }
                 }
                 
                 xSemaphoreGive(notificationManager->_notificationMutex);
             }
         }
     }
 }