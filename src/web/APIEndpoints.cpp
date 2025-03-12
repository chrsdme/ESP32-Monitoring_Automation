/**
 * @file APIEndpoints.cpp
 * @brief Implementation of the APIEndpoints class
 */

 #include "APIEndpoints.h"
 #include "WebServer.h"
 #include "../core/AppCore.h"
 #include "../components/SensorManager.h"
 #include "../components/RelayManager.h"
 #include "../network/NetworkManager.h"
 #include "../system/ProfileManager.h"
 #include "../system/StorageManager.h"
 #include "../system/LogManager.h"
 #include "../system/MaintenanceManager.h"
 #include "../system/PowerManager.h"
 #include "../system/NotificationManager.h"
 #include "../ota/OTAManager.h"
 #include <SPIFFS.h>
 
 APIEndpoints::APIEndpoints(WebServer* webServer) : _webServer(webServer) {
 }
 
 void APIEndpoints::registerEndpoints() {
    // Get the server instance using the correct method name
    AsyncWebServer* server = _webServer->getServer();
    
    // Sensor endpoints
    server->on("/api/sensors/data", HTTP_GET, std::bind(&APIEndpoints::handleGetSensorData, this, std::placeholders::_1));
    server->on("/api/sensors/graph", HTTP_GET, std::bind(&APIEndpoints::handleGetGraphData, this, std::placeholders::_1));
    server->on("/api/sensors/test", HTTP_POST, std::bind(&APIEndpoints::handleTestSensor, this, std::placeholders::_1));
    server->on("/api/sensors/reset", HTTP_POST, std::bind(&APIEndpoints::handleResetSensor, this, std::placeholders::_1));
    
    // Relay endpoints
    server->on("/api/relays/status", HTTP_GET, std::bind(&APIEndpoints::handleGetRelayStatus, this, std::placeholders::_1));
    server->addHandler(new AsyncCallbackJsonWebHandler("/api/relays/set", 
        std::bind(&APIEndpoints::handleSetRelayState, this, std::placeholders::_1, std::placeholders::_2)));
    server->addHandler(new AsyncCallbackJsonWebHandler("/api/relays/operating-time", 
        std::bind(&APIEndpoints::handleSetRelayOperatingTime, this, std::placeholders::_1, std::placeholders::_2)));
    server->addHandler(new AsyncCallbackJsonWebHandler("/api/relays/cycle-config", 
        std::bind(&APIEndpoints::handleSetCycleConfig, this, std::placeholders::_1, std::placeholders::_2)));
    
    // Settings endpoints
    server->on("/api/settings", HTTP_GET, std::bind(&APIEndpoints::handleGetSettings, this, std::placeholders::_1));
    server->addHandler(new AsyncCallbackJsonWebHandler("/api/settings/update", 
        std::bind(&APIEndpoints::handleUpdateSettings, this, std::placeholders::_1, std::placeholders::_2)));
    server->on("/api/environment/thresholds", HTTP_GET, std::bind(&APIEndpoints::handleGetEnvironmentalThresholds, this, std::placeholders::_1));
    server->addHandler(new AsyncCallbackJsonWebHandler("/api/environment/update", 
        std::bind(&APIEndpoints::handleUpdateEnvironmentalThresholds, this, std::placeholders::_1, std::placeholders::_2)));
    
    // Network endpoints
    server->on("/api/network/config", HTTP_GET, std::bind(&APIEndpoints::handleGetNetworkConfig, this, std::placeholders::_1));
    server->addHandler(new AsyncCallbackJsonWebHandler("/api/network/update", 
        std::bind(&APIEndpoints::handleUpdateNetworkConfig, this, std::placeholders::_1, std::placeholders::_2)));
    server->on("/api/wifi/scan", HTTP_GET, std::bind(&APIEndpoints::handleWiFiScan, this, std::placeholders::_1));
    server->addHandler(new AsyncCallbackJsonWebHandler("/api/wifi/test", 
        std::bind(&APIEndpoints::handleTestWiFi, this, std::placeholders::_1, std::placeholders::_2)));
    
    // System endpoints
    server->on("/api/system/info", HTTP_GET, std::bind(&APIEndpoints::handleGetSystemInfo, this, std::placeholders::_1));
    server->on("/api/system/files", HTTP_GET, std::bind(&APIEndpoints::handleGetFilesList, this, std::placeholders::_1));
    server->on("/api/system/delete", HTTP_DELETE, std::bind(&APIEndpoints::handleFileDelete, this, std::placeholders::_1));
    server->on("/api/system/reboot", HTTP_POST, std::bind(&APIEndpoints::handleReboot, this, std::placeholders::_1));
    server->on("/api/system/factory-reset", HTTP_POST, std::bind(&APIEndpoints::handleFactoryReset, this, std::placeholders::_1));
    
    // File upload handler
    server->on("/api/upload", HTTP_POST, 
        [](AsyncWebServerRequest* request) { request->send(200); },
        std::bind(&APIEndpoints::handleFileUpload, this, std::placeholders::_1, std::placeholders::_2, 
                  std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
    
    // Profile endpoints
    server->on("/api/profiles", HTTP_GET, std::bind(&APIEndpoints::handleGetProfiles, this, std::placeholders::_1));
    server->addHandler(new AsyncCallbackJsonWebHandler("/api/profiles/save", 
        std::bind(&APIEndpoints::handleSaveProfile, this, std::placeholders::_1, std::placeholders::_2)));
    server->addHandler(new AsyncCallbackJsonWebHandler("/api/profiles/load", 
        std::bind(&APIEndpoints::handleLoadProfile, this, std::placeholders::_1, std::placeholders::_2)));
    server->on("/api/profiles/export", HTTP_GET, std::bind(&APIEndpoints::handleExportProfiles, this, std::placeholders::_1));
    server->addHandler(new AsyncCallbackJsonWebHandler("/api/profiles/import", 
        std::bind(&APIEndpoints::handleImportProfiles, this, std::placeholders::_1, std::placeholders::_2)));
    
    // OTA update endpoints
    server->on("/api/ota/status", HTTP_GET, std::bind(&APIEndpoints::handleOTAStatus, this, std::placeholders::_1));
    server->on("/api/ota/update", HTTP_POST, std::bind(&APIEndpoints::handleOTAUpdate, this, std::placeholders::_1));
    
    // Maintenance endpoints
    server->on("/api/maintenance/diagnostics", HTTP_GET, std::bind(&APIEndpoints::handleRunDiagnostics, this, std::placeholders::_1));
    server->on("/api/maintenance/health", HTTP_GET, std::bind(&APIEndpoints::handleGetSystemHealth, this, std::placeholders::_1));
    server->addHandler(new AsyncCallbackJsonWebHandler("/api/maintenance/reboot-schedule", 
        std::bind(&APIEndpoints::handleSetRebootSchedule, this, std::placeholders::_1, std::placeholders::_2)));
    server->on("/api/maintenance/reboot-schedule", HTTP_GET, std::bind(&APIEndpoints::handleGetRebootSchedule, this, std::placeholders::_1));
    
    // Power management endpoints
    server->on("/api/power/mode", HTTP_GET, std::bind(&APIEndpoints::handleGetPowerMode, this, std::placeholders::_1));
    server->addHandler(new AsyncCallbackJsonWebHandler("/api/power/mode", 
        std::bind(&APIEndpoints::handleSetPowerMode, this, std::placeholders::_1, std::placeholders::_2)));
    server->addHandler(new AsyncCallbackJsonWebHandler("/api/power/schedule", 
        std::bind(&APIEndpoints::handleSetPowerSchedule, this, std::placeholders::_1, std::placeholders::_2)));
    server->on("/api/power/schedule", HTTP_GET, std::bind(&APIEndpoints::handleGetPowerSchedule, this, std::placeholders::_1));
    
    // Notification endpoints
    server->addHandler(new AsyncCallbackJsonWebHandler("/api/notifications/send", 
        std::bind(&APIEndpoints::handleSendNotification, this, std::placeholders::_1, std::placeholders::_2)));
    server->on("/api/notifications/recent", HTTP_GET, std::bind(&APIEndpoints::handleGetRecentNotifications, this, std::placeholders::_1));
    server->addHandler(new AsyncCallbackJsonWebHandler("/api/notifications/configure", 
        std::bind(&APIEndpoints::handleConfigureNotifications, this, std::placeholders::_1, std::placeholders::_2)));
    server->on("/api/notifications/test", HTTP_POST, std::bind(&APIEndpoints::handleTestNotificationChannel, this, std::placeholders::_1));
    
    // Tapo device endpoints
    server->on("/api/tapo/devices", HTTP_GET, std::bind(&APIEndpoints::handleGetTapoDevices, this, std::placeholders::_1));
    server->addHandler(new AsyncCallbackJsonWebHandler("/api/tapo/add", 
        std::bind(&APIEndpoints::handleAddTapoDevice, this, std::placeholders::_1, std::placeholders::_2)));
    server->on("/api/tapo/remove", HTTP_DELETE, std::bind(&APIEndpoints::handleRemoveTapoDevice, this, std::placeholders::_1));
    server->addHandler(new AsyncCallbackJsonWebHandler("/api/tapo/control", 
        std::bind(&APIEndpoints::handleControlTapoDevice, this, std::placeholders::_1, std::placeholders::_2)));
    
    // Logging endpoints
    server->on("/api/logs", HTTP_GET, std::bind(&APIEndpoints::handleGetLogs, this, std::placeholders::_1));
    server->addHandler(new AsyncCallbackJsonWebHandler("/api/logs/level", 
        std::bind(&APIEndpoints::handleSetLogLevel, this, std::placeholders::_1, std::placeholders::_2)));
    server->on("/api/logs/clear", HTTP_POST, std::bind(&APIEndpoints::handleClearLogs, this, std::placeholders::_1));
}
 
 bool APIEndpoints::authenticate(AsyncWebServerRequest* request) {
     return _webServer->authenticate(request);
 }
 
 void APIEndpoints::handleGetSensorData(AsyncWebServerRequest* request) {
     if (!authenticate(request)) {
         return;
     }
     
     // Get sensor readings
     SensorReading upperDht, lowerDht, scd;
     bool hasReadings = getAppCore()->getSensorManager()->getSensorReadings(upperDht, lowerDht, scd);
     
     // Create JSON response
     DynamicJsonDocument doc(1024);
     
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
 
 void APIEndpoints::handleGetGraphData(AsyncWebServerRequest* request) {
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
 
 void APIEndpoints::handleTestSensor(AsyncWebServerRequest* request) {
     if (!authenticate(request)) {
         return;
     }
     
     // Check for sensor type parameter
     if (!request->hasParam("type")) {
         request->send(400, "application/json", createJsonResponse(false, "Missing sensor type parameter"));
         return;
     }
     
     uint8_t sensorType = request->getParam("type")->value().toInt();
     
     if (sensorType > 2) {
         request->send(400, "application/json", createJsonResponse(false, "Invalid sensor type"));
         return;
     }
     
     // Run the test
     bool success = getAppCore()->getSensorManager()->testSensor(sensorType);
     
     // Create response
     String sensorName;
     switch (sensorType) {
         case 0: sensorName = "Upper DHT22"; break;
         case 1: sensorName = "Lower DHT22"; break;
         case 2: sensorName = "SCD40"; break;
     }
     
     String message = success ? 
         sensorName + " test passed" : 
         sensorName + " test failed";
     
     request->send(200, "application/json", createJsonResponse(success, message));
 }
 
 void APIEndpoints::handleResetSensor(AsyncWebServerRequest* request) {
     if (!authenticate(request)) {
         return;
     }
     
     // Check for sensor type parameter
     if (!request->hasParam("type")) {
         request->send(400, "application/json", createJsonResponse(false, "Missing sensor type parameter"));
         return;
     }
     
     uint8_t sensorType = request->getParam("type")->value().toInt();
     
     if (sensorType > 2) {
         request->send(400, "application/json", createJsonResponse(false, "Invalid sensor type"));
         return;
     }
     
     // Reset the sensor
     bool success = getAppCore()->getSensorManager()->resetSensor(sensorType);
     
     // Create response
     String sensorName;
     switch (sensorType) {
         case 0: sensorName = "Upper DHT22"; break;
         case 1: sensorName = "Lower DHT22"; break;
         case 2: sensorName = "SCD40"; break;
     }
     
     String message = success ? 
         sensorName + " reset successful" : 
         sensorName + " reset failed";
     
     request->send(200, "application/json", createJsonResponse(success, message));
 }
 
 void APIEndpoints::handleGetRelayStatus(AsyncWebServerRequest* request) {
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
 
 void APIEndpoints::handleSetRelayState(AsyncWebServerRequest* request, JsonVariant& json) {
     if (!authenticate(request)) {
         return;
     }
     
     // Parse JSON request
     JsonObject jsonObj = json.as<JsonObject>();
     
     if (!jsonObj.containsKey("relay_id") || !jsonObj.containsKey("state")) {
         request->send(400, "application/json", createJsonResponse(false, "Missing relay_id or state"));
         return;
     }
     
     uint8_t relayId = jsonObj["relay_id"].as<uint8_t>();
     int stateValue = jsonObj["state"].as<int>();
     
     if (relayId < 1 || relayId > 8 || stateValue < 0 || stateValue > 2) {
         request->send(400, "application/json", createJsonResponse(false, "Invalid relay_id or state"));
         return;
     }
     
     // Set relay state
     RelayState state = static_cast<RelayState>(stateValue);
     bool success = getAppCore()->getRelayManager()->setRelayState(relayId, state);
     
     // Return result
     String message = success ? 
         "Relay " + String(relayId) + " state updated to " + String(stateValue) : 
         "Failed to update relay state";
     
     request->send(200, "application/json", createJsonResponse(success, message));
 }
 
 void APIEndpoints::handleSetRelayOperatingTime(AsyncWebServerRequest* request, JsonVariant& json) {
     if (!authenticate(request)) {
         return;
     }
     
     // Parse JSON request
     JsonObject jsonObj = json.as<JsonObject>();
     
     if (!jsonObj.containsKey("relay_id") || 
         !jsonObj.containsKey("start_hour") || !jsonObj.containsKey("start_minute") || 
         !jsonObj.containsKey("end_hour") || !jsonObj.containsKey("end_minute")) {
         
         request->send(400, "application/json", createJsonResponse(false, "Missing required parameters"));
         return;
     }
     
     uint8_t relayId = jsonObj["relay_id"].as<uint8_t>();
     uint8_t startHour = jsonObj["start_hour"].as<uint8_t>();
     uint8_t startMinute = jsonObj["start_minute"].as<uint8_t>();
     uint8_t endHour = jsonObj["end_hour"].as<uint8_t>();
     uint8_t endMinute = jsonObj["end_minute"].as<uint8_t>();
     
     if (relayId < 1 || relayId > 8 || 
         startHour > 23 || startMinute > 59 || 
         endHour > 23 || endMinute > 59) {
         
         request->send(400, "application/json", createJsonResponse(false, "Invalid parameters"));
         return;
     }
     
     // Set operating time
     bool success = getAppCore()->getRelayManager()->setRelayOperatingTime(
         relayId, startHour, startMinute, endHour, endMinute);
     
     // Return result
     String message = success ? 
         "Relay " + String(relayId) + " operating time updated" : 
         "Failed to update relay operating time";
     
     request->send(200, "application/json", createJsonResponse(success, message));
 }
 
 void APIEndpoints::handleSetCycleConfig(AsyncWebServerRequest* request, JsonVariant& json) {
     if (!authenticate(request)) {
         return;
     }
     
     // Parse JSON request
     JsonObject jsonObj = json.as<JsonObject>();
     
     if (!jsonObj.containsKey("on_duration") || !jsonObj.containsKey("interval")) {
         request->send(400, "application/json", createJsonResponse(false, "Missing on_duration or interval"));
         return;
     }
     
     uint16_t onDuration = jsonObj["on_duration"].as<uint16_t>();
     uint16_t interval = jsonObj["interval"].as<uint16_t>();
     
     if (onDuration >= interval) {
         request->send(400, "application/json", createJsonResponse(false, "on_duration must be less than interval"));
         return;
     }
     
     // Set cycle configuration
     bool success = getAppCore()->getRelayManager()->setCycleConfig(onDuration, interval);
     
     // Return result
     String message = success ? 
         "Cycle configuration updated" : 
         "Failed to update cycle configuration";
     
     request->send(200, "application/json", createJsonResponse(success, message));
 }
 
 void APIEndpoints::handleGetSettings(AsyncWebServerRequest* request) {
     if (!authenticate(request)) {
         return;
     }
     
     // Create JSON response
     DynamicJsonDocument doc(4096);
     
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
     JsonObject loggingObj = doc.createNestedObject("logging");
     loggingObj["level"] = static_cast<int>(getAppCore()->getLogManager()->getLogLevel());
     loggingObj["max_size"] = Constants::MAX_LOG_FILE_SIZE / 1024;  // Convert to KB
     
     // Add reboot scheduler settings
     RebootSchedule rebootSchedule = getAppCore()->getMaintenanceManager()->getRebootSchedule();
     JsonObject rebootObj = doc.createNestedObject("reboot_scheduler");
     rebootObj["enabled"] = rebootSchedule.enabled;
     rebootObj["day_of_week"] = rebootSchedule.dayOfWeek;
     rebootObj["hour"] = rebootSchedule.hour;
     rebootObj["minute"] = rebootSchedule.minute;
     
     // Add power settings
     PowerSchedule powerSchedule = getAppCore()->getPowerManager()->getPowerSchedule();
     JsonObject powerObj = doc.createNestedObject("power");
     powerObj["mode"] = static_cast<int>(getAppCore()->getPowerManager()->getCurrentPowerMode());
     powerObj["schedule_enabled"] = powerSchedule.enabled;
     powerObj["schedule_mode"] = static_cast<int>(powerSchedule.mode);
     powerObj["schedule_start_hour"] = powerSchedule.startHour;
     powerObj["schedule_start_minute"] = powerSchedule.startMinute;
     powerObj["schedule_end_hour"] = powerSchedule.endHour;
     powerObj["schedule_end_minute"] = powerSchedule.endMinute;
     
     String response;
     serializeJson(doc, response);
     
     request->send(200, "application/json", response);
 }