/**
 * @file WebServer.cpp
 * @brief Implementation of the WebServer class
 */

 #include "WebServer.h"
 #include "../core/AppCore.h"
 #include "../system/LogManager.h"
 #include "../system/StorageManager.h"
 #include "../network/NetworkManager.h"
 #include "../components/SensorManager.h"
 #include "../components/RelayManager.h"
 #include "../system/ProfileManager.h"
 #include "../ota/OTAManager.h"
 
 WebServer::WebServer() :
     _server(nullptr),
     _port(Constants::DEFAULT_WEB_SERVER_PORT),
     _username(Constants::DEFAULT_HTTP_USERNAME),
     _password(Constants::DEFAULT_HTTP_PASSWORD),
     _isRunning(false),
     _isInConfigMode(false),
     _webServerMutex(nullptr),
     _webServerTaskHandle(nullptr)
 {
 }
 
 WebServer::~WebServer() {
     // Clean up RTOS resources
     if (_webServerMutex != nullptr) {
         vSemaphoreDelete(_webServerMutex);
     }
     
     if (_webServerTaskHandle != nullptr) {
         vTaskDelete(_webServerTaskHandle);
     }
     
     // Clean up server
     if (_server != nullptr) {
         delete _server;
     }
 }
 
 bool WebServer::begin() {
     // Create mutex for thread-safe operations
     _webServerMutex = xSemaphoreCreateMutex();
     if (_webServerMutex == nullptr) {
         Serial.println("Failed to create web server mutex!");
         return false;
     }
     
     // Load authentication credentials from NVS
     nvs_handle_t nvsHandle;
     esp_err_t err = nvs_open(Constants::NVS_CONFIG_NAMESPACE, NVS_READONLY, &nvsHandle);
     if (err == ESP_OK) {
         size_t userLen = 0;
         err = nvs_get_str(nvsHandle, Constants::NVS_HTTP_USER_KEY, nullptr, &userLen);
         if (err == ESP_OK && userLen > 0) {
             char* userBuffer = new char[userLen];
             nvs_get_str(nvsHandle, Constants::NVS_HTTP_USER_KEY, userBuffer, &userLen);
             _username = String(userBuffer);
             delete[] userBuffer;
         }
         
         size_t passLen = 0;
         err = nvs_get_str(nvsHandle, Constants::NVS_HTTP_PASS_KEY, nullptr, &passLen);
         if (err == ESP_OK && passLen > 0) {
             char* passBuffer = new char[passLen];
             nvs_get_str(nvsHandle, Constants::NVS_HTTP_PASS_KEY, passBuffer, &passLen);
             _password = String(passBuffer);
             delete[] passBuffer;
         }
         
         nvs_close(nvsHandle);
     }
     
     return true;
 }
 
 bool WebServer::startConfigurationMode() {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_webServerMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Create server if it doesn't exist
         if (_server == nullptr) {
             _server = new AsyncWebServer(_port);
         } else if (_isRunning) {
             // Stop the server if it's already running
             getAppCore()->getLogManager()->log(LogLevel::INFO, "WebServer", "Stopping existing web server");
             _server->reset();
         }
         
         // Set up routes specific to configuration mode
         setupConfigModeRoutes();
         
         // Set up common routes
         setupCommonRoutes();
         
         // Start the server
         _server->begin();
         _isRunning = true;
         _isInConfigMode = true;
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "WebServer", 
             "Web server started in configuration mode on port " + String(_port));
         
         // Release mutex
         xSemaphoreGive(_webServerMutex);
         return true;
     }
     
     return false;
 }
 
 bool WebServer::startNormalMode() {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_webServerMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Create server if it doesn't exist
         if (_server == nullptr) {
             _server = new AsyncWebServer(_port);
         } else if (_isRunning) {
             // Stop the server if it's already running
             getAppCore()->getLogManager()->log(LogLevel::INFO, "WebServer", "Stopping existing web server");
             _server->reset();
         }
         
         // Set up routes specific to normal mode
         setupNormalModeRoutes();
         
         // Set up common routes
         setupCommonRoutes();
         
         // Start the server
         _server->begin();
         _isRunning = true;
         _isInConfigMode = false;
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "WebServer", 
             "Web server started in normal mode on port " + String(_port));
         
         // Release mutex
         xSemaphoreGive(_webServerMutex);
         return true;
     }
     
     return false;
 }
 
 bool WebServer::setPort(uint16_t port) {
     if (port == 0) {
         return false;
     }
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_webServerMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         _port = port;
         
         // If server is running, restart it
         if (_isRunning) {
             if (_isInConfigMode) {
                 startConfigurationMode();
             } else {
                 startNormalMode();
             }
         }
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "WebServer", 
             "Web server port set to " + String(_port));
         
         // Release mutex
         xSemaphoreGive(_webServerMutex);
         return true;
     }
     
     return false;
 }
 
 uint16_t WebServer::getPort() {
     uint16_t port = Constants::DEFAULT_WEB_SERVER_PORT;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_webServerMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         port = _port;
         
         // Release mutex
         xSemaphoreGive(_webServerMutex);
     }
     
     return port;
 }
 
 bool WebServer::setHttpAuth(const String& username, const String& password) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_webServerMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         _username = username;
         _password = password;
         
         // Save credentials to NVS
         nvs_handle_t nvsHandle;
         esp_err_t err = nvs_open(Constants::NVS_CONFIG_NAMESPACE, NVS_READWRITE, &nvsHandle);
         if (err == ESP_OK) {
             nvs_set_str(nvsHandle, Constants::NVS_HTTP_USER_KEY, username.c_str());
             nvs_set_str(nvsHandle, Constants::NVS_HTTP_PASS_KEY, password.c_str());
             nvs_commit(nvsHandle);
             nvs_close(nvsHandle);
         }
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "WebServer", 
             "HTTP authentication credentials updated");
         
         // Release mutex
         xSemaphoreGive(_webServerMutex);
         return true;
     }
     
     return false;
 }
 
 bool WebServer::getHttpAuth(String& username, String& password) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_webServerMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         username = _username;
         password = _password;
         
         // Release mutex
         xSemaphoreGive(_webServerMutex);
         return true;
     }
     
     return false;
 }
 
 void WebServer::createTasks() {
     // No tasks needed for now since we're using AsyncWebServer,
     // but we could add a monitoring task in the future if needed
 }
 
 void WebServer::setupConfigModeRoutes() {
     // API endpoint for WiFi scanning
     _server->on("/api/wifi/scan", HTTP_GET, std::bind(&WebServer::handleWiFiScan, this, std::placeholders::_1));
     
     // API endpoint for testing WiFi credentials
     _server->addHandler(new AsyncCallbackJsonWebHandler("/api/wifi/test", 
         std::bind(&WebServer::handleTestWiFi, this, std::placeholders::_1, std::placeholders::_2)));
     
     // API endpoint for saving initial configuration
     _server->addHandler(new AsyncCallbackJsonWebHandler("/api/config/save", 
         std::bind(&WebServer::handleSaveSettings, this, std::placeholders::_1, std::placeholders::_2)));
 }
 
 void WebServer::setupNormalModeRoutes() {
     // Require authentication for all endpoints
     _server->on("/api/sensors/data", HTTP_GET, std::bind(&WebServer::handleGetSensorData, this, std::placeholders::_1));
     _server->on("/api/sensors/graph", HTTP_GET, std::bind(&WebServer::handleGetGraphData, this, std::placeholders::_1));
     
     _server->on("/api/relays/status", HTTP_GET, std::bind(&WebServer::handleGetRelayStatus, this, std::placeholders::_1));
     _server->addHandler(new AsyncCallbackJsonWebHandler("/api/relays/set", 
         std::bind(&WebServer::handleSetRelayState, this, std::placeholders::_1, std::placeholders::_2)));
     
     _server->on("/api/settings", HTTP_GET, std::bind(&WebServer::handleGetSettings, this, std::placeholders::_1));
     _server->addHandler(new AsyncCallbackJsonWebHandler("/api/settings/update", 
         std::bind(&WebServer::handleUpdateSettings, this, std::placeholders::_1, std::placeholders::_2)));
     
     _server->on("/api/network/config", HTTP_GET, std::bind(&WebServer::handleGetNetworkConfig, this, std::placeholders::_1));
     _server->addHandler(new AsyncCallbackJsonWebHandler("/api/network/update", 
         std::bind(&WebServer::handleUpdateNetworkConfig, this, std::placeholders::_1, std::placeholders::_2)));
     
     _server->on("/api/environment/thresholds", HTTP_GET, std::bind(&WebServer::handleGetEnvironmentalThresholds, this, std::placeholders::_1));
     _server->addHandler(new AsyncCallbackJsonWebHandler("/api/environment/update", 
         std::bind(&WebServer::handleUpdateEnvironmentalThresholds, this, std::placeholders::_1, std::placeholders::_2)));
     
     _server->on("/api/system/info", HTTP_GET, std::bind(&WebServer::handleGetSystemInfo, this, std::placeholders::_1));
     _server->on("/api/system/files", HTTP_GET, std::bind(&WebServer::handleGetFilesList, this, std::placeholders::_1));
     _server->on("/api/system/delete", HTTP_DELETE, std::bind(&WebServer::handleFileDelete, this, std::placeholders::_1));
     
     _server->on("/api/system/reboot", HTTP_POST, std::bind(&WebServer::handleReboot, this, std::placeholders::_1));
     _server->on("/api/system/factory-reset", HTTP_POST, std::bind(&WebServer::handleFactoryReset, this, std::placeholders::_1));
     
     // File upload handler
     _server->on("/api/upload", HTTP_POST, 
         [](AsyncWebServerRequest* request) { request->send(200); },
         std::bind(&WebServer::handleFileUpload, this, std::placeholders::_1, std::placeholders::_2, 
                   std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
     
     // Profile management
     _server->on("/api/profiles", HTTP_GET, std::bind(&WebServer::handleGetProfiles, this, std::placeholders::_1));
     _server->addHandler(new AsyncCallbackJsonWebHandler("/api/profiles/save", 
         std::bind(&WebServer::handleSaveProfile, this, std::placeholders::_1, std::placeholders::_2)));
     _server->addHandler(new AsyncCallbackJsonWebHandler("/api/profiles/load", 
         std::bind(&WebServer::handleLoadProfile, this, std::placeholders::_1, std::placeholders::_2)));
     _server->on("/api/profiles/export", HTTP_GET, std::bind(&WebServer::handleExportProfiles, this, std::placeholders::_1));
     _server->addHandler(new AsyncCallbackJsonWebHandler("/api/profiles/import", 
         std::bind(&WebServer::handleImportProfiles, this, std::placeholders::_1, std::placeholders::_2)));
 }
 
 void WebServer::setupCommonRoutes() {
     // Serve static files from SPIFFS
     _server->serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
     
     // Handle not found
     _server->onNotFound([](AsyncWebServerRequest* request) {
         request->send(404, "text/plain", "Not found");
     });
     
     // Set up CORS headers if needed
     DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
     DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE");
     DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
 }
 
 bool WebServer::authenticate(AsyncWebServerRequest* request) {
     if (!request->authenticate(_username.c_str(), _password.c_str())) {
         request->requestAuthentication();
         return false;
     }
     return true;
 }
 
 void WebServer::handleWiFiScan(AsyncWebServerRequest* request) {
     // No authentication required for AP mode
     
     // Get network scan results
     std::vector<NetworkInfo> networks = getAppCore()->getNetworkManager()->scanNetworks();
     
     // Create JSON response
     DynamicJsonDocument doc(4096);  // Adjust size based on expected number of networks
     JsonArray networksArray = doc.createNestedArray("networks");
     
     for (const auto& network : networks) {
         JsonObject networkObj = networksArray.createNestedObject();
         networkObj["ssid"] = network.ssid;
         networkObj["rssi"] = network.rssi;
         
         char bssidStr[18];
         sprintf(bssidStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
                 network.bssid[0], network.bssid[1], network.bssid[2], 
                 network.bssid[3], network.bssid[4], network.bssid[5]);
         networkObj["bssid"] = bssidStr;
         
         networkObj["channel"] = network.channel;
         networkObj["encrypted"] = (network.encryptionType != WIFI_AUTH_OPEN);
     }
     
     String response;
     serializeJson(doc, response);
     
     request->send(200, "application/json", response);
 }
 
 void WebServer::handleTestWiFi(AsyncWebServerRequest* request, JsonVariant& json) {
     // No authentication required for AP mode
     
     // Parse JSON request
     JsonObject jsonObj = json.as<JsonObject>();
     
     if (!jsonObj.containsKey("ssid") || !jsonObj.containsKey("password")) {
         request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing SSID or password\"}");
         return;
     }
     
     String ssid = jsonObj["ssid"].as<String>();
     String password = jsonObj["password"].as<String>();
     
     // Test the credentials
     bool success = getAppCore()->getNetworkManager()->testWiFiCredentials(ssid, password);
     
     // Return result
     String response = "{\"success\":" + String(success ? "true" : "false") + 
                       ",\"message\":\"" + (success ? "Connection successful" : "Connection failed") + "\"}";
     
     request->send(200, "application/json", response);
 }
 
 void WebServer::handleSaveSettings(AsyncWebServerRequest* request, JsonVariant& json) {
     // No authentication required for AP mode
     
     // Parse JSON request
     JsonObject jsonObj = json.as<JsonObject>();
     
     // Required fields
     if (!jsonObj.containsKey("wifi") || !jsonObj.containsKey("http_auth") || 
         !jsonObj.containsKey("gpio_config") || !jsonObj.containsKey("update_timings")) {
         request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing required configuration sections\"}");
         return;
     }
     
     // Process WiFi settings
     JsonObject wifiObj = jsonObj["wifi"];
     if (wifiObj.containsKey("ssid1") && wifiObj.containsKey("password1")) {
         getAppCore()->getNetworkManager()->setWiFiCredentials(0, 
             wifiObj["ssid1"].as<String>(), 
             wifiObj["password1"].as<String>());
     }
     
     if (wifiObj.containsKey("ssid2") && wifiObj.containsKey("password2")) {
         getAppCore()->getNetworkManager()->setWiFiCredentials(1, 
             wifiObj["ssid2"].as<String>(), 
             wifiObj["password2"].as<String>());
     }
     
     if (wifiObj.containsKey("ssid3") && wifiObj.containsKey("password3")) {
         getAppCore()->getNetworkManager()->setWiFiCredentials(2, 
             wifiObj["ssid3"].as<String>(), 
             wifiObj["password3"].as<String>());
     }
     
     if (wifiObj.containsKey("hostname")) {
         getAppCore()->getNetworkManager()->setHostname(wifiObj["hostname"].as<String>());
     }
     
 // Process HTTP authentication
     JsonObject authObj = jsonObj["http_auth"];
     if (authObj.containsKey("username") && authObj.containsKey("password")) {
         setHttpAuth(authObj["username"].as<String>(), authObj["password"].as<String>());
     }
     
     // Process MQTT settings
     if (jsonObj.containsKey("mqtt")) {
         JsonObject mqttObj = jsonObj["mqtt"];
         // Store MQTT settings in configuration
         // This would be handled by an MQTT manager in a real implementation
     }
     
     // Process GPIO configuration
     JsonObject gpioObj = jsonObj["gpio_config"];
     bool useDefault = gpioObj["use_default"].as<bool>();
     
     if (!useDefault) {
         // Set custom GPIO pins
         if (gpioObj.containsKey("upper_dht_pin") && gpioObj.containsKey("lower_dht_pin") && 
             gpioObj.containsKey("scd_sda_pin") && gpioObj.containsKey("scd_scl_pin")) {
             
             getAppCore()->getSensorManager()->setSensorPins(
                 gpioObj["upper_dht_pin"].as<uint8_t>(),
                 gpioObj["lower_dht_pin"].as<uint8_t>(),
                 gpioObj["scd_sda_pin"].as<uint8_t>(),
                 gpioObj["scd_scl_pin"].as<uint8_t>()
             );
         }
         
         // Set relay pins
         for (int i = 1; i <= 8; i++) {
             String pinKey = "relay" + String(i) + "_pin";
             if (gpioObj.containsKey(pinKey)) {
                 getAppCore()->getRelayManager()->setRelayPin(i, gpioObj[pinKey].as<uint8_t>());
             }
         }
     }
     
     // Process update timings
     JsonObject timingsObj = jsonObj["update_timings"];
     if (timingsObj.containsKey("dht_interval") && timingsObj.containsKey("scd_interval")) {
         getAppCore()->getSensorManager()->setSensorIntervals(
             timingsObj["dht_interval"].as<uint32_t>() * 1000,  // Convert to milliseconds
             timingsObj["scd_interval"].as<uint32_t>() * 1000
         );
     }
     
     if (timingsObj.containsKey("graph_interval") && timingsObj.containsKey("graph_points")) {
         // Configure graph settings (would be stored in configuration)
     }
     
     // Process relay configuration (optional)
     if (jsonObj.containsKey("relay_config")) {
         JsonObject relayObj = jsonObj["relay_config"];
         
         if (relayObj.containsKey("humidity_low") && relayObj.containsKey("humidity_high") && 
             relayObj.containsKey("temperature_low") && relayObj.containsKey("temperature_high") && 
             relayObj.containsKey("co2_low") && relayObj.containsKey("co2_high")) {
             
             getAppCore()->getRelayManager()->setEnvironmentalThresholds(
                 relayObj["humidity_low"].as<float>(),
                 relayObj["humidity_high"].as<float>(),
                 relayObj["temperature_low"].as<float>(),
                 relayObj["temperature_high"].as<float>(),
                 relayObj["co2_low"].as<float>(),
                 relayObj["co2_high"].as<float>()
             );
         }
         
         if (relayObj.containsKey("override_timer")) {
             getAppCore()->getRelayManager()->setOverrideDuration(relayObj["override_timer"].as<uint16_t>());
         }
     }
     
     // Save settings to storage
     getAppCore()->getStorageManager()->saveSettings();
     
     // Return success
     request->send(200, "application/json", "{\"success\":true,\"message\":\"Configuration saved successfully. Device will reboot.\"}");
     
     // Schedule a reboot after a short delay to allow the response to be sent
     vTaskDelay(pdMS_TO_TICKS(1000));
     getAppCore()->reboot();
 }
 
 void WebServer::handleGetSensorData(AsyncWebServerRequest* request) {
     if (!authenticate(request)) {
         return;
     }
     
     // Get sensor readings
     SensorReading upperDht, lowerDht, scd;
     getAppCore()->getSensorManager()->getSensorReadings(upperDht, lowerDht, scd);
     
     // Create JSON response
     DynamicJsonDocument doc(512);
     
     JsonObject upperDhtObj = doc.createNestedObject("upper_dht");
     upperDhtObj["temperature"] = upperDht.valid ? upperDht.temperature : 0;
     upperDhtObj["humidity"] = upperDht.valid ? upperDht.humidity : 0;
     upperDhtObj["valid"] = upperDht.valid;
     
     JsonObject lowerDhtObj = doc.createNestedObject("lower_dht");
     lowerDhtObj["temperature"] = lowerDht.valid ? lowerDht.temperature : 0;
     lowerDhtObj["humidity"] = lowerDht.valid ? lowerDht.humidity : 0;
     lowerDhtObj["valid"] = lowerDht.valid;
     
     JsonObject scdObj = doc.createNestedObject("scd");
     scdObj["temperature"] = scd.valid ? scd.temperature : 0;
     scdObj["humidity"] = scd.valid ? scd.humidity : 0;
     scdObj["co2"] = scd.valid ? scd.co2 : 0;
     scdObj["valid"] = scd.valid;
     
     // Calculate averages
     float avgTemp = 0;
     float avgHumidity = 0;
     int validCount = 0;
     
     if (upperDht.valid) {
         avgTemp += upperDht.temperature;
         avgHumidity += upperDht.humidity;
         validCount++;
     }
     
     if (lowerDht.valid) {
         avgTemp += lowerDht.temperature;
         avgHumidity += lowerDht.humidity;
         validCount++;
     }
     
     if (scd.valid) {
         avgTemp += scd.temperature;
         avgHumidity += scd.humidity;
         validCount++;
     }
     
     if (validCount > 0) {
         avgTemp /= validCount;
         avgHumidity /= validCount;
     }
     
     JsonObject avgObj = doc.createNestedObject("average");
     avgObj["temperature"] = avgTemp;
     avgObj["humidity"] = avgHumidity;
     
     // Get environmental thresholds for reference
     float humidityLow, humidityHigh, temperatureLow, temperatureHigh, co2Low, co2High;
     getAppCore()->getRelayManager()->getEnvironmentalThresholds(
         humidityLow, humidityHigh, temperatureLow, temperatureHigh, co2Low, co2High);
     
     JsonObject thresholdsObj = doc.createNestedObject("thresholds");
     thresholdsObj["humidity_low"] = humidityLow;
     thresholdsObj["humidity_high"] = humidityHigh;
     thresholdsObj["temperature_low"] = temperatureLow;
     thresholdsObj["temperature_high"] = temperatureHigh;
     thresholdsObj["co2_low"] = co2Low;
     thresholdsObj["co2_high"] = co2High;
     
     String response;
     serializeJson(doc, response);
     
     request->send(200, "application/json", response);
 }
 
 void WebServer::handleGetGraphData(AsyncWebServerRequest* request) {
     if (!authenticate(request)) {
         return;
     }
     
     // Get parameters
     uint8_t dataType = 0; // 0 = temperature, 1 = humidity, 2 = CO2
     uint16_t maxPoints = Constants::DEFAULT_GRAPH_MAX_POINTS;
     
     if (request->hasParam("type")) {
         dataType = request->getParam("type")->value().toInt();
     }
     
     if (request->hasParam("points")) {
         maxPoints = request->getParam("points")->value().toInt();
     }
     
     // Get graph data
     std::vector<std::vector<float>> data = getAppCore()->getSensorManager()->getGraphData(dataType, maxPoints);
     
     // Create JSON response
     DynamicJsonDocument doc(16384);  // Adjust size based on maxPoints
     
     JsonArray upperDhtArray = doc.createNestedArray("upper_dht");
     JsonArray lowerDhtArray = doc.createNestedArray("lower_dht");
     JsonArray scdArray = doc.createNestedArray("scd");
     JsonArray timestampsArray = doc.createNestedArray("timestamps");
     
     // Check if we have data
     if (data.size() >= 4 && !data[0].empty()) {
         // Add data points
         for (size_t i = 0; i < data[0].size(); i++) {
             upperDhtArray.add(data[0][i]);
             lowerDhtArray.add(data[1][i]);
             scdArray.add(data[2][i]);
             timestampsArray.add(data[3][i]);
         }
     }
     
     String response;
     serializeJson(doc, response);
     
     request->send(200, "application/json", response);
 }
 
 void WebServer::handleGetRelayStatus(AsyncWebServerRequest* request) {
     if (!authenticate(request)) {
         return;
     }
     
     // Get relay configurations
     std::vector<RelayConfig> relayConfigs = getAppCore()->getRelayManager()->getAllRelayConfigs();
     
     // Create JSON response
     DynamicJsonDocument doc(2048);
     JsonArray relaysArray = doc.createNestedArray("relays");
     
     for (const auto& config : relayConfigs) {
         JsonObject relayObj = relaysArray.createNestedObject();
         relayObj["id"] = config.relayId;
         relayObj["name"] = config.name;
         relayObj["pin"] = config.pin;
         relayObj["visible"] = config.visible;
         relayObj["is_on"] = config.isOn;
         relayObj["state"] = static_cast<int>(config.state);
         relayObj["last_trigger"] = static_cast<int>(config.lastTrigger);
         
         if (config.hasDependency) {
             relayObj["depends_on"] = config.dependsOnRelay;
         }
         
         JsonObject timeObj = relayObj.createNestedObject("operating_time");
         timeObj["start_hour"] = config.operatingTime.startHour;
         timeObj["start_minute"] = config.operatingTime.startMinute;
         timeObj["end_hour"] = config.operatingTime.endHour;
         timeObj["end_minute"] = config.operatingTime.endMinute;
     }
     
     // Get cycle configuration
     uint16_t onDuration, interval;
     getAppCore()->getRelayManager()->getCycleConfig(onDuration, interval);
     
     JsonObject cycleObj = doc.createNestedObject("cycle_config");
     cycleObj["on_duration"] = onDuration;
     cycleObj["interval"] = interval;
     
     // Get override duration
     doc["override_duration"] = getAppCore()->getRelayManager()->getOverrideDuration();
     
     String response;
     serializeJson(doc, response);
     
     request->send(200, "application/json", response);
 }
 
 void WebServer::handleSetRelayState(AsyncWebServerRequest* request, JsonVariant& json) {
     if (!authenticate(request)) {
         return;
     }
     
     // Parse JSON request
     JsonObject jsonObj = json.as<JsonObject>();
     
     if (!jsonObj.containsKey("relay_id") || !jsonObj.containsKey("state")) {
         request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing threshold parameters\"}");
         return;
     }
     
     // Update environmental thresholds
     bool success = getAppCore()->getRelayManager()->setEnvironmentalThresholds(
         jsonObj["humidity_low"].as<float>(),
         jsonObj["humidity_high"].as<float>(),
         jsonObj["temperature_low"].as<float>(),
         jsonObj["temperature_high"].as<float>(),
         jsonObj["co2_low"].as<float>(),
         jsonObj["co2_high"].as<float>()
     );
     
     // Save settings
     if (success) {
         getAppCore()->getStorageManager()->saveSettings();
     }
     
     // Return result
     String response = "{\"success\":" + String(success ? "true" : "false") + 
                       ",\"message\":\"" + (success ? "Environmental thresholds updated" : "Failed to update thresholds") + "\"}";
     
     request->send(200, "application/json", response);
 }
 
 void WebServer::handleGetSystemInfo(AsyncWebServerRequest* request) {
     if (!authenticate(request)) {
         return;
     }
     
     // Create JSON response
     DynamicJsonDocument doc(2048);
     
     // System information
     doc["app_name"] = Constants::APP_NAME;
     doc["app_version"] = Constants::APP_VERSION;
     doc["fs_version"] = Constants::FS_VERSION;
     
     // Network information
     JsonObject networkObj = doc.createNestedObject("network");
     networkObj["hostname"] = getAppCore()->getNetworkManager()->getHostname();
     networkObj["ip_address"] = getAppCore()->getNetworkManager()->getIPAddress();
     networkObj["ssid"] = getAppCore()->getNetworkManager()->getConnectedSSID();
     networkObj["rssi"] = getAppCore()->getNetworkManager()->getRSSI();
     networkObj["mac_address"] = WiFi.macAddress();
     
     // Memory information
     JsonObject memoryObj = doc.createNestedObject("memory");
     memoryObj["heap_size"] = ESP.getHeapSize();
     memoryObj["free_heap"] = ESP.getFreeHeap();
     memoryObj["min_free_heap"] = ESP.getMinFreeHeap();
     memoryObj["max_alloc_heap"] = ESP.getMaxAllocHeap();
     
     // SPIFFS information
     JsonObject spiffsObj = doc.createNestedObject("spiffs");
     spiffsObj["total_bytes"] = SPIFFS.totalBytes();
     spiffsObj["used_bytes"] = SPIFFS.usedBytes();
     
     // CPU information
     JsonObject cpuObj = doc.createNestedObject("cpu");
     cpuObj["chip_model"] = ESP.getChipModel();
     cpuObj["chip_revision"] = ESP.getChipRevision();
     cpuObj["cpu_freq_mhz"] = ESP.getCpuFreqMHz();
     cpuObj["cycle_count"] = ESP.getCycleCount();
     cpuObj["sdk_version"] = ESP.getSdkVersion();
     
     // Uptime
     doc["uptime_seconds"] = millis() / 1000;
     
     String response;
     serializeJson(doc, response);
     
     request->send(200, "application/json", response);
 }
 
 void WebServer::handleGetFilesList(AsyncWebServerRequest* request) {
     if (!authenticate(request)) {
         return;
     }
     
     // Create JSON response
     DynamicJsonDocument doc(16384);  // Adjust size based on expected number of files
     JsonArray filesArray = doc.createNestedArray("files");
     
     // Open the file system directory
     File root = SPIFFS.open("/");
     if (!root || !root.isDirectory()) {
         request->send(500, "application/json", "{\"success\":false,\"message\":\"Failed to open SPIFFS\"}");
         return;
     }
     
     // Iterate through all files
     File file = root.openNextFile();
     while (file) {
         if (!file.isDirectory()) {
             JsonObject fileObj = filesArray.createNestedObject();
             fileObj["name"] = String(file.name());
             fileObj["size"] = file.size();
             fileObj["url"] = String(file.name());
         }
         file = root.openNextFile();
     }
     
     // Add file system stats
     doc["total_bytes"] = SPIFFS.totalBytes();
     doc["used_bytes"] = SPIFFS.usedBytes();
     doc["free_bytes"] = SPIFFS.totalBytes() - SPIFFS.usedBytes();
     
     String response;
     serializeJson(doc, response);
     
     request->send(200, "application/json", response);
 }
 
 void WebServer::handleFileDelete(AsyncWebServerRequest* request) {
     if (!authenticate(request)) {
         return;
     }
     
     // Check if path parameter exists
     if (!request->hasParam("path")) {
         request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing file path\"}");
         return;
     }
     
     String path = request->getParam("path")->value();
     
     // Basic security check
     if (path.isEmpty() || path == "/" || !path.startsWith("/")) {
         request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid file path\"}");
         return;
     }
     
     // Try to delete the file
     if (SPIFFS.exists(path)) {
         if (SPIFFS.remove(path)) {
             getAppCore()->getLogManager()->log(LogLevel::INFO, "WebServer", "File deleted: " + path);
             request->send(200, "application/json", "{\"success\":true,\"message\":\"File deleted\"}");
         } else {
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "WebServer", "Failed to delete file: " + path);
             request->send(500, "application/json", "{\"success\":false,\"message\":\"Failed to delete file\"}");
         }
     } else {
         request->send(404, "application/json", "{\"success\":false,\"message\":\"File not found\"}");
     }
 }
 
 void WebServer::handleFileUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
     if (!authenticate(request)) {
         return;
     }
     
     // First chunk
     if (index == 0) {
         // Make sure path starts with /
         if (!filename.startsWith("/")) {
             filename = "/" + filename;
         }
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "WebServer", "Upload started: " + filename);
         
         // Open the file
         if (SPIFFS.exists(filename)) {
             SPIFFS.remove(filename);
         }
         
         request->_tempFile = SPIFFS.open(filename, FILE_WRITE);
         if (!request->_tempFile) {
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "WebServer", "Failed to open file for writing: " + filename);
             return;
         }
     }
     
     // Write chunk
     if (request->_tempFile) {
         if (len) {
             request->_tempFile.write(data, len);
         }
         
         // Last chunk
         if (final) {
             request->_tempFile.close();
             getAppCore()->getLogManager()->log(LogLevel::INFO, "WebServer", 
                 "Upload finished: " + filename + ", size: " + String(index + len));
         }
     }
 }
 
 void WebServer::handleReboot(AsyncWebServerRequest* request) {
     if (!authenticate(request)) {
         return;
     }
     
     // Schedule reboot
     request->send(200, "application/json", "{\"success\":true,\"message\":\"Device will reboot now\"}");
     
     // Give time for the response to be sent
     vTaskDelay(pdMS_TO_TICKS(1000));
     getAppCore()->reboot();
 }
 
 void WebServer::handleFactoryReset(AsyncWebServerRequest* request) {
     if (!authenticate(request)) {
         return;
     }
     
     // Schedule factory reset
     request->send(200, "application/json", "{\"success\":true,\"message\":\"Device will perform factory reset and reboot\"}");
     
     // Give time for the response to be sent
     vTaskDelay(pdMS_TO_TICKS(1000));
     getAppCore()->factoryReset();
 }
 
 void WebServer::handleGetProfiles(AsyncWebServerRequest* request) {
     if (!authenticate(request)) {
         return;
     }
     
     // Forward the request to the profile manager
     String profiles = getAppCore()->getProfileManager()->getProfilesJson();
     
     request->send(200, "application/json", profiles);
 }
 
 void WebServer::handleSaveProfile(AsyncWebServerRequest* request, JsonVariant& json) {
     if (!authenticate(request)) {
         return;
     }
     
     // Parse JSON request
     JsonObject jsonObj = json.as<JsonObject>();
     
     if (!jsonObj.containsKey("name") || !jsonObj.containsKey("settings")) {
         request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing profile name or settings\"}");
         return;
     }
     
     String name = jsonObj["name"].as<String>();
     JsonObject settings = jsonObj["settings"].as<JsonObject>();
     
     // Save the profile
     bool success = getAppCore()->getProfileManager()->saveProfile(name, settings);
     
     // Return result
     String response = "{\"success\":" + String(success ? "true" : "false") + 
                       ",\"message\":\"" + (success ? "Profile saved" : "Failed to save profile") + "\"}";
     
     request->send(200, "application/json", response);
 }
 
 void WebServer::handleLoadProfile(AsyncWebServerRequest* request, JsonVariant& json) {
     if (!authenticate(request)) {
         return;
     }
     
     // Parse JSON request
     JsonObject jsonObj = json.as<JsonObject>();
     
     if (!jsonObj.containsKey("name")) {
         request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing profile name\"}");
         return;
     }
     
     String name = jsonObj["name"].as<String>();
     
     // Load the profile
     bool success = getAppCore()->getProfileManager()->loadProfile(name);
     
     // Return result
     String response = "{\"success\":" + String(success ? "true" : "false") + 
                       ",\"message\":\"" + (success ? "Profile loaded" : "Failed to load profile") + "\"}";
     
     request->send(200, "application/json", response);
 }
 
 void WebServer::handleExportProfiles(AsyncWebServerRequest* request) {
     if (!authenticate(request)) {
         return;
     }
     
     // Get all profiles
     String profiles = getAppCore()->getProfileManager()->getProfilesJson();
     
     // Set headers for file download
     AsyncWebServerResponse* response = request->beginResponse(200, "application/json", profiles);
     response->addHeader("Content-Disposition", "attachment; filename=\"profiles.json\"");
     request->send(response);
 }
 
 void WebServer::handleImportProfiles(AsyncWebServerRequest* request, JsonVariant& json) {
     if (!authenticate(request)) {
         return;
     }
     
     // Import profiles from JSON
     bool success = getAppCore()->getProfileManager()->importProfilesJson(json);
     
     // Return result
     String response = "{\"success\":" + String(success ? "true" : "false") + 
                       ",\"message\":\"" + (success ? "Profiles imported" : "Failed to import profiles") + "\"}";
     
     request->send(200, "application/json", response);
 }
 
 String WebServer::getContentType(const String& filename) {
     if (filename.endsWith(".html")) return "text/html";
     else if (filename.endsWith(".css")) return "text/css";
     else if (filename.endsWith(".js")) return "application/javascript";
     else if (filename.endsWith(".json")) return "application/json";
     else if (filename.endsWith(".ico")) return "image/x-icon";
     else if (filename.endsWith(".jpg")) return "image/jpeg";
     else if (filename.endsWith(".png")) return "image/png";
     else if (filename.endsWith(".svg")) return "image/svg+xml";
     else if (filename.endsWith(".xml")) return "text/xml";
     else if (filename.endsWith(".pdf")) return "application/pdf";
     else if (filename.endsWith(".zip")) return "application/zip";
     else if (filename.endsWith(".gz")) return "application/gzip";
     return "text/plain";
 }
 
 void WebServer::loadDefaultConfigFile() {
     // This method would load the default configuration file from SPIFFS
     // and apply its settings to the system
     
     if (SPIFFS.exists(Constants::DEFAULT_CONFIG_FILE)) {
         File file = SPIFFS.open(Constants::DEFAULT_CONFIG_FILE, FILE_READ);
         if (!file) {
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "WebServer", 
                 "Failed to open default config file: " + String(Constants::DEFAULT_CONFIG_FILE));
             return;
         }
         
         // Read file content
         size_t size = file.size();
         std::unique_ptr<char[]> buf(new char[size + 1]);
         file.readBytes(buf.get(), size);
         buf[size] = '\0';
         file.close();
         
         // Parse JSON
         DynamicJsonDocument doc(8192);
         DeserializationError error = deserializeJson(doc, buf.get());
         
         if (error) {
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "WebServer", 
                 "Failed to parse default config file: " + String(error.c_str()));
             return;
         }
         
         // Apply settings from the config file
         // This would be implemented based on the expected config file structure
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "WebServer", 
             "Default configuration loaded from: " + String(Constants::DEFAULT_CONFIG_FILE));
     } else {
         getAppCore()->getLogManager()->log(LogLevel::WARN, "WebServer", 
             "Default config file not found: " + String(Constants::DEFAULT_CONFIG_FILE));
     }
     request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing relay_id or state\"}");
         return;
     }
     
     uint8_t relayId = jsonObj["relay_id"].as<uint8_t>();
     int stateValue = jsonObj["state"].as<int>();
     
     if (relayId < 1 || relayId > 8 || stateValue < 0 || stateValue > 2) {
         request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid relay_id or state\"}");
         return;
     }
     
     // Set relay state
     RelayState state = static_cast<RelayState>(stateValue);
     bool success = getAppCore()->getRelayManager()->setRelayState(relayId, state);
     
     // Return result
     String response = "{\"success\":" + String(success ? "true" : "false") + 
                       ",\"message\":\"" + (success ? "Relay state updated" : "Failed to update relay state") + "\"}";
     
     request->send(200, "application/json", response);
 }
 
 void WebServer::handleGetSettings(AsyncWebServerRequest* request) {
     if (!authenticate(request)) {
         return;
     }
     
     // Create JSON response
     DynamicJsonDocument doc(2048);
     
     // Sensor settings
     uint8_t dht1Pin, dht2Pin, scdSdaPin, scdSclPin;
     getAppCore()->getSensorManager()->getSensorPins(dht1Pin, dht2Pin, scdSdaPin, scdSclPin);
     
     uint32_t dhtInterval, scdInterval;
     getAppCore()->getSensorManager()->getSensorIntervals(dhtInterval, scdInterval);
     
     JsonObject sensorObj = doc.createNestedObject("sensors");
     sensorObj["upper_dht_pin"] = dht1Pin;
     sensorObj["lower_dht_pin"] = dht2Pin;
     sensorObj["scd_sda_pin"] = scdSdaPin;
     sensorObj["scd_scl_pin"] = scdSclPin;
     sensorObj["dht_interval"] = dhtInterval / 1000;  // Convert to seconds
     sensorObj["scd_interval"] = scdInterval / 1000;  // Convert to seconds
     
     // Relay pins
     JsonObject relayPinsObj = doc.createNestedObject("relay_pins");
     for (int i = 1; i <= 8; i++) {
         relayPinsObj["relay" + String(i)] = getAppCore()->getRelayManager()->getRelayPin(i);
     }
     
     // Environmental thresholds
     float humidityLow, humidityHigh, temperatureLow, temperatureHigh, co2Low, co2High;
     getAppCore()->getRelayManager()->getEnvironmentalThresholds(
         humidityLow, humidityHigh, temperatureLow, temperatureHigh, co2Low, co2High);
     
     JsonObject thresholdsObj = doc.createNestedObject("thresholds");
     thresholdsObj["humidity_low"] = humidityLow;
     thresholdsObj["humidity_high"] = humidityHigh;
     thresholdsObj["temperature_low"] = temperatureLow;
     thresholdsObj["temperature_high"] = temperatureHigh;
     thresholdsObj["co2_low"] = co2Low;
     thresholdsObj["co2_high"] = co2High;
     
     // Cycle configuration
     uint16_t onDuration, interval;
     getAppCore()->getRelayManager()->getCycleConfig(onDuration, interval);
     
     JsonObject cycleObj = doc.createNestedObject("cycle_config");
     cycleObj["on_duration"] = onDuration;
     cycleObj["interval"] = interval;
     
     // Override duration
     doc["override_duration"] = getAppCore()->getRelayManager()->getOverrideDuration();
     
     // Add logging settings
     // This would come from a LogManager in a real implementation
     JsonObject loggingObj = doc.createNestedObject("logging");
     loggingObj["level"] = 0;  // 0 = DEBUG, 1 = INFO, 2 = WARN, 3 = ERROR
     loggingObj["max_size"] = Constants::MAX_LOG_FILE_SIZE / 1024;  // Convert to KB
     loggingObj["log_server"] = "";  // Log server address
     loggingObj["flush_interval"] = 60;  // Seconds
     
     // Add reboot scheduler
     JsonObject rebootObj = doc.createNestedObject("reboot_scheduler");
     rebootObj["enabled"] = false;
     rebootObj["day_of_week"] = 0;  // 0 = Sunday
     rebootObj["hour"] = 3;
     rebootObj["minute"] = 0;
     
     String response;
     serializeJson(doc, response);
     
     request->send(200, "application/json", response);
 }
 
 void WebServer::handleUpdateSettings(AsyncWebServerRequest* request, JsonVariant& json) {
     if (!authenticate(request)) {
         return;
     }
     
     // Parse JSON request
     JsonObject jsonObj = json.as<JsonObject>();
     
     bool settingsUpdated = false;
     
     // Update sensor settings
     if (jsonObj.containsKey("sensors")) {
         JsonObject sensorObj = jsonObj["sensors"];
         
         if (sensorObj.containsKey("upper_dht_pin") && sensorObj.containsKey("lower_dht_pin") && 
             sensorObj.containsKey("scd_sda_pin") && sensorObj.containsKey("scd_scl_pin")) {
             
             getAppCore()->getSensorManager()->setSensorPins(
                 sensorObj["upper_dht_pin"].as<uint8_t>(),
                 sensorObj["lower_dht_pin"].as<uint8_t>(),
                 sensorObj["scd_sda_pin"].as<uint8_t>(),
                 sensorObj["scd_scl_pin"].as<uint8_t>()
             );
             
             settingsUpdated = true;
         }
         
         if (sensorObj.containsKey("dht_interval") && sensorObj.containsKey("scd_interval")) {
             getAppCore()->getSensorManager()->setSensorIntervals(
                 sensorObj["dht_interval"].as<uint32_t>() * 1000,  // Convert to milliseconds
                 sensorObj["scd_interval"].as<uint32_t>() * 1000
             );
             
             settingsUpdated = true;
         }
     }
     
     // Update relay pins
     if (jsonObj.containsKey("relay_pins")) {
         JsonObject relayPinsObj = jsonObj["relay_pins"];
         
         for (int i = 1; i <= 8; i++) {
             String pinKey = "relay" + String(i);
             if (relayPinsObj.containsKey(pinKey)) {
                 getAppCore()->getRelayManager()->setRelayPin(i, relayPinsObj[pinKey].as<uint8_t>());
                 settingsUpdated = true;
             }
         }
     }
     
     // Update environmental thresholds
     if (jsonObj.containsKey("thresholds")) {
         JsonObject thresholdsObj = jsonObj["thresholds"];
         
         if (thresholdsObj.containsKey("humidity_low") && thresholdsObj.containsKey("humidity_high") && 
             thresholdsObj.containsKey("temperature_low") && thresholdsObj.containsKey("temperature_high") && 
             thresholdsObj.containsKey("co2_low") && thresholdsObj.containsKey("co2_high")) {
             
             getAppCore()->getRelayManager()->setEnvironmentalThresholds(
                 thresholdsObj["humidity_low"].as<float>(),
                 thresholdsObj["humidity_high"].as<float>(),
                 thresholdsObj["temperature_low"].as<float>(),
                 thresholdsObj["temperature_high"].as<float>(),
                 thresholdsObj["co2_low"].as<float>(),
                 thresholdsObj["co2_high"].as<float>()
             );
             
             settingsUpdated = true;
         }
     }
     
     // Update cycle configuration
     if (jsonObj.containsKey("cycle_config")) {
         JsonObject cycleObj = jsonObj["cycle_config"];
         
         if (cycleObj.containsKey("on_duration") && cycleObj.containsKey("interval")) {
             getAppCore()->getRelayManager()->setCycleConfig(
                 cycleObj["on_duration"].as<uint16_t>(),
                 cycleObj["interval"].as<uint16_t>()
             );
             
             settingsUpdated = true;
         }
     }
     
     // Update override duration
     if (jsonObj.containsKey("override_duration")) {
         getAppCore()->getRelayManager()->setOverrideDuration(
             jsonObj["override_duration"].as<uint16_t>()
         );
         
         settingsUpdated = true;
     }
     
     // Update logging settings
     if (jsonObj.containsKey("logging")) {
         // This would be handled by a LogManager in a real implementation
         settingsUpdated = true;
     }
     
     // Update reboot scheduler
     if (jsonObj.containsKey("reboot_scheduler")) {
         // This would be handled by a MaintenanceManager in a real implementation
         settingsUpdated = true;
     }
     
     // Save settings if anything was updated
     if (settingsUpdated) {
         getAppCore()->getStorageManager()->saveSettings();
     }
     
     // Return result
     String response = "{\"success\":" + String(settingsUpdated ? "true" : "false") + 
                       ",\"message\":\"" + (settingsUpdated ? "Settings updated" : "No settings were updated") + "\"}";
     
     request->send(200, "application/json", response);
 }
 
 void WebServer::handleGetNetworkConfig(AsyncWebServerRequest* request) {
     if (!authenticate(request)) {
         return;
     }
     
     // Create JSON response
     DynamicJsonDocument doc(2048);
     
     // WiFi credentials (we don't send the passwords for security)
     JsonObject wifiObj = doc.createNestedObject("wifi");
     
     String ssid1, ssid2, ssid3, dummy;
     getAppCore()->getNetworkManager()->getWiFiCredentials(0, ssid1, dummy);
     getAppCore()->getNetworkManager()->getWiFiCredentials(1, ssid2, dummy);
     getAppCore()->getNetworkManager()->getWiFiCredentials(2, ssid3, dummy);
     
     wifiObj["ssid1"] = ssid1;
     wifiObj["ssid2"] = ssid2;
     wifiObj["ssid3"] = ssid3;
     
     // Hostname
     wifiObj["hostname"] = getAppCore()->getNetworkManager()->getHostname();
     
     // Current connection info
     wifiObj["current_ssid"] = getAppCore()->getNetworkManager()->getConnectedSSID();
     wifiObj["ip_address"] = getAppCore()->getNetworkManager()->getIPAddress();
     wifiObj["rssi"] = getAppCore()->getNetworkManager()->getRSSI();
     
     // IP configuration
     bool useDHCP;
     String ip, gateway, subnet, dns1, dns2;
     getAppCore()->getNetworkManager()->getIPConfig(useDHCP, ip, gateway, subnet, dns1, dns2);
     
     JsonObject ipObj = wifiObj.createNestedObject("ip_config");
     ipObj["dhcp"] = useDHCP;
     ipObj["ip"] = ip;
     ipObj["gateway"] = gateway;
     ipObj["subnet"] = subnet;
     ipObj["dns1"] = dns1;
     ipObj["dns2"] = dns2;
     
     // WiFi watchdog settings
     JsonObject watchdogObj = wifiObj.createNestedObject("watchdog");
     watchdogObj["min_rssi"] = getAppCore()->getNetworkManager()->getMinRSSI();
     watchdogObj["check_interval"] = 30;  // Seconds
     
     // MQTT settings (placeholder)
     JsonObject mqttObj = doc.createNestedObject("mqtt");
     mqttObj["enabled"] = false;
     mqttObj["broker"] = Constants::DEFAULT_MQTT_BROKER;
     mqttObj["port"] = Constants::DEFAULT_MQTT_PORT;
     mqttObj["topic"] = Constants::DEFAULT_MQTT_TOPIC;
     mqttObj["username"] = "";
     mqttObj["password"] = "";
     
     String response;
     serializeJson(doc, response);
     
     request->send(200, "application/json", response);
 }
 
 void WebServer::handleUpdateNetworkConfig(AsyncWebServerRequest* request, JsonVariant& json) {
     if (!authenticate(request)) {
         return;
     }
     
     // Parse JSON request
     JsonObject jsonObj = json.as<JsonObject>();
     
     bool configUpdated = false;
     
     // Update WiFi settings
     if (jsonObj.containsKey("wifi")) {
         JsonObject wifiObj = jsonObj["wifi"];
         
         // Update credentials if provided
         if (wifiObj.containsKey("ssid1") && wifiObj.containsKey("password1")) {
             getAppCore()->getNetworkManager()->setWiFiCredentials(0, 
                 wifiObj["ssid1"].as<String>(), 
                 wifiObj["password1"].as<String>());
             configUpdated = true;
         }
         
         if (wifiObj.containsKey("ssid2") && wifiObj.containsKey("password2")) {
             getAppCore()->getNetworkManager()->setWiFiCredentials(1, 
                 wifiObj["ssid2"].as<String>(), 
                 wifiObj["password2"].as<String>());
             configUpdated = true;
         }
         
         if (wifiObj.containsKey("ssid3") && wifiObj.containsKey("password3")) {
             getAppCore()->getNetworkManager()->setWiFiCredentials(2, 
                 wifiObj["ssid3"].as<String>(), 
                 wifiObj["password3"].as<String>());
             configUpdated = true;
         }
         
         // Update hostname
         if (wifiObj.containsKey("hostname")) {
             getAppCore()->getNetworkManager()->setHostname(wifiObj["hostname"].as<String>());
             configUpdated = true;
         }
         
         // Update IP configuration
         if (wifiObj.containsKey("ip_config")) {
             JsonObject ipObj = wifiObj["ip_config"];
             
             if (ipObj.containsKey("dhcp")) {
                 bool useDHCP = ipObj["dhcp"].as<bool>();
                 
                 if (useDHCP) {
                     getAppCore()->getNetworkManager()->setIPConfig(true);
                     configUpdated = true;
                 } else if (ipObj.containsKey("ip") && ipObj.containsKey("gateway") && 
                           ipObj.containsKey("subnet") && ipObj.containsKey("dns1")) {
                     
                     getAppCore()->getNetworkManager()->setIPConfig(false,
                         ipObj["ip"].as<String>(),
                         ipObj["gateway"].as<String>(),
                         ipObj["subnet"].as<String>(),
                         ipObj["dns1"].as<String>(),
                         ipObj.containsKey("dns2") ? ipObj["dns2"].as<String>() : ""
                     );
                     configUpdated = true;
                 }
             }
         }
         
         // Update WiFi watchdog settings
         if (wifiObj.containsKey("watchdog")) {
             JsonObject watchdogObj = wifiObj["watchdog"];
             
             if (watchdogObj.containsKey("min_rssi")) {
                 getAppCore()->getNetworkManager()->setMinRSSI(watchdogObj["min_rssi"].as<int32_t>());
                 configUpdated = true;
             }
             
             if (watchdogObj.containsKey("check_interval")) {
                 getAppCore()->getNetworkManager()->setWiFiCheckInterval(
                     watchdogObj["check_interval"].as<uint32_t>() * 1000  // Convert to milliseconds
                 );
                 configUpdated = true;
             }
         }
     }
     
     // Update MQTT settings
     if (jsonObj.containsKey("mqtt")) {
         // This would be handled by an MQTT manager in a real implementation
         configUpdated = true;
     }
     
     // Save settings if anything was updated
     if (configUpdated) {
         getAppCore()->getStorageManager()->saveSettings();
     }
     
     // Return result
     bool needsReboot = configUpdated;
     String response = "{\"success\":" + String(configUpdated ? "true" : "false") + 
                       ",\"needs_reboot\":" + String(needsReboot ? "true" : "false") + 
                       ",\"message\":\"" + (configUpdated ? "Network configuration updated" : "No settings were updated") + "\"}";
     
     request->send(200, "application/json", response);
     
     // Reboot if needed
     if (needsReboot) {
         vTaskDelay(pdMS_TO_TICKS(1000));  // Give time for the response to be sent
         getAppCore()->reboot();
     }
 }
 
 void WebServer::handleGetEnvironmentalThresholds(AsyncWebServerRequest* request) {
     if (!authenticate(request)) {
         return;
     }
     
     // Get environmental thresholds
     float humidityLow, humidityHigh, temperatureLow, temperatureHigh, co2Low, co2High;
     getAppCore()->getRelayManager()->getEnvironmentalThresholds(
         humidityLow, humidityHigh, temperatureLow, temperatureHigh, co2Low, co2High);
     
     // Create JSON response
     DynamicJsonDocument doc(512);
     
     doc["humidity_low"] = humidityLow;
     doc["humidity_high"] = humidityHigh;
     doc["temperature_low"] = temperatureLow;
     doc["temperature_high"] = temperatureHigh;
     doc["co2_low"] = co2Low;
     doc["co2_high"] = co2High;
     
     String response;
     serializeJson(doc, response);
     
     request->send(200, "application/json", response);
 }
 
 void WebServer::handleUpdateEnvironmentalThresholds(AsyncWebServerRequest* request, JsonVariant& json) {
     if (!authenticate(request)) {
         return;
     }
     
     // Parse JSON request
     JsonObject jsonObj = json.as<JsonObject>();
     
     if (!jsonObj.containsKey("humidity_low") || !jsonObj.containsKey("humidity_high") || 
         !jsonObj.containsKey("temperature_low") || !jsonObj.containsKey("temperature_high") || 
         !jsonObj.containsKey("co2_low") || !jsonObj.containsKey("co2_high")) {
         
            request->send(400, "application/json", "{\"error\":\"Bad Request\"}");
  * @file WebServer.cpp
  * @brief Implementation of the WebServer class
  */
 
 #include "WebServer.h"
 #include "../core/AppCore.h"
 #include "../system/LogManager.h"
 #include "../system/StorageManager.h"
 #include "../network/NetworkManager.h"
 #include "../components/SensorManager.h"
 #include "../components/RelayManager.h"
 #include "../system/ProfileManager.h"
 #include "../ota/OTAManager.h"
 
 WebServer::WebServer() :
     _server(nullptr),
     _port(Constants::DEFAULT_WEB_SERVER_PORT),
     _username(Constants::DEFAULT_HTTP_USERNAME),
     _password(Constants::DEFAULT_HTTP_PASSWORD),
     _isRunning(false),
     _isInConfigMode(false),
     _webServerMutex(nullptr),
     _webServerTaskHandle(nullptr)
 {
 }
 
 WebServer::~WebServer() {
     // Clean up RTOS resources
     if (_webServerMutex != nullptr) {
         vSemaphoreDelete(_webServerMutex);
     }
     
     if (_webServerTaskHandle != nullptr) {
         vTaskDelete(_webServerTaskHandle);
     }
     
     // Clean up server
     if (_server != nullptr) {
         delete _server;
     }
 }
 
 bool WebServer::begin() {
     // Create mutex for thread-safe operations
     _webServerMutex = xSemaphoreCreateMutex();
     if (_webServerMutex == nullptr) {
         Serial.println("Failed to create web server mutex!");
         return false;
     }
     
     // Load authentication credentials from NVS
     nvs_handle_t nvsHandle;
     esp_err_t err = nvs_open(Constants::NVS_CONFIG_NAMESPACE, NVS_READONLY, &nvsHandle);
     if (err == ESP_OK) {
         size_t userLen = 0;
         err = nvs_get_str(nvsHandle, Constants::NVS_HTTP_USER_KEY, nullptr, &userLen);
         if (err == ESP_OK && userLen > 0) {
             char* userBuffer = new char[userLen];
             nvs_get_str(nvsHandle, Constants::NVS_HTTP_USER_KEY, userBuffer, &userLen);
             _username = String(userBuffer);
             delete[] userBuffer;
         }
         
         size_t passLen = 0;
         err = nvs_get_str(nvsHandle, Constants::NVS_HTTP_PASS_KEY, nullptr, &passLen);
         if (err == ESP_OK && passLen > 0) {
             char* passBuffer = new char[passLen];
             nvs_get_str(nvsHandle, Constants::NVS_HTTP_PASS_KEY, passBuffer, &passLen);
             _password = String(passBuffer);
             delete[] passBuffer;
         }
         
         nvs_close(nvsHandle);
     }
     
     return true;
 }
 
 bool WebServer::startConfigurationMode() {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_webServerMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Create server if it doesn't exist
         if (_server == nullptr) {
             _server = new AsyncWebServer(_port);
         } else if (_isRunning) {
             // Stop the server if it's already running
             getAppCore()->getLogManager()->log(LogLevel::INFO, "WebServer", "Stopping existing web server");
             _server->reset();
         }
         
         // Set up routes specific to configuration mode
         setupConfigModeRoutes();
         
         // Set up common routes
         setupCommonRoutes();
         
         // Start the server
         _server->begin();
         _isRunning = true;
         _isInConfigMode = true;
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "WebServer", 
             "Web server started in configuration mode on port " + String(_port));
         
         // Release mutex
         xSemaphoreGive(_webServerMutex);
         return true;
     }
     
     return false;
 }
 
 bool WebServer::startNormalMode() {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_webServerMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Create server if it doesn't exist
         if (_server == nullptr) {
             _server = new AsyncWebServer(_port);
         } else if (_isRunning) {
             // Stop the server if it's already running
             getAppCore()->getLogManager()->log(LogLevel::INFO, "WebServer", "Stopping existing web server");
             _server->reset();
         }
         
         // Set up routes specific to normal mode
         setupNormalModeRoutes();
         
         // Set up common routes
         setupCommonRoutes();
         
         // Start the server
         _server->begin();
         _isRunning = true;
         _isInConfigMode = false;
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "WebServer", 
             "Web server started in normal mode on port " + String(_port));
         
         // Release mutex
         xSemaphoreGive(_webServerMutex);
         return true;
     }
     
     return false;
 }
 
 bool WebServer::setPort(uint16_t port) {
     if (port == 0) {
         return false;
     }
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_webServerMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         _port = port;
         
         // If server is running, restart it
         if (_isRunning) {
             if (_isInConfigMode) {
                 startConfigurationMode();
             } else {
                 startNormalMode();
             }
         }
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "WebServer", 
             "Web server port set to " + String(_port));
         
         // Release mutex
         xSemaphoreGive(_webServerMutex);
         return true;
     }
     
     return false;
 }
 
 uint16_t WebServer::getPort() {
     uint16_t port = Constants::DEFAULT_WEB_SERVER_PORT;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_webServerMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         port = _port;
         
         // Release mutex
         xSemaphoreGive(_webServerMutex);
     }
     
     return port;
 }
 
 bool WebServer::setHttpAuth(const String& username, const String& password) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_webServerMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         _username = username;
         _password = password;
         
         // Save credentials to NVS
         nvs_handle_t nvsHandle;
         esp_err_t err = nvs_open(Constants::NVS_CONFIG_NAMESPACE, NVS_READWRITE, &nvsHandle);
         if (err == ESP_OK) {
             nvs_set_str(nvsHandle, Constants::NVS_HTTP_USER_KEY, username.c_str());
             nvs_set_str(nvsHandle, Constants::NVS_HTTP_PASS_KEY, password.c_str());
             nvs_commit(nvsHandle);
             nvs_close(nvsHandle);
         }
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "WebServer", 
             "HTTP authentication credentials updated");
         
         // Release mutex
         xSemaphoreGive(_webServerMutex);
         return true;
     }
     
     return false;
 }
 
 bool WebServer::getHttpAuth(String& username, String& password) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_webServerMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         username = _username;
         password = _password;
         
         // Release mutex
         xSemaphoreGive(_webServerMutex);
         return true;
     }
     
     return false;
 }
 
 void WebServer::createTasks() {
     // No tasks needed for now since we're using AsyncWebServer,
     // but we could add a monitoring task in the future if needed
 }
 
 void WebServer::setupConfigModeRoutes() {
     // API endpoint for WiFi scanning
     _server->on("/api/wifi/scan", HTTP_GET, std::bind(&WebServer::handleWiFiScan, this, std::placeholders::_1));
     
     // API endpoint for testing WiFi credentials
     _server->addHandler(new AsyncCallbackJsonWebHandler("/api/wifi/test", 
         std::bind(&WebServer::handleTestWiFi, this, std::placeholders::_1, std::placeholders::_2)));
     
     // API endpoint for saving initial configuration
     _server->addHandler(new AsyncCallbackJsonWebHandler("/api/config/save", 
         std::bind(&WebServer::handleSaveSettings, this, std::placeholders::_1, std::placeholders::_2)));
 }
 
 void WebServer::setupNormalModeRoutes() {
     // Require authentication for all endpoints
     _server->on("/api/sensors/data", HTTP_GET, std::bind(&WebServer::handleGetSensorData, this, std::placeholders::_1));
     _server->on("/api/sensors/graph", HTTP_GET, std::bind(&WebServer::handleGetGraphData, this, std::placeholders::_1));
     
     _server->on("/api/relays/status", HTTP_GET, std::bind(&WebServer::handleGetRelayStatus, this, std::placeholders::_1));
     _server->addHandler(new AsyncCallbackJsonWebHandler("/api/relays/set", 
         std::bind(&WebServer::handleSetRelayState, this, std::placeholders::_1, std::placeholders::_2)));
     
     _server->on("/api/settings", HTTP_GET, std::bind(&WebServer::handleGetSettings, this, std::placeholders::_1));
     _server->addHandler(new AsyncCallbackJsonWebHandler("/api/settings/update", 
         std::bind(&WebServer::handleUpdateSettings, this, std::placeholders::_1, std::placeholders::_2)));
     
     _server->on("/api/network/config", HTTP_GET, std::bind(&WebServer::handleGetNetworkConfig, this, std::placeholders::_1));
     _server->addHandler(new AsyncCallbackJsonWebHandler("/api/network/update", 
         std::bind(&WebServer::handleUpdateNetworkConfig, this, std::placeholders::_1, std::placeholders::_2)));
     
     _server->on("/api/environment/thresholds", HTTP_GET, std::bind(&WebServer::handleGetEnvironmentalThresholds, this, std::placeholders::_1));
     _server->addHandler(new AsyncCallbackJsonWebHandler("/api/environment/update", 
         std::bind(&WebServer::handleUpdateEnvironmentalThresholds, this, std::placeholders::_1, std::placeholders::_2)));
     
     _server->on("/api/system/info", HTTP_GET, std::bind(&WebServer::handleGetSystemInfo, this, std::placeholders::_1));
     _server->on("/api/system/files", HTTP_GET, std::bind(&WebServer::handleGetFilesList, this, std::placeholders::_1));
     _server->on("/api/system/delete", HTTP_DELETE, std::bind(&WebServer::handleFileDelete, this, std::placeholders::_1));
     
     _server->on("/api/system/reboot", HTTP_POST, std::bind(&WebServer::handleReboot, this, std::placeholders::_1));
     _server->on("/api/system/factory-reset", HTTP_POST, std::bind(&WebServer::handleFactoryReset, this, std::placeholders::_1));
     
     // File upload handler
     _server->on("/api/upload", HTTP_POST, 
         [](AsyncWebServerRequest* request) { request->send(200); },
         std::bind(&WebServer::handleFileUpload, this, std::placeholders::_1, std::placeholders::_2, 
                   std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
     
     // Profile management
     _server->on("/api/profiles", HTTP_GET, std::bind(&WebServer::handleGetProfiles, this, std::placeholders::_1));
     _server->addHandler(new AsyncCallbackJsonWebHandler("/api/profiles/save", 
         std::bind(&WebServer::handleSaveProfile, this, std::placeholders::_1, std::placeholders::_2)));
     _server->addHandler(new AsyncCallbackJsonWebHandler("/api/profiles/load", 
         std::bind(&WebServer::handleLoadProfile, this, std::placeholders::_1, std::placeholders::_2)));
     _server->on("/api/profiles/export", HTTP_GET, std::bind(&WebServer::handleExportProfiles, this, std::placeholders::_1));
     _server->addHandler(new AsyncCallbackJsonWebHandler("/api/profiles/import", 
         std::bind(&WebServer::handleImportProfiles, this, std::placeholders::_1, std::placeholders::_2)));
 }
 
 void WebServer::setupCommonRoutes() {
     // Serve static files from SPIFFS
     _server->serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
     
     // Handle not found
     _server->onNotFound([](AsyncWebServerRequest* request) {
         request->send(404, "text/plain", "Not found");
     });
     
     // Set up CORS headers if needed
     DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
     DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE");
     DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
 }
 
 bool WebServer::authenticate(AsyncWebServerRequest* request) {
     if (!request->authenticate(_username.c_str(), _password.c_str())) {
         request->requestAuthentication();
         return false;
     }
     return true;
 }
 
 void WebServer::handleWiFiScan(AsyncWebServerRequest* request) {
     // No authentication required for AP mode
     
     // Get network scan results
     std::vector<NetworkInfo> networks = getAppCore()->getNetworkManager()->scanNetworks();
     
     // Create JSON response
     DynamicJsonDocument doc(4096);  // Adjust size based on expected number of networks
     JsonArray networksArray = doc.createNestedArray("networks");
     
     for (const auto& network : networks) {
         JsonObject networkObj = networksArray.createNestedObject();
         networkObj["ssid"] = network.ssid;
         networkObj["rssi"] = network.rssi;
         
         char bssidStr[18];
         sprintf(bssidStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
                 network.bssid[0], network.bssid[1], network.bssid[2], 
                 network.bssid[3], network.bssid[4], network.bssid[5]);
         networkObj["bssid"] = bssidStr;
         
         networkObj["channel"] = network.channel;
         networkObj["encrypted"] = (network.encryptionType != WIFI_AUTH_OPEN);
     }
     
     String response;
     serializeJson(doc, response);
     
     request->send(200, "application/json", response);
 }
 
 void WebServer::handleTestWiFi(AsyncWebServerRequest* request, JsonVariant& json) {
     // No authentication required for AP mode
     
     // Parse JSON request
     JsonObject jsonObj = json.as<JsonObject>();
     
     if (!jsonObj.containsKey("ssid") || !jsonObj.containsKey("password")) {
         request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing SSID or password\"}");
         return;
     }
     
     String ssid = jsonObj["ssid"].as<String>();
     String password = jsonObj["password"].as<String>();
     
     // Test the credentials
     bool success = getAppCore()->getNetworkManager()->testWiFiCredentials(ssid, password);
     
     // Return result
     String response = "{\"success\":" + String(success ? "true" : "false") + 
                       ",\"message\":\"" + (success ? "Connection successful" : "Connection failed") + "\"}";
     
     request->send(200, "application/json", response);
 }
 
 void WebServer::handleSaveSettings(AsyncWebServerRequest* request, JsonVariant& json) {
     // No authentication required for AP mode
     
     // Parse JSON request
     JsonObject jsonObj = json.as<JsonObject>();
     
     // Required fields
     if (!jsonObj.containsKey("wifi") || !jsonObj.containsKey("http_auth") || 
         !jsonObj.containsKey("gpio_config") || !jsonObj.containsKey("update_timings")) {
         request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing required configuration sections\"}");
         return;
     }
     
     // Process WiFi settings
     JsonObject wifiObj = jsonObj["wifi"];
     if (wifiObj.containsKey("ssid1") && wifiObj.containsKey("password1")) {
         getAppCore()->getNetworkManager()->setWiFiCredentials(0, 
             wifiObj["ssid1"].as<String>(), 
             wifiObj["password1"].as<String>());
     }
     
     if (wifiObj.containsKey("ssid2") && wifiObj.containsKey("password2")) {
         getAppCore()->getNetworkManager()->setWiFiCredentials(1, 
             wifiObj["ssid2"].as<String>(), 
             wifiObj["password2"].as<String>());
     }
     
     if (wifiObj.containsKey("ssid3") && wifiObj.containsKey("password3")) {
         getAppCore()->getNetworkManager()->setWiFiCredentials(2, 
             wifiObj["ssid3"].as<String>(), 
             wifiObj["password3"].as<String>());
     }
     
     if (wifiObj.containsKey("hostname")) {
         getAppCore()->getNetworkManager()->setHostname(wifiObj["hostname"].as<String>());
     }
     
     // Process HTTP authentication
     JsonObject authObj = jsonObj["http_auth"];
     if (authObj.containsKey("username") && authObj.contains