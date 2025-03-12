/**
 * @file MQTTClient.h
 * @brief MQTT client for publishing sensor data and subscribing to commands
 */

 #ifndef MQTT_CLIENT_H
 #define MQTT_CLIENT_H
 
 #include <Arduino.h>
 #include <WiFi.h>
 #include <PubSubClient.h>
 #include <freertos/FreeRTOS.h>
 #include <freertos/task.h>
 #include <freertos/semphr.h>
 #include <freertos/queue.h>
 #include <vector>
 #include "../utils/Constants.h"
 
 // Forward declarations
 class AppCore;
 
 /**
  * @struct MqttMessage
  * @brief Structure to hold an MQTT message
  */
 struct MqttMessage {
     String topic;
     String payload;
     bool retain;
 };
 
 /**
  * @class MQTTClient
  * @brief Manages MQTT connection, publishing, and subscription
  */
 class MQTTClient {
 public:
     MQTTClient();
     ~MQTTClient();
     
     /**
      * @brief Initialize the MQTT client
      * @return True if initialized successfully
      */
     bool begin();
     
     /**
      * @brief Connect to the MQTT broker
      * @return True if connected successfully
      */
     bool connect();
     
     /**
      * @brief Disconnect from the MQTT broker
      */
     void disconnect();
     
     /**
      * @brief Check if connected to the MQTT broker
      * @return True if connected
      */
     bool isConnected();
     
     /**
      * @brief Set MQTT configuration
      * @param broker Broker address
      * @param port Broker port
      * @param username Username (optional)
      * @param password Password (optional)
      * @param clientId Client ID (optional)
      * @return True if configuration set successfully
      */
     bool setConfig(const String& broker, uint16_t port = 1883, 
                   const String& username = "", const String& password = "",
                   const String& clientId = "");
     
     /**
      * @brief Set the base topic for all publications and subscriptions
      * @param baseTopic Base topic
      */
     void setBaseTopic(const String& baseTopic);
     
     /**
      * @brief Get the base topic
      * @return Base topic
      */
     String getBaseTopic();
     
     /**
      * @brief Publish a message to a topic
      * @param subtopic Topic suffix (added to base topic)
      * @param payload Message payload
      * @param retain Whether to retain the message
      * @return True if message published successfully
      */
     bool publish(const String& subtopic, const String& payload, bool retain = false);
     
     /**
      * @brief Subscribe to a topic
      * @param subtopic Topic suffix (added to base topic)
      * @return True if subscribed successfully
      */
     bool subscribe(const String& subtopic);
     
     /**
      * @brief Unsubscribe from a topic
      * @param subtopic Topic suffix (added to base topic)
      * @return True if unsubscribed successfully
      */
     bool unsubscribe(const String& subtopic);
     
     /**
      * @brief Publish sensor data
      * @return True if data published successfully
      */
     bool publishSensorData();
     
     /**
      * @brief Publish relay status
      * @return True if status published successfully
      */
     bool publishRelayStatus();
     
     /**
      * @brief Publish system status
      * @return True if status published successfully
      */
     bool publishSystemStatus();
     
     /**
      * @brief Create RTOS tasks for MQTT operations
      */
     void createTasks();
     
 private:
     // MQTT client instance
     WiFiClient _wifiClient;
     PubSubClient _mqttClient;
     
     // Configuration
     String _broker;
     uint16_t _port;
     String _username;
     String _password;
     String _clientId;
     String _baseTopic;
     
     // Subscription list
     std::vector<String> _subscriptions;
     
     // Status tracking
     bool _isConnected;
     uint32_t _lastConnectAttempt;
     uint32_t _lastPublishTime;
     uint32_t _connectRetryInterval;
     uint32_t _publishInterval;
     
     // RTOS resources
     SemaphoreHandle_t _mqttMutex;
     TaskHandle_t _mqttTaskHandle;
     QueueHandle_t _publishQueue;
     
     // Private methods
     void processIncomingMessage(const String& topic, const String& payload);
     void handleCommand(const String& command, const String& payload);
     String getFullTopic(const String& subtopic);
     
     // Static callback for PubSubClient
     static void mqttCallback(char* topic, byte* payload, unsigned int length);
     
     // Task function
     static void mqttTask(void* parameter);
 };
 
 #endif // MQTT_CLIENT_H