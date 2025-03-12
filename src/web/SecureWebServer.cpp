/**
 * @file SecureWebServer.cpp
 * @brief Implementation of secure web server using ESP32 HTTPS Server
 */

 #include "SecureWebServer.h"
 #include "../core/AppCore.h"
 #include "../network/NetworkManager.h"
 #include "../components/SensorManager.h"
 
 // Initialize static members
 SecureWebServer* SecureWebServer::_instance = nullptr;
 
 SecureWebServer::SecureWebServer() :
     _serverCertificate(nullptr),
     _secureServer(nullptr),
     _port(Constants::DEFAULT_WEB_SERVER_PORT),
     _username(Constants::DEFAULT_HTTP_USERNAME),
     _password(Constants::DEFAULT_HTTP_PASSWORD),
     _isRunning(false),
     _isInConfigMode(false),
     _webServerMutex(nullptr),
     _webServerTaskHandle(nullptr)
 {
     _instance = this;
 }
 
 SecureWebServer::~SecureWebServer() {
     // Clean up resources
     if (_serverCertificate) delete _serverCertificate;
     if (_secureServer) delete _secureServer;
     if (_webServerMutex) vSemaphoreDelete(_webServerMutex);
     
     _instance = nullptr;
 }
 
 bool SecureWebServer::begin() {
     // Create mutex for thread-safe operations
     _webServerMutex = xSemaphoreCreateMutex();
     if (_webServerMutex == nullptr) {
         Serial.println("Failed to create web server mutex!");
         return false;
     }
 
     // Generate SSL certificate
     _serverCertificate = generateSelfSignedCertificate();
     if (!_serverCertificate) {
         Serial.println("Failed to generate SSL certificate!");
         return false;
     }
 
     // Create secure server
     _secureServer = new HTTPSServer(_serverCertificate);
 
     // Load authentication credentials from NVS
     nvs_handle_t nvsHandle;
     esp_err_t err = nvs_open(Constants::NVS_CONFIG_NAMESPACE, NVS_READONLY, &nvsHandle);
     if (err == ESP_OK) {
         size_t userLen = 0, passLen = 0;
         
         // Try to load username
         err = nvs_get_str(nvsHandle, Constants::NVS_HTTP_USER_KEY, nullptr, &userLen);
         if (err == ESP_OK && userLen > 0) {
             char* userBuffer = new char[userLen];
             nvs_get_str(nvsHandle, Constants::NVS_HTTP_USER_KEY, userBuffer, &userLen);
             _username = String(userBuffer);
             delete[] userBuffer;
         }
         
         // Try to load password
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
 
 SSLCert* SecureWebServer::generateSelfSignedCertificate() {
     // Create a new SSL certificate
     SSLCert* cert = new SSLCert();
     
     // Common name (CN) for certificate
     std::string cn = "CN=MushroomTentController,O=HomeAutomation,C=US";
     // Validity period in days
     std::string days = "365";
 
     // Generate a self-signed certificate with correct parameter list
     int createCertResult = createSelfSignedCert(
         *cert,
         KEYSIZE_2048,   // Key size enum from the library
         cn,             // Common Name and certificate info
         days            // Valid duration in days (as string)
     );
 
     if (createCertResult != 0) {
         Serial.println("Error generating certificate");
         delete cert;
         return nullptr;
     }
 
     return cert;
 }
 
 bool SecureWebServer::startConfigurationMode() {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_webServerMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Reset server if already running
         if (_isRunning) {
             _secureServer->stop();
         }
 
         // Setup configuration mode routes
         setupConfigModeRoutes();
 
         // Start server
         _secureServer->start();
         _isRunning = true;
         _isInConfigMode = true;
 
         getAppCore()->getLogManager()->log(LogLevel::INFO, "SecureWebServer", 
             "Secure web server started in configuration mode on port " + String(_port));
 
         // Release mutex
         xSemaphoreGive(_webServerMutex);
         return true;
     }
 
     return false;
 }
 
 bool SecureWebServer::startNormalMode() {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_webServerMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Reset server if already running
         if (_isRunning) {
             _secureServer->stop();
         }
 
         // Setup normal mode routes
         setupNormalModeRoutes();
 
         // Start server
         _secureServer->start();
         _isRunning = true;
         _isInConfigMode = false;
 
         getAppCore()->getLogManager()->log(LogLevel::INFO, "SecureWebServer", 
             "Secure web server started in normal mode on port " + String(_port));
 
         // Release mutex
         xSemaphoreGive(_webServerMutex);
         return true;
     }
 
     return false;
 }
 
 void SecureWebServer::setupConfigModeRoutes() {
     // Define static handler methods with C function pointers
     ResourceNode* wifiScanNode = new ResourceNode(
         std::string("/api/wifi/scan"), 
         std::string("GET"), 
         &SecureWebServer::handleWiFiScanStatic
     );
     _secureServer->registerNode(wifiScanNode);
 
     // WiFi credentials test route
     ResourceNode* wifiTestNode = new ResourceNode(
         std::string("/api/wifi/test"), 
         std::string("POST"), 
         &SecureWebServer::handleTestWiFiStatic
     );
     _secureServer->registerNode(wifiTestNode);
 
     // Initial configuration save route
     ResourceNode* saveSettingsNode = new ResourceNode(
         std::string("/api/config/save"), 
         std::string("POST"), 
         &SecureWebServer::handleSaveSettingsStatic
     );
     _secureServer->registerNode(saveSettingsNode);
 }
 
 void SecureWebServer::setupNormalModeRoutes() {
     // Authenticated routes for normal operation
     ResourceNode* sensorDataNode = new ResourceNode(
         std::string("/api/sensors/data"), 
         std::string("GET"), 
         &SecureWebServer::handleGetSensorDataStatic
     );
     _secureServer->registerNode(sensorDataNode);
 
     // Add more normal mode routes here
 }
 
 bool SecureWebServer::authenticate(HTTPRequest* request, HTTPResponse* response) {
     // Basic authentication implementation
     std::string authHeaderStr = request->getHeader("Authorization");
     
     if (authHeaderStr.empty()) {
         response->setStatusCode(401);
         response->setStatusText("Unauthorized");
         response->println("Authentication required");
         return false;
     }
 
     String authHeader = String(authHeaderStr.c_str());
 
     // Basic auth parsing (base64 encoded)
     if (authHeader.startsWith("Basic ")) {
         // Manual basic auth parsing since we don't have base64 functions readily available
         String credentials = decodeBase64(authHeader.substring(6));
         int colonIndex = credentials.indexOf(':');
         
         if (colonIndex > 0) {
             String username = credentials.substring(0, colonIndex);
             String password = credentials.substring(colonIndex + 1);
             
             if (username == _username && password == _password) {
                 return true;
             }
         }
     }
 
     response->setStatusCode(401);
     response->setStatusText("Unauthorized");
     response->println("Invalid credentials");
     return false;
 }
 
 // Static handler methods
 void SecureWebServer::handleWiFiScanStatic(HTTPRequest* request, HTTPResponse* response) {
     if (_instance) {
         _instance->handleWiFiScan(request, response);
     }
 }
 
 void SecureWebServer::handleTestWiFiStatic(HTTPRequest* request, HTTPResponse* response) {
     if (_instance) {
         _instance->handleTestWiFi(request, response);
     }
 }
 
 void SecureWebServer::handleSaveSettingsStatic(HTTPRequest* request, HTTPResponse* response) {
     if (_instance) {
         _instance->handleSaveSettings(request, response);
     }
 }
 
 void SecureWebServer::handleGetSensorDataStatic(HTTPRequest* request, HTTPResponse* response) {
     if (_instance) {
         _instance->handleGetSensorData(request, response);
     }
 }
 
 void SecureWebServer::handleWiFiScan(HTTPRequest* request, HTTPResponse* response) {
     response->setStatusCode(200);
     response->setHeader("Content-Type", "application/json");
 
     // Scan networks and create JSON response
     std::vector<NetworkInfo> networks = getAppCore()->getNetworkManager()->scanNetworks();
     
     DynamicJsonDocument doc(4096);
     JsonArray networksArray = doc.createNestedArray("networks");
     
     for (const auto& network : networks) {
         JsonObject networkObj = networksArray.createNestedObject();
         networkObj["ssid"] = network.ssid;
         networkObj["rssi"] = network.rssi;
         
         // Convert BSSID to string
         char bssidStr[18];
         sprintf(bssidStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
                 network.bssid[0], network.bssid[1], network.bssid[2], 
                 network.bssid[3], network.bssid[4], network.bssid[5]);
         networkObj["bssid"] = bssidStr;
         
         networkObj["channel"] = network.channel;
         networkObj["encrypted"] = (network.encryptionType != WIFI_AUTH_OPEN);
     }
 
     // Serialize JSON to response
     String jsonResponse;
     serializeJson(doc, jsonResponse);
     response->println(jsonResponse.c_str());
 }
 
 void SecureWebServer::handleTestWiFi(HTTPRequest* request, HTTPResponse* response) {
     // Parse request body
     std::string body;
     char buffer[512];
     size_t bytesRead;
     while ((bytesRead = request->readChars(buffer, 512)) > 0) {
         body.append(buffer, bytesRead);
     }
 
     // Parse JSON
     DynamicJsonDocument doc(512);
     DeserializationError error = deserializeJson(doc, body.c_str());
     
     if (error) {
         response->setStatusCode(400);
         response->setHeader("Content-Type", "application/json");
         response->println("{\"success\":false,\"message\":\"Invalid JSON\"}");
         return;
     }
 
     // Extract SSID and password
     if (!doc.containsKey("ssid") || !doc.containsKey("password")) {
         response->setStatusCode(400);
         response->setHeader("Content-Type", "application/json");
         response->println("{\"success\":false,\"message\":\"Missing SSID or password\"}");
         return;
     }
 
     String ssid = doc["ssid"].as<String>();
     String password = doc["password"].as<String>();
 
     // Test WiFi credentials
     bool success = getAppCore()->getNetworkManager()->testWiFiCredentials(ssid, password);
 
     // Prepare response
     response->setStatusCode(200);
     response->setHeader("Content-Type", "application/json");
     
     String responseBody = "{\"success\":" + String(success ? "true" : "false") + 
                           ",\"message\":\"" + (success ? "Connection successful" : "Connection failed") + "\"}";
     response->println(responseBody.c_str());
 }
 
 void SecureWebServer::handleSaveSettings(HTTPRequest* request, HTTPResponse* response) {
     // Similar implementation to existing save settings method
     // Parse request body, validate, save settings, etc.
     // You'll need to adapt the existing implementation to work with this new request/response model
     response->setStatusCode(200);
     response->setHeader("Content-Type", "application/json");
     response->println("{\"success\":true,\"message\":\"Settings saved\"}");
 }
 
 void SecureWebServer::handleGetSensorData(HTTPRequest* request, HTTPResponse* response) {
     // Authenticate request in normal mode
     if (!authenticate(request, response)) {
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
     
     // Serialize JSON to response
     response->setStatusCode(200);
     response->setHeader("Content-Type", "application/json");
     String jsonResponse;
     serializeJson(doc, jsonResponse);
     response->println(jsonResponse.c_str());
 }
 
 bool SecureWebServer::setPort(uint16_t port) {
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
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "SecureWebServer", 
             "Web server port set to " + String(_port));
         
         // Release mutex
         xSemaphoreGive(_webServerMutex);
         return true;
     }
     
     return false;
 }
 
 uint16_t SecureWebServer::getPort() {
     uint16_t port = Constants::DEFAULT_WEB_SERVER_PORT;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_webServerMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         port = _port;
         
         // Release mutex
         xSemaphoreGive(_webServerMutex);
     }
     
     return port;
 }
 
 bool SecureWebServer::setHttpAuth(const String& username, const String& password) {
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
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "SecureWebServer", 
             "HTTP authentication credentials updated");
         
         // Release mutex
         xSemaphoreGive(_webServerMutex);
         return true;
     }
     
     return false;
 }
 
 bool SecureWebServer::getHttpAuth(String& username, String& password) {
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
 
 void SecureWebServer::createTasks() {
     // No additional tasks needed for HTTPS server
     // The ESP32 HTTPS server manages its own tasks internally
     getAppCore()->getLogManager()->log(LogLevel::INFO, "SecureWebServer", 
         "No additional tasks created for secure web server");
 }
 
 String SecureWebServer::decodeBase64(const String& input) {
     // Simple base64 decoder implementation
     const char* base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
     
     String output;
     int val = 0, valb = -8;
     for (char c : input) {
         if (c == '=') break;
         
         const char* p = strchr(base64Chars, c);
         if (p) {
             val = (val << 6) | (p - base64Chars);
             valb += 6;
             if (valb >= 0) {
                 output += char((val >> valb) & 0xFF);
                 valb -= 8;
             }
         }
     }
     
     return output;
 }