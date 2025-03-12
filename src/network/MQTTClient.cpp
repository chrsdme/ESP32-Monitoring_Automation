/**
 * @file MQTTClient.cpp
 * @brief Implementation of the MQTTClient class
 */

 #include "MQTTClient.h"
 #include "../core/AppCore.h"
 
 // Static pointer to the current instance for use in the callback
 static MQTTClient* currentInstance = nullptr;
 
 MQTTClient::MQTTClient() :
     _mqttClient(_wifiClient),
     _broker(Constants::DEFAULT_MQTT_BROKER),
     _port(Constants::DEFAULT_MQTT_PORT),
     _username(Constants::DEFAULT_MQTT_USERNAME),
     _password(Constants::DEFAULT_MQTT_PASSWORD),
     _clientId(""),
     _baseTopic(Constants::DEFAULT_MQTT_TOPIC),
     _isConnected(false),
     _lastConnectAttempt(0),
     _lastPublishTime(0),
     _connectRetryInterval(5000),  // 5 seconds
     _publishInterval(30000),      // 30 seconds
     _mqttMutex(nullptr),
     _mqttTaskHandle(nullptr),
     _publishQueue(nullptr)
 {
     // Set the current instance for the static callback
     currentInstance = this;
     
     // Set the callback function for incoming messages
     _mqttClient.setCallback(mqttCallback);
 }
 
 MQTTClient::~MQTTClient() {
     // Disconnect from MQTT broker
     disconnect();
     
     // Clean up RTOS resources
     if (_mqttMutex != nullptr) {
         vSemaphoreDelete(_mqttMutex);
     }
     
     if (_mqttTaskHandle != nullptr) {
         vTaskDelete(_mqttTaskHandle);
     }
     
     if (_publishQueue != nullptr) {
         vQueueDelete(_publishQueue);
     }
 }
 
 bool MQTTClient::begin() {
     // Create mutex for thread-safe operations
     _mqttMutex = xSemaphoreCreateMutex();
     if (_mqttMutex == nullptr) {
         Serial.println("Failed to create MQTT mutex!");
         return false;
     }
     
     // Create queue for publish messages
     _publishQueue = xQueueCreate(20, sizeof(MqttMessage));
     if (_publishQueue == nullptr) {
         Serial.println("Failed to create MQTT publish queue!");
         return false;
     }
     
     // Generate client ID if not set
     if (_clientId.isEmpty()) {
         _clientId = "mushroomtent_" + String(ESP.getEfuseMac(), HEX);
     }
     
     return true;
 }
 
 bool MQTTClient::connect() {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_mqttMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Check if already connected
         if (_mqttClient.connected()) {
             _isConnected = true;
             
             // Release mutex
             xSemaphoreGive(_mqttMutex);
             return true;
         }
         
         // Configure server
         _mqttClient.setServer(_broker.c_str(), _port);
         
         // Try to connect
         bool result = false;
         if (_username.length() > 0 && _password.length() > 0) {
             result = _mqttClient.connect(_clientId.c_str(), _username.c_str(), _password.c_str());
         } else {
             result = _mqttClient.connect(_clientId.c_str());
         }
         
         if (result) {
             _isConnected = true;
             _lastConnectAttempt = millis();
             
             getAppCore()->getLogManager()->log(LogLevel::INFO, "MQTT", 
                 "Connected to MQTT broker: " + _broker);
             
             // Re-subscribe to all topics
             for (const auto& subtopic : _subscriptions) {
                 _mqttClient.subscribe(getFullTopic(subtopic).c_str());
             }
             
             // Publish a connection message
             _mqttClient.publish(getFullTopic("status").c_str(), "online", true);
         } else {
             _isConnected = false;
             _lastConnectAttempt = millis();
             
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "MQTT", 
                 "Failed to connect to MQTT broker: " + _broker);
         }
         
         // Release mutex
         xSemaphoreGive(_mqttMutex);
         return result;
     }
     
     return false;
 }
 
 void MQTTClient::disconnect() {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_mqttMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         if (_mqttClient.connected()) {
             // Publish an offline status message
             _mqttClient.publish(getFullTopic("status").c_str(), "offline", true);
             
             // Disconnect
             _mqttClient.disconnect();
             _isConnected = false;
             
             getAppCore()->getLogManager()->log(LogLevel::INFO, "MQTT", 
                 "Disconnected from MQTT broker");
         }
         
         // Release mutex
         xSemaphoreGive(_mqttMutex);
     }
 }
 
 bool MQTTClient::isConnected() {
     bool connected = false;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_mqttMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         connected = _isConnected && _mqttClient.connected();
         
         // Update connection status if it's different from what we think
         if (_isConnected != connected) {
             _isConnected = connected;
         }
         
         // Release mutex
         xSemaphoreGive(_mqttMutex);
     }
     
     return connected;
 }
 
 bool MQTTClient::setConfig(const String& broker, uint16_t port, const String& username, 
                          const String& password, const String& clientId) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_mqttMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Store the new configuration
         _broker = broker;
         _port = port;
         _username = username;
         _password = password;
         
         if (clientId.length() > 0) {
             _clientId = clientId;
         }
         
         // Disconnect if already connected
         if (_mqttClient.connected()) {
             _mqttClient.disconnect();
             _isConnected = false;
         }
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "MQTT", 
             "MQTT configuration updated: " + _broker + ":" + String(_port));
         
         // Release mutex
         xSemaphoreGive(_mqttMutex);
         return true;
     }
     
     return false;
 }
 
 void MQTTClient::setBaseTopic(const String& baseTopic) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_mqttMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         _baseTopic = baseTopic;
         
         // Make sure base topic ends with a slash
         if (_baseTopic.length() > 0 && _baseTopic.charAt(_baseTopic.length() - 1) != '/') {
             _baseTopic += "/";
         }
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "MQTT", 
             "MQTT base topic set to: " + _baseTopic);
         
         // Release mutex
         xSemaphoreGive(_mqttMutex);
     }
 }
 
 String MQTTClient::getBaseTopic() {
     String baseTopic = "";
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_mqttMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         baseTopic = _baseTopic;
         
         // Release mutex
         xSemaphoreGive(_mqttMutex);
     }
     
     return baseTopic;
 }
 
 bool MQTTClient::publish(const String& subtopic, const String& payload, bool retain) {
     // Create a message structure
     MqttMessage message;
     message.topic = getFullTopic(subtopic);
     message.payload = payload;
     message.retain = retain;
     
     // Add to publish queue
     if (_publishQueue && xQueueSend(_publishQueue, &message, 0) == pdPASS) {
         return true;
     }
     
     // If queue is full or not available, try to publish directly
     return _mqttClient.publish(message.topic.c_str(), message.payload.c_str(), message.retain);
 }
 
 bool MQTTClient::subscribe(const String& subtopic) {
     bool result = false;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_mqttMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Add to subscriptions list if not already present
         bool alreadySubscribed = false;
         for (const auto& topic : _subscriptions) {
             if (topic == subtopic) {
                 alreadySubscribed = true;
                 break;
             }
         }
         
         if (!alreadySubscribed) {
             _subscriptions.push_back(subtopic);
         }
         
         // Subscribe if connected
         if (_mqttClient.connected()) {
             result = _mqttClient.subscribe(getFullTopic(subtopic).c_str());
             
             if (result) {
                 getAppCore()->getLogManager()->log(LogLevel::INFO, "MQTT", 
                     "Subscribed to: " + getFullTopic(subtopic));
             } else {
                 getAppCore()->getLogManager()->log(LogLevel::ERROR, "MQTT", 
                     "Failed to subscribe to: " + getFullTopic(subtopic));
             }
         } else {
             // If not connected, subscription will happen on connect
             result = true;
         }
         
         // Release mutex
         xSemaphoreGive(_mqttMutex);
     }
     
     return result;
 }
 
 bool MQTTClient::unsubscribe(const String& subtopic) {
     bool result = false;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_mqttMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Remove from subscriptions list
         for (auto it = _subscriptions.begin(); it != _subscriptions.end(); ++it) {
             if (*it == subtopic) {
                 _subscriptions.erase(it);
                 break;
             }
         }
         
         // Unsubscribe if connected
         if (_mqttClient.connected()) {
             result = _mqttClient.unsubscribe(getFullTopic(subtopic).c_str());
             
             if (result) {
                 getAppCore()->getLogManager()->log(LogLevel::INFO, "MQTT", 
                     "Unsubscribed from: " + getFullTopic(subtopic));
             } else {
                 getAppCore()->getLogManager()->log(LogLevel::ERROR, "MQTT", 
                     "Failed to unsubscribe from: " + getFullTopic(subtopic));
             }
         } else {
             // If not connected, it's already effectively unsubscribed
             result = true;
         }
         
         // Release mutex
         xSemaphoreGive(_mqttMutex);
     }
     
     return result;
 }
 
 bool MQTTClient::publishSensorData() {
     // Get sensor readings
     SensorReading upperDht, lowerDht, scd;
     bool success = getAppCore()->getSensorManager()->getSensorReadings(upperDht, lowerDht, scd);
     
     if (!success) {
         return false;
     }
     
     // Create a JSON document with sensor data
     DynamicJsonDocument doc(1024);
     
     // Add timestamp
     doc["timestamp"] = millis();
     
     // Upper DHT sensor
     JsonObject upperDhtObj = doc.createNestedObject("upper_dht");
     upperDhtObj["valid"] = upperDht.valid;
     if (upperDht.valid) {
         upperDhtObj["temperature"] = upperDht.temperature;
         upperDhtObj["humidity"] = upperDht.humidity;
     }
     
     // Lower DHT sensor
     JsonObject lowerDhtObj = doc.createNestedObject("lower_dht");
     lowerDhtObj["valid"] = lowerDht.valid;
     if (lowerDht.valid) {
         lowerDhtObj["temperature"] = lowerDht.temperature;
         lowerDhtObj["humidity"] = lowerDht.humidity;
     }
     
     // SCD40 sensor
     JsonObject scdObj = doc.createNestedObject("scd40");
     scdObj["valid"] = scd.valid;
     if (scd.valid) {
         scdObj["temperature"] = scd.temperature;
         scdObj["humidity"] = scd.humidity;
         scdObj["co2"] = scd.co2;
     }
     
     // Serialize to JSON
     String payload;
     serializeJson(doc, payload);
     
     // Publish to MQTT
     return publish("sensors", payload);
 }
 
 bool MQTTClient::publishRelayStatus() {
     // Get relay status
     std::vector<RelayConfig> relays = getAppCore()->getRelayManager()->getAllRelayConfigs();
     
     // Create a JSON document with relay status
     DynamicJsonDocument doc(2048);
     
     // Add timestamp
     doc["timestamp"] = millis();
     
     // Add relay array
     JsonArray relaysArray = doc.createNestedArray("relays");
     
     for (const auto& relay : relays) {
         JsonObject relayObj = relaysArray.createNestedObject();
         relayObj["id"] = relay.relayId;
         relayObj["name"] = relay.name;
         relayObj["state"] = static_cast<int>(relay.state);
         relayObj["is_on"] = relay.isOn;
         relayObj["last_trigger"] = static_cast<int>(relay.lastTrigger);
     }
     
     // Serialize to JSON
     String payload;
     serializeJson(doc, payload);
     
     // Publish to MQTT
     return publish("relays", payload);
 }
 
 bool MQTTClient::publishSystemStatus() {
     // Get system information
     FilesystemStats fsStats = getAppCore()->getStorageManager()->getFilesystemStats();
     
     // Create a JSON document with system status
     DynamicJsonDocument doc(1024);
     
     // Add timestamp
     doc["timestamp"] = millis();
     
     // Add uptime
     doc["uptime"] = millis() / 1000;  // seconds
     
     // Add firmware version
     doc["version"] = Constants::APP_VERSION;
     
     // Add WiFi information
     JsonObject wifiObj = doc.createNestedObject("wifi");
     wifiObj["ssid"] = getAppCore()->getNetworkManager()->getConnectedSSID();
     wifiObj["rssi"] = getAppCore()->getNetworkManager()->getRSSI();
     wifiObj["ip"] = getAppCore()->getNetworkManager()->getIPAddress();
     
     // Add filesystem information
     JsonObject fsObj = doc.createNestedObject("filesystem");
     fsObj["total"] = fsStats.totalBytes;
     fsObj["used"] = fsStats.usedBytes;
     fsObj["free"] = fsStats.freeBytes;
     
     // Add memory information
     JsonObject memoryObj = doc.createNestedObject("memory");
     memoryObj["free_heap"] = ESP.getFreeHeap();
     memoryObj["min_free_heap"] = ESP.getMinFreeHeap();
     memoryObj["max_alloc_heap"] = ESP.getMaxAllocHeap();
     
     // Serialize to JSON
     String payload;
     serializeJson(doc, payload);
     
     // Publish to MQTT
     return publish("system", payload);
 }
 
 void MQTTClient::createTasks() {
     // Create MQTT task
     BaseType_t result = xTaskCreatePinnedToCore(
         mqttTask,                      // Task function
         "MQTTTask",                    // Task name
         Constants::STACK_SIZE_MQTT,    // Stack size (words)
         this,                          // Task parameters
         Constants::PRIORITY_MQTT,      // Priority
         &_mqttTaskHandle,              // Task handle
         0                              // Core ID (0 - protocol core)
     );
     
     if (result != pdPASS) {
         getAppCore()->getLogManager()->log(LogLevel::ERROR, "MQTT", 
             "Failed to create MQTT task");
     }
 }
 
 void MQTTClient::processIncomingMessage(const String& topic, const String& payload) {
     // Extract subtopic by removing base topic prefix
     String subtopic = topic;
     if (topic.startsWith(_baseTopic)) {
         subtopic = topic.substring(_baseTopic.length());
     }
     
     // Log the message
     getAppCore()->getLogManager()->log(LogLevel::INFO, "MQTT", 
         "Received message on topic: " + subtopic + ", payload: " + payload);
     
     // Handle different types of messages
     if (subtopic == "command") {
         handleCommand(payload, "");
     } else if (subtopic.startsWith("command/")) {
         String command = subtopic.substring(8);
         handleCommand(command, payload);
     }
 }
 
 void MQTTClient::handleCommand(const String& command, const String& payload) {
     // Handle different commands
     if (command == "reboot" || payload == "reboot") {
         getAppCore()->getLogManager()->log(LogLevel::INFO, "MQTT", 
             "Received reboot command");
         
         // Schedule a reboot
         getAppCore()->reboot();
     } else if (command == "relay") {
         // Parse relay command (format: "relay_id:state")
         int separatorPos = payload.indexOf(':');
         if (separatorPos > 0) {
             String relayIdStr = payload.substring(0, separatorPos);
             String stateStr = payload.substring(separatorPos + 1);
             
             uint8_t relayId = relayIdStr.toInt();
             int state = stateStr.toInt();
             
             if (relayId >= 1 && relayId <= 8 && state >= 0 && state <= 2) {
                 getAppCore()->getLogManager()->log(LogLevel::INFO, "MQTT", 
                     "Setting relay " + String(relayId) + " to state " + String(state));
                 
                 getAppCore()->getRelayManager()->setRelayState(relayId, static_cast<RelayState>(state));
             }
         }
     }
 }
 
 String MQTTClient::getFullTopic(const String& subtopic) {
     return _baseTopic + subtopic;
 }
 
 void MQTTClient::mqttCallback(char* topic, byte* payload, unsigned int length) {
     // Convert payload to string
     String payloadStr;
     for (unsigned int i = 0; i < length; i++) {
         payloadStr += (char)payload[i];
     }
     
     // Pass to the current instance
     if (currentInstance) {
         currentInstance->processIncomingMessage(String(topic), payloadStr);
     }
 }
 
 void MQTTClient::mqttTask(void* parameter) {
     MQTTClient* mqttClient = static_cast<MQTTClient*>(parameter);
     TickType_t lastWakeTime = xTaskGetTickCount();
     
     while (true) {
         // Check if we need to connect
         if (!mqttClient->isConnected()) {
             // Try to connect if enough time has passed since last attempt
             if (millis() - mqttClient->_lastConnectAttempt > mqttClient->_connectRetryInterval) {
                 mqttClient->connect();
             }
         } else {
             // Keep the connection alive
             mqttClient->_mqttClient.loop();
             
             // Process publish queue
             MqttMessage message;
             while (xQueueReceive(mqttClient->_publishQueue, &message, 0) == pdPASS) {
                 mqttClient->_mqttClient.publish(message.topic.c_str(), message.payload.c_str(), message.retain);
             }
             
             // Publish periodic updates
             if (millis() - mqttClient->_lastPublishTime > mqttClient->_publishInterval) {
                 mqttClient->publishSensorData();
                 mqttClient->publishRelayStatus();
                 mqttClient->publishSystemStatus();
                 mqttClient->_lastPublishTime = millis();
             }
         }
         
         // Sleep to avoid hogging CPU
         vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(100));
     }
 }