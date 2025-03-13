/**
 * @file WebServer.h
 * @brief Manages the web server and API endpoints (non-SSL version)
 */

 #ifndef WEB_SERVER_H
 #define WEB_SERVER_H
 
 #include <Arduino.h>
 #include <functional>
 #include <AsyncJson.h>
 #include <ArduinoJson.h>
 #include <ESPAsyncWebServer.h>
 #include <SPIFFS.h>
 #include <freertos/FreeRTOS.h>
 #include <freertos/task.h>
 #include <freertos/semphr.h>
 #include "../utils/Constants.h"
 
 // Forward declarations
 class AppCore;
 class APIEndpoints;
 
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
     
     // API endpoints manager
     APIEndpoints* _apiEndpoints;
     
     // Configuration
     uint16_t _port;
     String _username;
     String _password;
     
     // Status tracking
     bool _isRunning;
     bool _isInConfigMode;
     
     // RTOS resources
     SemaphoreHandle_t _webServerMutex;
     
     // Private methods
     void setupConfigModeRoutes();
     void setupCommonRoutes();
     
     void handleWiFiScan(AsyncWebServerRequest* request);
     void handleTestWiFi(AsyncWebServerRequest* request, JsonVariant& json);
     void handleSaveSettings(AsyncWebServerRequest* request, JsonVariant& json);
     
     // Helper methods
     String getContentType(const String& filename);
 };
 
 #endif // WEB_SERVER_H