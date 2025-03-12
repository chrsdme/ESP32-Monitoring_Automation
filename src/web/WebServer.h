/**
 * @file WebServer.h
 * @brief Manages the web server and API endpoints
 */

 #ifndef WEB_SERVER_H
 #define WEB_SERVER_H
 
 #include <Arduino.h>
 #include <functional>
 #include "ESPAsyncWebServerFix.h" 
 #include "AsyncWebServerWrapper.h"
 #include <AsyncJson.h>
 #include <ArduinoJson.h>
 #include <SPIFFS.h>
 #include <freertos/FreeRTOS.h>
 #include <freertos/task.h>
 #include <freertos/semphr.h>
 #include "../utils/Constants.h"
 
 // Forward declarations
 class AppCore;
 
 /**
  * @class WebServer
  * @brief Manages the web server, file serving, and API endpoints
  */
 class WebServer {
 public:
     WebServer();
     ~WebServer();
     
     /**
      * @brief Initialize the web server
      * @return True if initialized successfully
      */
     bool begin();
     
     /**
      * @brief Start the web server in configuration mode
      * @return True if started successfully
      */
     bool startConfigurationMode();
     
     /**
      * @brief Start the web server in normal mode
      * @return True if started successfully
      */
     bool startNormalMode();
     
     /**
      * @brief Set web server port
      * @param port Port number
      * @return True if port set successfully
      */
     bool setPort(uint16_t port);
     
     /**
      * @brief Get web server port
      * @return Current port number
      */
     uint16_t getPort();
     
     /**
      * @brief Set HTTP authentication credentials
      * @param username Username
      * @param password Password
      * @return True if credentials set successfully
      */
     bool setHttpAuth(const String& username, const String& password);
     
     /**
      * @brief Get HTTP authentication credentials
      * @param username Output parameter for username
      * @param password Output parameter for password
      * @return True if credentials retrieved successfully
      */
     bool getHttpAuth(String& username, String& password);
     
     /**
      * @brief Create RTOS tasks for web server operations
      */
     void createTasks();
     
     /**
      * @brief Get the AsyncWebServer instance
      * @return Pointer to the AsyncWebServer
      */
     AsyncWebServer* getServer() { return _server; }
     
     /**
      * @brief Authenticate a request
      * @param request HTTP request to authenticate
      * @return True if authentication successful
      */
     bool authenticate(AsyncWebServerRequest* request);
     
 private:
     // Web server instance
     AsyncWebServer* _server;
     
     // Configuration
     uint16_t _port;
     String _username;
     String _password;
     
     // Status tracking
     bool _isRunning;
     bool _isInConfigMode;
     
     // RTOS resources
     SemaphoreHandle_t _webServerMutex;
     TaskHandle_t _webServerTaskHandle;
     
     // Private methods
     void setupConfigModeRoutes();
     void setupNormalModeRoutes();
     void setupCommonRoutes();
     
     void handleWiFiScan(AsyncWebServerRequest* request);
     void handleTestWiFi(AsyncWebServerRequest* request, JsonVariant& json);
     void handleSaveSettings(AsyncWebServerRequest* request, JsonVariant& json);
     
     void handleGetSensorData(AsyncWebServerRequest* request);
     void handleGetGraphData(AsyncWebServerRequest* request);
     void handleGetRelayStatus(AsyncWebServerRequest* request);
     void handleSetRelayState(AsyncWebServerRequest* request, JsonVariant& json);
     void handleGetSettings(AsyncWebServerRequest* request);
     void handleUpdateSettings(AsyncWebServerRequest* request, JsonVariant& json);
     void handleGetNetworkConfig(AsyncWebServerRequest* request);
     void handleUpdateNetworkConfig(AsyncWebServerRequest* request, JsonVariant& json);
     void handleGetEnvironmentalThresholds(AsyncWebServerRequest* request);
     void handleUpdateEnvironmentalThresholds(AsyncWebServerRequest* request, JsonVariant& json);
     void handleGetSystemInfo(AsyncWebServerRequest* request);
     void handleGetFilesList(AsyncWebServerRequest* request);
     void handleFileDelete(AsyncWebServerRequest* request);
     void handleFileUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final);
     void handleReboot(AsyncWebServerRequest* request);
     void handleFactoryReset(AsyncWebServerRequest* request);
     
     // Profile management
     void handleGetProfiles(AsyncWebServerRequest* request);
     void handleSaveProfile(AsyncWebServerRequest* request, JsonVariant& json);
     void handleLoadProfile(AsyncWebServerRequest* request, JsonVariant& json);
     void handleExportProfiles(AsyncWebServerRequest* request);
     void handleImportProfiles(AsyncWebServerRequest* request, JsonVariant& json);
     
     // Helper methods
     String getContentType(const String& filename);
     void loadDefaultConfigFile();
 };
 
 #endif // WEB_SERVER_H