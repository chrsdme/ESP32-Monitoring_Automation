/**
 * @file SecureWebServer.h
 * @brief Secure web server implementation using ESP32 HTTPS Server
 */

 #ifndef SECURE_WEB_SERVER_H
 #define SECURE_WEB_SERVER_H
 
 // Use our wrappers to fix various issues
 #include "IPAddressWrapper.h"
 
 // ESP-IDF headers
 #include <esp_system.h>
 #include <esp_tls.h>
 
 // Arduino headers
 #include <Arduino.h>
 #include <WiFi.h>
 
 // HTTPS Server headers
 #include <HTTPSServer.hpp>
 #include <SSLCert.hpp>
 #include <HTTPRequest.hpp>
 #include <HTTPResponse.hpp>
 #include <ResourceNode.hpp>
 
 // Other required headers
 #include <ArduinoJson.h>
 #include <SPIFFS.h>
 #include <freertos/FreeRTOS.h>
 #include <freertos/task.h>
 #include <freertos/semphr.h>
 #include <nvs_flash.h>
 
 #include "../utils/Constants.h"
 
 using namespace httpsserver;
 
 class SecureWebServer {
 public:
     SecureWebServer();
     ~SecureWebServer();
 
     /**
      * @brief Initialize the secure web server
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
 
 private:
     // Static instance pointer for callback functions
     static SecureWebServer* _instance;
 
     // Server configuration
     SSLCert* _serverCertificate;
     HTTPSServer* _secureServer;
     
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
 
     /**
      * @brief Generate a self-signed SSL certificate
      * @return Pointer to generated SSLCert
      */
     SSLCert* generateSelfSignedCertificate();
 
     /**
      * @brief Setup routes for configuration mode
      */
     void setupConfigModeRoutes();
 
     /**
      * @brief Setup routes for normal operation mode
      */
     void setupNormalModeRoutes();
 
     /**
      * @brief Authenticate a request
      * @param request HTTP request to authenticate
      * @param response HTTP response to send authentication errors
      * @return True if authentication successful
      */
     bool authenticate(HTTPRequest* request, HTTPResponse* response);
 
     /**
      * @brief Decode base64 string (used for Basic Auth)
      * @param input Base64 encoded string
      * @return Decoded string
      */
     String decodeBase64(const String& input);
 
     // Static handler methods for callbacks
     static void handleWiFiScanStatic(HTTPRequest* request, HTTPResponse* response);
     static void handleTestWiFiStatic(HTTPRequest* request, HTTPResponse* response);
     static void handleSaveSettingsStatic(HTTPRequest* request, HTTPResponse* response);
     static void handleGetSensorDataStatic(HTTPRequest* request, HTTPResponse* response);
     
     // Instance handler methods
     void handleWiFiScan(HTTPRequest* request, HTTPResponse* response);
     void handleTestWiFi(HTTPRequest* request, HTTPResponse* response);
     void handleSaveSettings(HTTPRequest* request, HTTPResponse* response);
     void handleGetSensorData(HTTPRequest* request, HTTPResponse* response);
 };
 
 #endif // SECURE_WEB_SERVER_H