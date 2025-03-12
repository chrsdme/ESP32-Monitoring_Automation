/**
 * @file NetworkManager.cpp
 * @brief Implementation of the NetworkManager class
 */

 #include "NetworkManager.h"
 #include "../core/AppCore.h"
 #include "../system/LogManager.h"
 
 NetworkManager::NetworkManager() :
     _isInitialized(false),
     _isInAPMode(false),
     _isConnected(false),
     _hostname(Constants::DEFAULT_HOSTNAME),
     _currentSSID(""),
     _minRSSI(Constants::DEFAULT_MIN_RSSI),
     _wifiCheckInterval(Constants::WIFI_CHECK_INTERVAL),
     _useDHCP(true),
     _networkMutex(nullptr),
     _wifiTaskHandle(nullptr),
     _connectedCallback(nullptr),
     _disconnectedCallback(nullptr)
 {
 }
 
 NetworkManager::~NetworkManager() {
     // Clean up RTOS resources
     if (_networkMutex != nullptr) {
         vSemaphoreDelete(_networkMutex);
     }
     
     if (_wifiTaskHandle != nullptr) {
         vTaskDelete(_wifiTaskHandle);
     }
 }
 
 bool NetworkManager::begin() {
     // Create mutex for thread-safe operations
     _networkMutex = xSemaphoreCreateMutex();
     if (_networkMutex == nullptr) {
         Serial.println("Failed to create network mutex!");
         return false;
     }
     
     // Initialize NVS for storing WiFi credentials
     if (!initializeNVS()) {
         Serial.println("Failed to initialize NVS for WiFi!");
         return false;
     }
     
     // Load hostname from NVS if available
     nvs_handle_t nvsHandle;
     esp_err_t err = nvs_open(Constants::NVS_WIFI_NAMESPACE, NVS_READONLY, &nvsHandle);
     if (err == ESP_OK) {
         size_t len = 0;
         err = nvs_get_str(nvsHandle, Constants::NVS_HOSTNAME_KEY, nullptr, &len);
         if (err == ESP_OK && len > 0) {
             char* hostnameBuffer = new char[len];
             nvs_get_str(nvsHandle, Constants::NVS_HOSTNAME_KEY, hostnameBuffer, &len);
             _hostname = String(hostnameBuffer);
             delete[] hostnameBuffer;
         }
         nvs_close(nvsHandle);
     }
     
     _isInitialized = true;
     return true;
 }
 
 bool NetworkManager::startAPMode(const char* ssid, const char* password, bool enableSTA) {
     if (!_isInitialized) {
         return false;
     }
     
     WiFi.mode(enableSTA ? WIFI_AP_STA : WIFI_AP);
     
     // Start the access point
     bool result = WiFi.softAP(ssid, password);
     if (!result) {
         return false;
     }
     
     _isInAPMode = true;
     
     // Print AP info
     IPAddress apIP = WiFi.softAPIP();
     Serial.println("AP Mode Started");
     Serial.print("IP Address: ");
     Serial.println(apIP.toString());
     
     getAppCore()->getLogManager()->log(LogLevel::INFO, "Network", "AP Mode started with SSID: " + String(ssid));
     getAppCore()->getLogManager()->log(LogLevel::INFO, "Network", "AP IP Address: " + apIP.toString());
     
     return true;
 }
 
 bool NetworkManager::startSTAMode() {
     if (!_isInitialized) {
         return false;
     }
     
     // Set WiFi mode to station
     WiFi.mode(WIFI_STA);
     
     // Set hostname
     WiFi.setHostname(_hostname.c_str());
     
     // Setup mDNS
     setupMDNS();
     
     // Connect to WiFi using credentials
     bool connected = connectToWiFi();
     
     if (connected) {
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Network", 
             "Connected to WiFi SSID: " + getConnectedSSID() + 
             " with IP: " + getIPAddress());
         
         _isInAPMode = false;
         _isConnected = true;
         
         // Call connection callback if set
         if (_connectedCallback) {
             _connectedCallback(getIPAddress(), getConnectedSSID());
         }
     } else {
         getAppCore()->getLogManager()->log(LogLevel::ERROR, "Network", "Failed to connect to any WiFi network");
     }
     
     return connected;
 }
 
 std::vector<NetworkInfo> NetworkManager::scanNetworks() {
     std::vector<NetworkInfo> networks;
     
     // Ensure we're in a mode that can scan (STA or AP+STA)
     if (WiFi.getMode() == WIFI_AP) {
         WiFi.mode(WIFI_AP_STA);
     } else if (WiFi.getMode() == WIFI_OFF) {
         WiFi.mode(WIFI_STA);
     }
     
     getAppCore()->getLogManager()->log(LogLevel::INFO, "Network", "Scanning for WiFi networks...");
     
     int numNetworks = WiFi.scanNetworks();
     
     if (numNetworks == 0) {
         getAppCore()->getLogManager()->log(LogLevel::WARN, "Network", "No WiFi networks found!");
     } else {
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Network", String(numNetworks) + " WiFi networks found");
         
         for (int i = 0; i < numNetworks; i++) {
             NetworkInfo network;
             network.ssid = WiFi.SSID(i);
             network.rssi = WiFi.RSSI(i);
             memcpy(network.bssid, WiFi.BSSID(i), 6);
             network.channel = WiFi.channel(i);
             network.encryptionType = WiFi.encryptionType(i);
             networks.push_back(network);
             
             // Log network info
             char bssidStr[18];
             sprintf(bssidStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
                     network.bssid[0], network.bssid[1], network.bssid[2], 
                     network.bssid[3], network.bssid[4], network.bssid[5]);
             
             Serial.printf("Network %d: SSID: %s, MAC: %s, RSSI: %d, Channel: %d\n", 
                         i + 1, network.ssid.c_str(), bssidStr, network.rssi, network.channel);
         }
     }
     
     // Free memory used by scan
     WiFi.scanDelete();
     
     return networks;
 }
 
 bool NetworkManager::setWiFiCredentials(uint8_t index, const String& ssid, const String& password) {
     if (index > 2) {
         return false;  // Only support 3 credential sets (indices 0-2)
     }
     
     nvs_handle_t nvsHandle;
     esp_err_t err = nvs_open(Constants::NVS_WIFI_NAMESPACE, NVS_READWRITE, &nvsHandle);
     if (err != ESP_OK) {
         return false;
     }
     
     // Define key strings based on index
     String ssidKey = "wifi_ssid" + String(index + 1);
     String passKey = "wifi_pass" + String(index + 1);
     
     // Save SSID and password
     err = nvs_set_str(nvsHandle, ssidKey.c_str(), ssid.c_str());
     if (err != ESP_OK) {
         nvs_close(nvsHandle);
         return false;
     }
     
     err = nvs_set_str(nvsHandle, passKey.c_str(), password.c_str());
     if (err != ESP_OK) {
         nvs_close(nvsHandle);
         return false;
     }
     
     // Commit changes
     err = nvs_commit(nvsHandle);
     nvs_close(nvsHandle);
     
     if (err != ESP_OK) {
         return false;
     }
     
     getAppCore()->getLogManager()->log(LogLevel::INFO, "Network", 
         "WiFi credentials saved for SSID: " + ssid + " at position " + String(index + 1));
     
     return true;
 }
 
 bool NetworkManager::getWiFiCredentials(uint8_t index, String& ssid, String& password) {
     if (index > 2) {
         return false;  // Only support 3 credential sets (indices 0-2)
     }
     
     nvs_handle_t nvsHandle;
     esp_err_t err = nvs_open(Constants::NVS_WIFI_NAMESPACE, NVS_READONLY, &nvsHandle);
     if (err != ESP_OK) {
         return false;
     }
     
     // Define key strings based on index
     String ssidKey = "wifi_ssid" + String(index + 1);
     String passKey = "wifi_pass" + String(index + 1);
     
     // Get SSID
     size_t ssidLen = 0;
     err = nvs_get_str(nvsHandle, ssidKey.c_str(), nullptr, &ssidLen);
     if (err != ESP_OK || ssidLen == 0) {
         nvs_close(nvsHandle);
         return false;
     }
     
     char* ssidBuffer = new char[ssidLen];
     err = nvs_get_str(nvsHandle, ssidKey.c_str(), ssidBuffer, &ssidLen);
     if (err != ESP_OK) {
         delete[] ssidBuffer;
         nvs_close(nvsHandle);
         return false;
     }
     ssid = String(ssidBuffer);
     delete[] ssidBuffer;
     
     // Get password
     size_t passLen = 0;
     err = nvs_get_str(nvsHandle, passKey.c_str(), nullptr, &passLen);
     if (err != ESP_OK) {
         nvs_close(nvsHandle);
         return false;
     }
     
     char* passBuffer = new char[passLen];
     err = nvs_get_str(nvsHandle, passKey.c_str(), passBuffer, &passLen);
     if (err != ESP_OK) {
         delete[] passBuffer;
         nvs_close(nvsHandle);
         return false;
     }
     password = String(passBuffer);
     delete[] passBuffer;
     
     nvs_close(nvsHandle);
     return true;
 }
 
 bool NetworkManager::testWiFiCredentials(const String& ssid, const String& password) {
     // Only test if we're not already connected to that SSID
     if (_isConnected && _currentSSID == ssid) {
         return true;
     }
     
     // Save current WiFi state
     bool wasInAPMode = _isInAPMode;
     bool wasConnected = _isConnected;
     String prevSSID = _currentSSID;
     
     // Set mode to station for testing
     WiFi.mode(WIFI_STA);
     
     // Disconnect from any current connection
     WiFi.disconnect();
     
     getAppCore()->getLogManager()->log(LogLevel::INFO, "Network", 
         "Testing WiFi credentials for SSID: " + ssid);
     
     // Try to connect with the provided credentials
     WiFi.begin(ssid.c_str(), password.c_str());
     
     // Wait for connection with timeout
     uint32_t startTime = millis();
     while (WiFi.status() != WL_CONNECTED && millis() - startTime < Constants::WIFI_CONNECT_TIMEOUT) {
         delay(100);
     }
     
     bool connected = (WiFi.status() == WL_CONNECTED);
     
     if (connected) {
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Network", 
             "WiFi credential test succeeded for SSID: " + ssid);
     } else {
         getAppCore()->getLogManager()->log(LogLevel::WARN, "Network", 
             "WiFi credential test failed for SSID: " + ssid);
     }
     
     // Disconnect
     WiFi.disconnect();
     
     // Restore previous state
     if (wasInAPMode) {
         startAPMode(Constants::DEFAULT_AP_SSID, Constants::DEFAULT_AP_PASSWORD);
     } else if (wasConnected) {
         // Try to reconnect to previous network
         WiFi.begin(prevSSID.c_str(), nullptr);  // Password should be in memory
     }
     
     return connected;
 }
 
 bool NetworkManager::setHostname(const String& hostname) {
     _hostname = hostname;
     
     // Save hostname in NVS
     nvs_handle_t nvsHandle;
     esp_err_t err = nvs_open(Constants::NVS_WIFI_NAMESPACE, NVS_READWRITE, &nvsHandle);
     if (err != ESP_OK) {
         return false;
     }
     
     err = nvs_set_str(nvsHandle, Constants::NVS_HOSTNAME_KEY, hostname.c_str());
     if (err != ESP_OK) {
         nvs_close(nvsHandle);
         return false;
     }
     
     err = nvs_commit(nvsHandle);
     nvs_close(nvsHandle);
     
     if (err != ESP_OK) {
         return false;
     }
     
     // If we're connected, update the hostname
     if (_isConnected) {
         WiFi.setHostname(hostname.c_str());
         setupMDNS();  // Restart mDNS with new hostname
     }
     
     getAppCore()->getLogManager()->log(LogLevel::INFO, "Network", 
         "Hostname set to: " + hostname);
     
     return true;
 }
 
 String NetworkManager::getHostname() {
     return _hostname;
 }
 
 bool NetworkManager::setIPConfig(bool useDHCP, const String& ip, const String& gateway, 
                                 const String& subnet, const String& dns1, const String& dns2) {
     _useDHCP = useDHCP;
     
     if (!useDHCP) {
         // Parse IP addresses
         if (!_staticIP.fromString(ip) || 
             !_staticGateway.fromString(gateway) || 
             !_staticSubnet.fromString(subnet) || 
             !_staticDNS1.fromString(dns1)) {
             return false;
         }
         
         // DNS2 is optional
         if (dns2.length() > 0) {
             _staticDNS2.fromString(dns2);
         }
     }
     
     // Save configuration to storage
     // Note: In a real implementation, you'd save these to NVS or SPIFFS
     
     // If we're connected, apply the new configuration
     if (_isConnected) {
         if (useDHCP) {
             // Configure for DHCP
             WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
         } else {
             // Configure for static IP
             WiFi.config(_staticIP, _staticGateway, _staticSubnet, _staticDNS1, _staticDNS2);
         }
     }
     
     return true;
 }
 
 bool NetworkManager::getIPConfig(bool& useDHCP, String& ip, String& gateway, 
                                 String& subnet, String& dns1, String& dns2) {
     useDHCP = _useDHCP;
     
     if (!useDHCP) {
         ip = _staticIP.toString();
         gateway = _staticGateway.toString();
         subnet = _staticSubnet.toString();
         dns1 = _staticDNS1.toString();
         dns2 = _staticDNS2.toString();
     } else {
         // If using DHCP, return current values
         ip = WiFi.localIP().toString();
         gateway = WiFi.gatewayIP().toString();
         subnet = WiFi.subnetMask().toString();
         dns1 = WiFi.dnsIP(0).toString();
         dns2 = WiFi.dnsIP(1).toString();
     }
     
     return true;
 }
 
 String NetworkManager::getIPAddress() {
     if (_isInAPMode) {
         return WiFi.softAPIP().toString();
     } else if (_isConnected) {
         return WiFi.localIP().toString();
     } else {
         return "0.0.0.0";
     }
 }
 
 String NetworkManager::getConnectedSSID() {
     if (_isConnected) {
         return WiFi.SSID();
     } else {
         return "";
     }
 }
 
 int32_t NetworkManager::getRSSI() {
     if (_isConnected) {
         return WiFi.RSSI();
     } else {
         return 0;
     }
 }
 
 void NetworkManager::setMinRSSI(int32_t rssi) {
     _minRSSI = rssi;
 }
 
 int32_t NetworkManager::getMinRSSI() {
     return _minRSSI;
 }
 
 void NetworkManager::setWiFiCheckInterval(uint32_t interval) {
     _wifiCheckInterval = interval;
 }
 
 void NetworkManager::onWiFiConnected(WiFiConnectedCallback callback) {
     _connectedCallback = callback;
 }
 
 void NetworkManager::onWiFiDisconnected(WiFiDisconnectedCallback callback) {
     _disconnectedCallback = callback;
 }
 
 bool NetworkManager::initializeNVS() {
     // NVS should already be initialized by AppCore
     return true;
 }
 
 bool NetworkManager::setupMDNS() {
     // Stop any existing mDNS
     MDNS.end();
     
     // Start mDNS with the configured hostname
     if (!MDNS.begin(_hostname.c_str())) {
         getAppCore()->getLogManager()->log(LogLevel::ERROR, "Network", "Failed to start mDNS");
         return false;
     }
     
     // Add service to mDNS
     MDNS.addService("http", "tcp", 80);
     
     getAppCore()->getLogManager()->log(LogLevel::INFO, "Network", 
         "mDNS started with hostname: " + _hostname + ".local");
     
     return true;
 }
 
 bool NetworkManager::connectToWiFi() {
     // Clear any existing configurations in WiFiMulti
     _wifiMulti = WiFiMulti();
     
     // Add all stored WiFi credentials to WiFiMulti
     bool anyCredentialsAdded = false;
     for (uint8_t i = 0; i < 3; i++) {
         String ssid, password;
         if (getWiFiCredentials(i, ssid, password)) {
             if (ssid.length() > 0) {
                 _wifiMulti.addAP(ssid.c_str(), password.c_str());
                 anyCredentialsAdded = true;
                 getAppCore()->getLogManager()->log(LogLevel::INFO, "Network", 
                     "Added WiFi credentials for SSID: " + ssid);
             }
         }
     }
     
     if (!anyCredentialsAdded) {
         getAppCore()->getLogManager()->log(LogLevel::ERROR, "Network", 
             "No WiFi credentials available");
         return false;
     }
     
     // Configure IP if using static IP
     if (!_useDHCP) {
         WiFi.config(_staticIP, _staticGateway, _staticSubnet, _staticDNS1, _staticDNS2);
     }
     
     // Try to connect with WiFiMulti
     getAppCore()->getLogManager()->log(LogLevel::INFO, "Network", "Connecting to WiFi...");
     
     uint32_t startTime = millis();
     while (_wifiMulti.run() != WL_CONNECTED && millis() - startTime < Constants::WIFI_CONNECT_TIMEOUT) {
         delay(100);
     }
     
     if (WiFi.status() == WL_CONNECTED) {
         _currentSSID = WiFi.SSID();
         return true;
     }
     
     return false;
 }
 
 void NetworkManager::createTasks() {
     // Create the WiFi watchdog task
     BaseType_t result = xTaskCreatePinnedToCore(
         wifiWatchdogTask,          // Task function
         "WiFiWatchdog",            // Task name
         Constants::STACK_SIZE_WIFI,// Stack size (words)
         this,                      // Task parameters
         Constants::PRIORITY_WIFI,  // Priority
         &_wifiTaskHandle,          // Task handle
         0                          // Core ID (0 - protocol core)
     );
     
     if (result != pdPASS) {
         getAppCore()->getLogManager()->log(LogLevel::ERROR, "Network", 
             "Failed to create WiFi watchdog task");
     }
 }
 
 void NetworkManager::wifiWatchdogTask(void* parameter) {
     NetworkManager* networkManager = static_cast<NetworkManager*>(parameter);
     TickType_t lastWakeTime = xTaskGetTickCount();
     
     while (true) {
         // Only do checks in station mode
         if (!networkManager->_isInAPMode) {
             if (WiFi.status() != WL_CONNECTED) {
                 // We lost connection, try to reconnect
                 if (networkManager->_isConnected) {
                     networkManager->_isConnected = false;
                     
                     // Call disconnection callback if set
                     if (networkManager->_disconnectedCallback) {
                         networkManager->_disconnectedCallback();
                     }
                 }
                 
                 // Try to reconnect
                 if (networkManager->connectToWiFi()) {
                     networkManager->_isConnected = true;
                     
                     // Call connection callback if set
                     if (networkManager->_connectedCallback) {
                         networkManager->_connectedCallback(
                             networkManager->getIPAddress(), 
                             networkManager->getConnectedSSID()
                         );
                     }
                 }
             } else {
                 // We're connected, check signal strength
                 if (networkManager->_isConnected) {
                     int32_t rssi = WiFi.RSSI();
                     if (rssi < networkManager->_minRSSI) {
                         // Signal is too weak, try to find a better network
                         getAppCore()->getLogManager()->log(LogLevel::WARN, "Network", 
                             "WiFi signal weak (" + String(rssi) + " dBm), looking for better network");
                         
                         // Try to connect to a different network
                         WiFi.disconnect();
                         networkManager->_isConnected = false;
                         
                         // Call disconnection callback if set
                         if (networkManager->_disconnectedCallback) {
                             networkManager->_disconnectedCallback();
                         }
                     }
                 } else {
                     // We just got connected
                     networkManager->_isConnected = true;
                     
                     // Call connection callback if set
                     if (networkManager->_connectedCallback) {
                         networkManager->_connectedCallback(
                             networkManager->getIPAddress(), 
                             networkManager->getConnectedSSID()
                         );
                     }
                 }
             }
         }
         
         // Wait for the next check period
         vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(networkManager->_wifiCheckInterval));
     }
 }