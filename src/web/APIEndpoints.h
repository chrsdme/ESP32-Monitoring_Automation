/**
 * @file APIEndpoints.h
 * @brief Defines all API endpoints for the web server
 */

 #ifndef API_ENDPOINTS_H
 #define API_ENDPOINTS_H
 
 #include <Arduino.h>
 // Use our wrapper instead of direct inclusion
 #include "AsyncWebServerWrapper.h"
 #include <AsyncJson.h>
 #include <ArduinoJson.h>
 
 // Forward declarations
 class WebServer;
 class AppCore;
 
 /**
  * @class APIEndpoints
  * @brief Defines and handles all API endpoints for the web server
  */
 class APIEndpoints {
 public:
     /**
      * @brief Constructor
      * @param webServer Pointer to the web server
      */
     APIEndpoints(WebServer* webServer);
     
     /**
      * @brief Register all API endpoints with the web server
      */
     void registerEndpoints();
     
 private:
     // Reference to the web server
     WebServer* _webServer;
     
     // Authentication helper
     bool authenticate(AsyncWebServerRequest* request);
     
     // Sensor endpoints
     void handleGetSensorData(AsyncWebServerRequest* request);
     void handleGetGraphData(AsyncWebServerRequest* request);
     void handleTestSensor(AsyncWebServerRequest* request);
     void handleResetSensor(AsyncWebServerRequest* request);
     
     // Relay endpoints
     void handleGetRelayStatus(AsyncWebServerRequest* request);
     void handleSetRelayState(AsyncWebServerRequest* request, JsonVariant& json);
     void handleSetRelayOperatingTime(AsyncWebServerRequest* request, JsonVariant& json);
     void handleSetCycleConfig(AsyncWebServerRequest* request, JsonVariant& json);
     
     // Settings endpoints
     void handleGetSettings(AsyncWebServerRequest* request);
     void handleUpdateSettings(AsyncWebServerRequest* request, JsonVariant& json);
     void handleGetEnvironmentalThresholds(AsyncWebServerRequest* request);
     void handleUpdateEnvironmentalThresholds(AsyncWebServerRequest* request, JsonVariant& json);
     
     // Network endpoints
     void handleGetNetworkConfig(AsyncWebServerRequest* request);
     void handleUpdateNetworkConfig(AsyncWebServerRequest* request, JsonVariant& json);
     void handleWiFiScan(AsyncWebServerRequest* request);
     void handleTestWiFi(AsyncWebServerRequest* request, JsonVariant& json);
     
     // System endpoints
     void handleGetSystemInfo(AsyncWebServerRequest* request);
     void handleGetFilesList(AsyncWebServerRequest* request);
     void handleFileDelete(AsyncWebServerRequest* request);
     void handleFileUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final);
     void handleReboot(AsyncWebServerRequest* request);
     void handleFactoryReset(AsyncWebServerRequest* request);
     
     // Profile endpoints
     void handleGetProfiles(AsyncWebServerRequest* request);
     void handleSaveProfile(AsyncWebServerRequest* request, JsonVariant& json);
     void handleLoadProfile(AsyncWebServerRequest* request, JsonVariant& json);
     void handleExportProfiles(AsyncWebServerRequest* request);
     void handleImportProfiles(AsyncWebServerRequest* request, JsonVariant& json);
     
     // OTA update endpoints
     void handleOTAUpdate(AsyncWebServerRequest* request);
     void handleOTAStatus(AsyncWebServerRequest* request);
     
     // Maintenance endpoints
     void handleRunDiagnostics(AsyncWebServerRequest* request);
     void handleGetSystemHealth(AsyncWebServerRequest* request);
     void handleSetRebootSchedule(AsyncWebServerRequest* request, JsonVariant& json);
     void handleGetRebootSchedule(AsyncWebServerRequest* request);
     
     // Power management endpoints
     void handleGetPowerMode(AsyncWebServerRequest* request);
     void handleSetPowerMode(AsyncWebServerRequest* request, JsonVariant& json);
     void handleSetPowerSchedule(AsyncWebServerRequest* request, JsonVariant& json);
     void handleGetPowerSchedule(AsyncWebServerRequest* request);
     
     // Notification endpoints
     void handleSendNotification(AsyncWebServerRequest* request, JsonVariant& json);
     void handleGetRecentNotifications(AsyncWebServerRequest* request);
     void handleConfigureNotifications(AsyncWebServerRequest* request, JsonVariant& json);
     void handleTestNotificationChannel(AsyncWebServerRequest* request);
     
     // Tapo device endpoints
     void handleGetTapoDevices(AsyncWebServerRequest* request);
     void handleAddTapoDevice(AsyncWebServerRequest* request, JsonVariant& json);
     void handleRemoveTapoDevice(AsyncWebServerRequest* request);
     void handleControlTapoDevice(AsyncWebServerRequest* request, JsonVariant& json);
     
     // Logging endpoints
     void handleGetLogs(AsyncWebServerRequest* request);
     void handleSetLogLevel(AsyncWebServerRequest* request, JsonVariant& json);
     void handleClearLogs(AsyncWebServerRequest* request);
     
     // Helper methods
     String getContentType(const String& filename);
     String createJsonResponse(bool success, const String& message);
 };
 
 #endif // API_ENDPOINTS_H