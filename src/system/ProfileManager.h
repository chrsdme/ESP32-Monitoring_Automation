/**
 * @file ProfileManager.h
 * @brief Manages user profiles for configuring the system
 */

 #ifndef PROFILE_MANAGER_H
 #define PROFILE_MANAGER_H
 
 #include <Arduino.h>
 #include <ArduinoJson.h>
 #include <SPIFFS.h>
 #include <freertos/FreeRTOS.h>
 #include <freertos/semphr.h>
 #include <vector>
 #include "../utils/Constants.h"
 
 // Forward declarations
 class AppCore;
 
 /**
  * @class ProfileManager
  * @brief Manages user profiles for storing and applying configuration settings
  */
 class ProfileManager {
 public:
     // Structure to represent a profile
     struct ProfileEntry {
         String name;
         DynamicJsonDocument doc;  // Each profile has its own JSON document
         
         // Constructor with mandatory size parameter
         ProfileEntry(const String& name, size_t capacity = 8192) 
             : name(name), doc(capacity) {}
     };
 
     ProfileManager();
     ~ProfileManager();
     
     /**
      * @brief Initialize the profile manager
      * @return True if initialized successfully
      */
     bool begin();
     
     /**
      * @brief Check if MQTT is enabled in the current profile
      * @return True if MQTT is enabled
      */
     bool isMQTTEnabled();
     
     /**
      * @brief Get all available profile names
      * @return Vector of profile names
      */
     std::vector<String> getProfileNames();
     
     /**
      * @brief Get the current profile name
      * @return Current profile name
      */
     String getCurrentProfileName();
     
     /**
      * @brief Get profile settings as JSON string
      * @param name Profile name
      * @return Profile settings as JSON string
      */
     String getProfileJson(const String& name);
     
     /**
      * @brief Get all profiles as JSON string
      * @return All profiles as JSON string
      */
     String getProfilesJson();
     
     /**
      * @brief Save profile settings
      * @param name Profile name
      * @param settings Profile settings as JSON object
      * @return True if profile saved successfully
      */
     bool saveProfile(const String& name, const JsonObject& settings);
     
     /**
      * @brief Load profile settings
      * @param name Profile name
      * @return True if profile loaded successfully
      */
     bool loadProfile(const String& name);
     
     /**
      * @brief Rename a profile
      * @param oldName Old profile name
      * @param newName New profile name
      * @return True if profile renamed successfully
      */
     bool renameProfile(const String& oldName, const String& newName);
     
     /**
      * @brief Delete a profile
      * @param name Profile name
      * @return True if profile deleted successfully
      */
     bool deleteProfile(const String& name);
     
     /**
      * @brief Import profiles from JSON
      * @param json JSON containing profiles
      * @return True if profiles imported successfully
      */
     bool importProfilesJson(const JsonVariant& json);
     
     /**
      * @brief Create default profiles
      * @return True if default profiles created successfully
      */
     bool createDefaultProfiles();
     
 private:
     // Profile data
     std::vector<ProfileEntry> _profiles;
     String _currentProfile;
     
     // MQTT status
     bool _mqttEnabled;
     
     // RTOS resources
     SemaphoreHandle_t _profileMutex;
     
     // Private methods
     bool loadProfilesFromFile();
     bool saveProfilesToFile();
     void applyProfileSettings(const DynamicJsonDocument& profileSettings);
     
     // Helper methods
     int findProfileIndex(const String& name);
 };
 
 #endif // PROFILE_MANAGER_H