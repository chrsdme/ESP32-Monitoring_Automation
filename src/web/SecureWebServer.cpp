/**
 * @file SecureWebServer.cpp
 * @brief Implementation of secure web server using ESP32 HTTPS Server
 */

#include "SecureWebServer.h"
#include "../core/AppCore.h"
#include "../network/NetworkManager.h"
#include "../components/SensorManager.h"

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
}

SecureWebServer::~SecureWebServer() {
    // Clean up resources
    if (_serverCertificate) delete _serverCertificate;
    if (_secureServer) delete _secureServer;
    if (_webServerMutex) vSemaphoreDelete(_webServerMutex);
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

    // Generate a self-signed certificate
    int createCertResult = createSelfSignedCert(
        *cert,
        EMBER_CERT_KEY_TYPE_RSA,  // Key type
        2048,                     // Key length
        "CN=MushroomTentController,O=HomeAutomation,C=US",  // Subject
        365                       // Validity days
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
    // Add configuration mode routes
    ResourceNode* wifiScanNode = new ResourceNode("/api/wifi/scan", "GET", 
        [this](HTTPRequest* request, HTTPResponse* response) { 
            this->handleWiFiScan(request, response); 
        });
    _secureServer->registerNode(wifiScanNode);

    // WiFi credentials test route
    ResourceNode* wifiTestNode = new ResourceNode("/api/wifi/test", "POST", 
        [this](HTTPRequest* request, HTTPResponse* response) { 
            this->handleTestWiFi(request, response); 
        });
    _secureServer->registerNode(wifiTestNode);

    // Initial configuration save route
    ResourceNode* saveSettingsNode = new ResourceNode("/api/config/save", "POST", 
        [this](HTTPRequest* request, HTTPResponse* response) { 
            this->handleSaveSettings(request, response); 
        });
    _secureServer->registerNode(saveSettingsNode);
}

void SecureWebServer::setupNormalModeRoutes() {
    // Authenticated routes for normal operation
    ResourceNode* sensorDataNode = new ResourceNode("/api/sensors/data", "GET", 
        [this](HTTPRequest* request, HTTPResponse* response) { 
            this->handleGetSensorData(request, response); 
        });
    _secureServer->registerNode(sensorDataNode);

    // Add more normal mode routes here
}

bool SecureWebServer::authenticate(HTTPRequest* request, HTTPResponse* response) {
    // Basic authentication implementation
    String authHeader = request->getHeaderValue("Authorization");
    
    if (authHeader.isEmpty()) {
        response->setStatusCode(401);
        response->setStatusText("Unauthorized");
        response->println("Authentication required");
        return false;
    }

    // Basic auth parsing (base64 encoded)
    if (authHeader.startsWith("Basic ")) {
        // Note: In a real-world scenario, use a more secure base64 decoding method
        String credentials = base64::decode(authHeader.substring(6));
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
    response->println(jsonResponse);
}

void SecureWebServer::handleTestWiFi(HTTPRequest* request, HTTPResponse* response) {
    // Parse request body
    std::string body;
    char buffer[512];
    size_t bytesRead;
    while ((bytesRead = request->readBytes(buffer, 512)) > 0) {
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
    response->println(responseBody);
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
    response->println(jsonResponse);
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

// Add more methods as needed for full functionality