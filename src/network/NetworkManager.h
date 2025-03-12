/**
 * @file NetworkManager.h
 * @brief Handles all networking functionality including WiFi, mDNS, and network configuration
 */

 #ifndef NETWORK_MANAGER_H
 #define NETWORK_MANAGER_H
 
 #include <Arduino.h>
 #include <WiFi.h>
 #include <WiFiMulti.h>
 #include <ESPmDNS.h>
 #include <freertos/FreeRTOS.h>
 #include <freertos/task.h>
 #include <freertos/semphr.h>
 #include <nvs.h>
 #include <nvs_flash.h>
 #include "../utils/Constants.h"
 
 // Callback function types
 typedef std::function<void(const String&, const String&)> WiFiConnectedCallback;
 typedef std::function<void()> WiFiDisconnectedCallback;
 
 /**
  * @struct NetworkInfo
  * @brief Structure to hold network scan information
  */
 struct NetworkInfo {
     String ssid;
     int rssi;
     uint8_t bssid[6];
     int channel;
     wifi_auth_mode_t encryptionType;
 };
 
 /**
  * @class NetworkManager
  * @brief Manages all network-related functionality
  */
 class NetworkManager {
 public:
     NetworkManager();
     ~NetworkManager();
     
     /**
      * @brief Initialize the network manager
      * @return True if initialized successfully
      */
     bool begin();
     
     /**
      * @brief Start in Access Point mode for configuration
      * @param ssid Access point SSID
      * @param password Access point password (empty for open network)
      * @param enableSTA Also enable station mode for scanning
      * @return True if AP started successfully
      */
     bool startAPMode(const char* ssid, const char* password, bool enableSTA = false);
     
     /**
      * @brief Start in Station mode to connect to existing networks
      * @return True if STA mode started successfully
      */
     bool startSTAMode();
     
     /**
      * @brief Scan for WiFi networks
      * @return Vector of NetworkInfo structures with scan results
      */
     std::vector<NetworkInfo> scanNetworks();
     
     /**
      * @brief Set WiFi credentials
      * @param index Credential set index (0-2)
      * @param ssid Network SSID
      * @param password Network password
      * @return True if credentials saved successfully
      */
     bool setWiFiCredentials(uint8_t index, const String& ssid, const String& password);
     
     /**
      * @brief Get WiFi credentials
      * @param index Credential set index (0-2)
      * @param ssid Output parameter for SSID
      * @param password Output parameter for password
      * @return True if credentials retrieved successfully
      */
     bool getWiFiCredentials(uint8_t index, String& ssid, String& password);
     
     /**
      * @brief Test WiFi credentials
      * @param ssid Network SSID
      * @param password Network password
      * @return True if connection successful
      */
     bool testWiFiCredentials(const String& ssid, const String& password);
     
     /**
      * @brief Set hostname for mDNS
      * @param hostname Device hostname
      * @return True if hostname set successfully
      */
     bool setHostname(const String& hostname);
     
     /**
      * @brief Get current hostname
      * @return Device hostname
      */
     String getHostname();
     
     /**
      * @brief Set IP configuration
      * @param useDHCP True to use DHCP, false for static IP
      * @param ip Static IP address (if useDHCP is false)
      * @param gateway Gateway address
      * @param subnet Subnet mask
      * @param dns1 Primary DNS server
      * @param dns2 Secondary DNS server
      * @return True if configuration saved successfully
      */
     bool setIPConfig(bool useDHCP, const String& ip = "", const String& gateway = "", 
                     const String& subnet = "", const String& dns1 = "", const String& dns2 = "");
     
     /**
      * @brief Get IP configuration
      * @param useDHCP Output parameter for DHCP status
      * @param ip Output parameter for static IP
      * @param gateway Output parameter for gateway
      * @param subnet Output parameter for subnet
      * @param dns1 Output parameter for primary DNS
      * @param dns2 Output parameter for secondary DNS
      * @return True if configuration retrieved successfully
      */
     bool getIPConfig(bool& useDHCP, String& ip, String& gateway, 
                     String& subnet, String& dns1, String& dns2);
     
     /**
      * @brief Get current IP address
      * @return IP address as string
      */
     String getIPAddress();
     
     /**
      * @brief Get current connected SSID
      * @return SSID as string
      */
     String getConnectedSSID();
     
     /**
      * @brief Get current signal strength (RSSI)
      * @return RSSI value
      */
     int32_t getRSSI();
     
     /**
      * @brief Set minimum acceptable RSSI
      * @param rssi Minimum RSSI value
      */
     void setMinRSSI(int32_t rssi);
     
     /**
      * @brief Get minimum acceptable RSSI
      * @return Minimum RSSI value
      */
     int32_t getMinRSSI();
     
     /**
      * @brief Set WiFi check interval
      * @param interval Check interval in milliseconds
      */
     void setWiFiCheckInterval(uint32_t interval);
     
     /**
      * @brief Register callback for WiFi connected event
      * @param callback Function to call when WiFi connects
      */
     void onWiFiConnected(WiFiConnectedCallback callback);
     
     /**
      * @brief Register callback for WiFi disconnected event
      * @param callback Function to call when WiFi disconnects
      */
     void onWiFiDisconnected(WiFiDisconnectedCallback callback);
     
     /**
      * @brief Create RTOS tasks for network management
      */
     void createTasks();
     
 private:
     // State variables
     bool _isInitialized;
     bool _isInAPMode;
     bool _isConnected;
     String _hostname;
     String _currentSSID;
     int32_t _minRSSI;
     uint32_t _wifiCheckInterval;
     
     // IP configuration
     bool _useDHCP;
     IPAddress _staticIP;
     IPAddress _staticGateway;
     IPAddress _staticSubnet;
     IPAddress _staticDNS1;
     IPAddress _staticDNS2;
     
     // WiFi instances
     WiFiMulti _wifiMulti;
     
     // RTOS resources
     SemaphoreHandle_t _networkMutex;
     TaskHandle_t _wifiTaskHandle;
     
     // Callbacks
     WiFiConnectedCallback _connectedCallback;
     WiFiDisconnectedCallback _disconnectedCallback;
     
     // Private methods
     bool initializeNVS();
     bool setupMDNS();
     bool connectToWiFi();
     
     // Task function
     static void wifiWatchdogTask(void* parameter);
 };
 
 #endif // NETWORK_MANAGER_H