/**
 * @file TapoManager.h
 * @brief Manages Tapo P100 smart sockets
 */

 #ifndef TAPO_MANAGER_H
 #define TAPO_MANAGER_H
 
 #include <Arduino.h>
 #include <freertos/FreeRTOS.h>
 #include <freertos/task.h>
 #include <freertos/semphr.h>
 #include <HTTPClient.h>
 #include <ArduinoJson.h>
 #include <vector>
 #include <map>
 #include "../utils/Constants.h"
 
 // Forward declarations
 class AppCore;
 
 /**
  * @struct TapoDevice
  * @brief Structure to hold Tapo device information
  */
 struct TapoDevice {
     String id;             // Unique identifier for the device
     String name;           // Display name
     String deviceType;     // Device type (P100, P110, etc.)
     String ipAddress;      // IP address of the device
     String macAddress;     // MAC address
     bool isOn;             // Current power state
     uint8_t relayReplacement; // Which relay this device replaces (0 = none)
     uint32_t lastUpdate;   // Timestamp of last status update
     bool online;           // Whether the device is online
     
     TapoDevice() : 
         id(""),
         name(""),
         deviceType(""),
         ipAddress(""),
         macAddress(""),
         isOn(false),
         relayReplacement(0),
         lastUpdate(0),
         online(false) {}
     
     TapoDevice(const String& id, const String& name, const String& deviceType, 
               const String& ipAddress, const String& macAddress, uint8_t relayReplacement) :
         id(id),
         name(name),
         deviceType(deviceType),
         ipAddress(ipAddress),
         macAddress(macAddress),
         isOn(false),
         relayReplacement(relayReplacement),
         lastUpdate(0),
         online(false) {}
 };
 
 /**
  * @class TapoManager
  * @brief Manages Tapo P100 smart socket devices
  */
 class TapoManager {
 public:
     TapoManager();
     ~TapoManager();
     
     /**
      * @brief Initialize the Tapo manager
      * @return True if initialized successfully
      */
     bool begin();
     
     /**
      * @brief Set Tapo account credentials
      * @param username Tapo account username/email
      * @param password Tapo account password
      * @return True if credentials set successfully
      */
     bool setCredentials(const String& username, const String& password);
     
     /**
      * @brief Get Tapo account credentials
      * @param username Output parameter for username
      * @param password Output parameter for password
      * @return True if credentials retrieved successfully
      */
     bool getCredentials(String& username, String& password);
     
     /**
      * @brief Add a Tapo device
      * @param device Device information
      * @return True if device added successfully
      */
     bool addDevice(const TapoDevice& device);
     
     /**
      * @brief Remove a Tapo device
      * @param deviceId Device ID to remove
      * @return True if device removed successfully
      */
     bool removeDevice(const String& deviceId);
     
     /**
      * @brief Update a Tapo device's information
      * @param deviceId Device ID to update
      * @param device Updated device information
      * @return True if device updated successfully
      */
     bool updateDevice(const String& deviceId, const TapoDevice& device);
     
     /**
      * @brief Get a specific Tapo device
      * @param deviceId Device ID to retrieve
      * @return Device information
      */
     TapoDevice getDevice(const String& deviceId);
     
     /**
      * @brief Get all Tapo devices
      * @return Vector of all Tapo devices
      */
     std::vector<TapoDevice> getAllDevices();
     
     /**
      * @brief Turn a Tapo device on or off
      * @param deviceId Device ID to control
      * @param state True to turn on, false to turn off
      * @return True if control operation successful
      */
     bool controlDevice(const String& deviceId, bool state);
     
     /**
      * @brief Get the status of a Tapo device
      * @param deviceId Device ID to check
      * @param forceUpdate Force a status update from the device
      * @return True if device is on
      */
     bool getDeviceStatus(const String& deviceId, bool forceUpdate = false);
     
     /**
      * @brief Discover Tapo devices on the network
      * @return Number of devices discovered
      */
     int discoverDevices();
     
     /**
      * @brief Update all device statuses
      * @return Number of devices successfully updated
      */
     int updateAllDeviceStatus();
     
     /**
      * @brief Create RTOS tasks for Tapo operations
      */
     void createTasks();
     
 private:
     // Configuration
     String _username;
     String _password;
     String _token;
     uint32_t _tokenExpiry;
     
     // Device storage
     std::map<String, TapoDevice> _devices;
     
     // RTOS resources
     SemaphoreHandle_t _tapoMutex;
     TaskHandle_t _tapoTaskHandle;
     
     // API implementation
     bool authenticate();
     bool sendDeviceCommand(const String& ipAddress, const String& command, JsonVariant& payload = JsonVariant());
     bool refreshToken();
     String getDeviceToken(const String& ipAddress);
     
     // Helper methods
     bool loadDevices();
     bool saveDevices();
     
     // Task function
     static void tapoTask(void* parameter);
 };
 
 #endif // TAPO_MANAGER_H