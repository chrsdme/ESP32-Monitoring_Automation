/**
 * @file ProfileManager.cpp
 * @brief Implementation of the ProfileManager class
 */

 #include "ProfileManager.h"
 #include "../core/AppCore.h"
 #include "../system/LogManager.h"
 #include "../components/RelayManager.h"
 #include "../components/SensorManager.h"
 
 ProfileManager::ProfileManager() :
     _currentProfile("Default"),
     _mqttEnabled(false),
     _profileMutex(nullptr)
 {
 }
 
 ProfileManager::~ProfileManager() {
     // Clean up RTOS resources
     if (_profileMutex != nullptr) {
         vSemaphoreDelete(_profileMutex);
     }
 }
 
 bool ProfileManager::begin() {
     // Create mutex for thread-safe operations
     _profileMutex = xSemaphoreCreateMutex();
     if (_profileMutex == nullptr) {
         Serial.println("Failed to create profile mutex!");
         return false;
     }
     
     // Load profiles from file
     if (!loadProfilesFromFile()) {
         // Create default profiles if file doesn't exist
         createDefaultProfiles();
         saveProfilesToFile();
     }
     
     // Load the default profile
     loadProfile(_currentProfile);
     
     return true;
 }
 
 bool ProfileManager::isMQTTEnabled() {
     bool enabled = false;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_profileMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         enabled = _mqttEnabled;
         
         // Release mutex
         xSemaphoreGive(_profileMutex);
     }
     
     return enabled;
 }
 
 std::vector<String> ProfileManager::getProfileNames() {
     std::vector<String> names;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_profileMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         for (const auto& entry : _profiles) {
             names.push_back(entry.name);
         }
         
         // Release mutex
         xSemaphoreGive(_profileMutex);
     }
     
     return names;
 }
 
 String ProfileManager::getCurrentProfileName() {
     String name;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_profileMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         name = _currentProfile;
         
         // Release mutex
         xSemaphoreGive(_profileMutex);
     }
     
     return name;
 }
 
 int ProfileManager::findProfileIndex(const String& name) {
     for (size_t i = 0; i < _profiles.size(); i++) {
         if (_profiles[i].name == name) {
             return i;
         }
     }
     return -1; // Not found
 }
 
 String ProfileManager::getProfileJson(const String& name) {
     String json;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_profileMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Find profile by name
         int index = findProfileIndex(name);
         if (index >= 0) {
             // Serialize profile to JSON
             serializeJson(_profiles[index].doc, json);
         }
         
         // Release mutex
         xSemaphoreGive(_profileMutex);
     }
     
     return json;
 }
 
 String ProfileManager::getProfilesJson() {
     String json;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_profileMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Create JSON document with all profiles
         DynamicJsonDocument doc(16384);  // Adjust size based on expected number of profiles
         JsonObject profilesObj = doc.createNestedObject("profiles");
         
         for (const auto& entry : _profiles) {
            JsonObject profileObj = profilesObj.createNestedObject(entry.name);
            if (entry.doc.is<JsonObject>()) {
                JsonObject docObj = entry.doc;
                profileObj.set(docObj);
            }
        }
         
         doc["current_profile"] = _currentProfile;
         
         // Serialize to JSON
         serializeJson(doc, json);
         
         // Release mutex
         xSemaphoreGive(_profileMutex);
     }
     
     return json;
 }
 
 bool ProfileManager::saveProfile(const String& name, const JsonObject& settings) {
     if (name.isEmpty()) {
         return false;
     }
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_profileMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Find existing profile or create a new one
         int index = findProfileIndex(name);
         
         if (index >= 0) {
             // Update existing profile
             _profiles[index].doc.clear();
             _profiles[index].doc.set(settings);
         } else {
             // Create new profile
             _profiles.push_back(ProfileEntry(name, 8192));
             _profiles.back().doc.set(settings);
         }
         
         // Save to file
         bool success = saveProfilesToFile();
         
         // Release mutex
         xSemaphoreGive(_profileMutex);
         
         if (success) {
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Profiles", 
                 "Profile saved: " + name);
         } else {
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "Profiles", 
                 "Failed to save profile: " + name);
         }
         
         return success;
     }
     
     return false;
 }
 
 bool ProfileManager::loadProfile(const String& name) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_profileMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Find profile by name
         int index = findProfileIndex(name);
         
         if (index < 0) {
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "Profiles", 
                 "Profile not found: " + name);
             
             // Release mutex
             xSemaphoreGive(_profileMutex);
             return false;
         }
         
         // Apply profile settings
         applyProfileSettings(_profiles[index].doc);
         
         // Set as current profile
         _currentProfile = name;
         
         // Release mutex
         xSemaphoreGive(_profileMutex);
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Profiles", 
             "Profile loaded: " + name);
         
         return true;
     }
     
     return false;
 }
 
 bool ProfileManager::renameProfile(const String& oldName, const String& newName) {
     if (oldName.isEmpty() || newName.isEmpty() || oldName == newName) {
         return false;
     }
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_profileMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Find old profile
         int oldIndex = findProfileIndex(oldName);
         int newIndex = findProfileIndex(newName);
         
         if (oldIndex < 0 || newIndex >= 0) {
             // Old profile not found or new name already exists
             xSemaphoreGive(_profileMutex);
             return false;
         }
         
         // Rename profile
         _profiles[oldIndex].name = newName;
         
         // Update current profile if needed
         if (_currentProfile == oldName) {
             _currentProfile = newName;
         }
         
         // Save changes
         bool success = saveProfilesToFile();
         
         // Release mutex
         xSemaphoreGive(_profileMutex);
         
         if (success) {
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Profiles", 
                 "Profile renamed from '" + oldName + "' to '" + newName + "'");
         } else {
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "Profiles", 
                 "Failed to rename profile from '" + oldName + "' to '" + newName + "'");
         }
         
         return success;
     }
     
     return false;
 }
 
 bool ProfileManager::deleteProfile(const String& name) {
     if (name.isEmpty() || _profiles.size() <= 1) {
         return false;  // Can't delete the last profile
     }
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_profileMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Find profile by name
         int index = findProfileIndex(name);
         
         if (index < 0) {
             // Profile not found
             xSemaphoreGive(_profileMutex);
             return false;
         }
         
         // If deleting current profile, switch to another one
         if (_currentProfile == name) {
             for (const auto& entry : _profiles) {
                 if (entry.name != name) {
                     _currentProfile = entry.name;
                     break;
                 }
             }
         }
         
         // Remove profile
         _profiles.erase(_profiles.begin() + index);
         
         // Save changes
         bool success = saveProfilesToFile();
         
         // Release mutex
         xSemaphoreGive(_profileMutex);
         
         if (success) {
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Profiles", 
                 "Profile deleted: " + name);
         } else {
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "Profiles", 
                 "Failed to delete profile: " + name);
         }
         
         return success;
     }
     
     return false;
 }
 
 bool ProfileManager::importProfilesJson(const JsonVariant& json) {
    // Check if json is an object first
    if (!json.is<JsonObject>()) {
        return false;
    }
    
    // Access as JsonObject (without using as<>())
    JsonObject jsonObject = json;
    if (!jsonObject.containsKey("profiles")) {
        return false;
    }
    
    // Check if "profiles" is an object
    if (!jsonObject["profiles"].is<JsonObject>()) {
        return false;
    }
    
    // Now safely get the profiles object
    JsonObject profilesObj = jsonObject["profiles"];
    
    // Take mutex to ensure thread safety
    if (xSemaphoreTake(_profileMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        // Clear existing profiles
        _profiles.clear();
        
        // Import profiles
        for (JsonPair kv : profilesObj) {
            String name = kv.key().c_str();
            
            // Make sure this value is an object before trying to use it
            if (kv.value().is<JsonObject>()) {
                JsonObject settings = kv.value();
                
                ProfileEntry entry(name, 8192);
                entry.doc.set(settings);
                
                _profiles.push_back(entry);
            }
        }
        
        // Get current profile
        if (jsonObject.containsKey("current_profile")) {
            String currentProfile = jsonObject["current_profile"].as<String>();
            
            // Check if the profile exists
            int index = findProfileIndex(currentProfile);
            if (index >= 0) {
                _currentProfile = currentProfile;
            } else if (!_profiles.empty()) {
                _currentProfile = _profiles[0].name;
            }
        } else if (!_profiles.empty()) {
            _currentProfile = _profiles[0].name;
        }
        
        // Save changes and load current profile
        bool success = saveProfilesToFile();
        if (success && !_currentProfile.isEmpty()) {
            int index = findProfileIndex(_currentProfile);
            if (index >= 0) {
                applyProfileSettings(_profiles[index].doc);
            }
        }
        
        // Release mutex
        xSemaphoreGive(_profileMutex);
        
        if (success) {
            getAppCore()->getLogManager()->log(LogLevel::INFO, "Profiles", 
                "Profiles imported successfully");
        } else {
            getAppCore()->getLogManager()->log(LogLevel::ERROR, "Profiles", 
                "Failed to import profiles");
        }
        
        return success;
    }
    
    return false;
}
 
 bool ProfileManager::createDefaultProfiles() {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_profileMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Clear existing profiles
         _profiles.clear();
         
         // Create default profile
         ProfileEntry defaultProfile("Default", 8192);
         JsonObject defaultObj = defaultProfile.doc.to<JsonObject>();
         
         // Default profile settings
         defaultObj["name"] = "Default";
         
         JsonObject environmentObj = defaultObj.createNestedObject("environment");
         environmentObj["humidity_low"] = Constants::DEFAULT_HUMIDITY_LOW_THRESHOLD;
         environmentObj["humidity_high"] = Constants::DEFAULT_HUMIDITY_HIGH_THRESHOLD;
         environmentObj["temperature_low"] = Constants::DEFAULT_TEMPERATURE_LOW_THRESHOLD;
         environmentObj["temperature_high"] = Constants::DEFAULT_TEMPERATURE_HIGH_THRESHOLD;
         environmentObj["co2_low"] = Constants::DEFAULT_CO2_LOW_THRESHOLD;
         environmentObj["co2_high"] = Constants::DEFAULT_CO2_HIGH_THRESHOLD;
         
         JsonObject timingObj = defaultObj.createNestedObject("timing");
         timingObj["dht_interval"] = Constants::DEFAULT_DHT_READ_INTERVAL_MS / 1000;  // Convert to seconds
         timingObj["scd_interval"] = Constants::DEFAULT_SCD40_READ_INTERVAL_MS / 1000;  // Convert to seconds
         timingObj["graph_interval"] = Constants::DEFAULT_GRAPH_UPDATE_INTERVAL_MS / 1000;  // Convert to seconds
         timingObj["graph_points"] = Constants::DEFAULT_GRAPH_MAX_POINTS;
         
         JsonObject cycleObj = defaultObj.createNestedObject("cycle");
         cycleObj["on_duration"] = Constants::DEFAULT_FANS_ON_DURATION_MIN;
         cycleObj["interval"] = Constants::DEFAULT_FANS_CYCLE_INTERVAL_MIN;
         
         JsonObject mqttObj = defaultObj.createNestedObject("mqtt");
         mqttObj["enabled"] = false;
         mqttObj["broker"] = Constants::DEFAULT_MQTT_BROKER;
         mqttObj["port"] = Constants::DEFAULT_MQTT_PORT;
         mqttObj["topic"] = Constants::DEFAULT_MQTT_TOPIC;
         mqttObj["username"] = Constants::DEFAULT_MQTT_USERNAME;
         mqttObj["password"] = Constants::DEFAULT_MQTT_PASSWORD;
         
         JsonObject relayTimesObj = defaultObj.createNestedObject("relay_times");
         for (int i = 1; i <= 8; i++) {
             JsonObject relayObj = relayTimesObj.createNestedObject("relay" + String(i));
             relayObj["start_hour"] = 0;
             relayObj["start_minute"] = 0;
             relayObj["end_hour"] = 23;
             relayObj["end_minute"] = 59;
         }
         
         _profiles.push_back(defaultProfile);
         
         // Create Test profile
         ProfileEntry testProfile("Test", 8192);
         JsonObject testObj = testProfile.doc.to<JsonObject>();
         
         // Test profile settings (similar to default but with different thresholds)
         testObj["name"] = "Test";
         
         JsonObject testEnvObj = testObj.createNestedObject("environment");
         testEnvObj["humidity_low"] = 40.0f;
         testEnvObj["humidity_high"] = 80.0f;
         testEnvObj["temperature_low"] = 18.0f;
         testEnvObj["temperature_high"] = 24.0f;
         testEnvObj["co2_low"] = 800.0f;
         testEnvObj["co2_high"] = 1400.0f;
         
         JsonObject testTimingObj = testObj.createNestedObject("timing");
         testTimingObj["dht_interval"] = 10;  // 10 seconds
         testTimingObj["scd_interval"] = 20;  // 20 seconds
         testTimingObj["graph_interval"] = 30;  // 30 seconds
         testTimingObj["graph_points"] = 60;
         
         JsonObject testCycleObj = testObj.createNestedObject("cycle");
         testCycleObj["on_duration"] = 5;
         testCycleObj["interval"] = 15;
         
         JsonObject testMqttObj = testObj.createNestedObject("mqtt");
         testMqttObj["enabled"] = false;
         testMqttObj["broker"] = Constants::DEFAULT_MQTT_BROKER;
         testMqttObj["port"] = Constants::DEFAULT_MQTT_PORT;
         testMqttObj["topic"] = "test/mushroom/tent";
         testMqttObj["username"] = Constants::DEFAULT_MQTT_USERNAME;
         testMqttObj["password"] = Constants::DEFAULT_MQTT_PASSWORD;
         
         JsonObject testRelayTimesObj = testObj.createNestedObject("relay_times");
         for (int i = 1; i <= 8; i++) {
             JsonObject relayObj = testRelayTimesObj.createNestedObject("relay" + String(i));
             relayObj["start_hour"] = 8;
             relayObj["start_minute"] = 0;
             relayObj["end_hour"] = 20;
             relayObj["end_minute"] = 0;
         }
         
         _profiles.push_back(testProfile);
         
         // Set default as current profile
         _currentProfile = "Default";
         
         // Release mutex
         xSemaphoreGive(_profileMutex);
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Profiles", 
             "Default profiles created");
         
         return true;
     }
     
     return false;
 }
 bool ProfileManager::loadProfilesFromFile() {
     // Check if profiles file exists
     if (!SPIFFS.exists(Constants::PROFILES_FILE)) {
         getAppCore()->getLogManager()->log(LogLevel::WARN, "Profiles", 
             "Profiles file not found: " + String(Constants::PROFILES_FILE));
         return false;
     }
     
     // Open profiles file
     File file = SPIFFS.open(Constants::PROFILES_FILE, FILE_READ);
     if (!file) {
         getAppCore()->getLogManager()->log(LogLevel::ERROR, "Profiles", 
             "Failed to open profiles file: " + String(Constants::PROFILES_FILE));
         return false;
     }
     
     // Read file content
     size_t size = file.size();
     std::unique_ptr<char[]> buf(new char[size + 1]);
     file.readBytes(buf.get(), size);
     buf[size] = '\0';
     file.close();
     
     // Parse JSON
     DynamicJsonDocument doc(16384);
     DeserializationError error = deserializeJson(doc, buf.get());
     
     if (error) {
         getAppCore()->getLogManager()->log(LogLevel::ERROR, "Profiles", 
             "Failed to parse profiles file: " + String(error.c_str()));
         return false;
     }
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_profileMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Clear existing profiles
         _profiles.clear();
         
        // Load profiles         
if (doc.containsKey("profiles") && doc["profiles"].is<JsonObject>()) {
    JsonObject profilesObj = doc["profiles"];
    for (JsonPair kv : profilesObj) {
        // Safe conversion when we know it's an object
        String name = kv.key().c_str();
        // Check if this element is an object before converting
        if (kv.value().is<JsonObject>()) {
            JsonObject settings = kv.value();
            
            ProfileEntry entry(name, 8192);
            entry.doc.set(settings);
            
            _profiles.push_back(entry);
        }
    }
}
         
         // Get current profile
         if (doc.containsKey("current_profile")) {
             String currentProfile = doc["current_profile"].as<String>();
             
             // Check if the profile exists
             int index = findProfileIndex(currentProfile);
             if (index >= 0) {
                 _currentProfile = currentProfile;
             } else if (!_profiles.empty()) {
                 _currentProfile = _profiles[0].name;
             }
         } else if (!_profiles.empty()) {
             _currentProfile = _profiles[0].name;
         }
         
         // Release mutex
         xSemaphoreGive(_profileMutex);
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Profiles", 
             "Profiles loaded from file: " + String(Constants::PROFILES_FILE));
         
         return true;
     }
     
     return false;
 }
 
 bool ProfileManager::saveProfilesToFile() {
     // Create directory if it doesn't exist
     String configDir = "/config";
     if (!SPIFFS.exists(configDir)) {
         SPIFFS.mkdir(configDir);
     }
     
     // Create JSON document with all profiles
     DynamicJsonDocument doc(16384);
     JsonObject profilesObj = doc.createNestedObject("profiles");
     
     for (const auto& entry : _profiles) {
        JsonObject profileObj = profilesObj.createNestedObject(entry.name);
        if (entry.doc.is<JsonObject>()) {
            JsonObject docObj = entry.doc;
            profileObj.set(docObj);
        }
    }

     
     doc["current_profile"] = _currentProfile;
     
     // Open file for writing
     File file = SPIFFS.open(Constants::PROFILES_FILE, FILE_WRITE);
     if (!file) {
         getAppCore()->getLogManager()->log(LogLevel::ERROR, "Profiles", 
             "Failed to open profiles file for writing: " + String(Constants::PROFILES_FILE));
         return false;
     }
     
     // Write JSON to file
     if (serializeJson(doc, file) == 0) {
         file.close();
         getAppCore()->getLogManager()->log(LogLevel::ERROR, "Profiles", 
             "Failed to write profiles to file: " + String(Constants::PROFILES_FILE));
         return false;
     }
     
     file.close();
     
     getAppCore()->getLogManager()->log(LogLevel::INFO, "Profiles", 
         "Profiles saved to file: " + String(Constants::PROFILES_FILE));
     
     return true;
 }
 
 void ProfileManager::applyProfileSettings(const DynamicJsonDocument& profileSettings) {
    // Apply environment settings
    if (profileSettings.containsKey("environment") && profileSettings["environment"].is<JsonObject>()) {
        JsonObject environmentObj = profileSettings["environment"];
        
        if (environmentObj.containsKey("humidity_low") && environmentObj.containsKey("humidity_high") && 
            environmentObj.containsKey("temperature_low") && environmentObj.containsKey("temperature_high") && 
            environmentObj.containsKey("co2_low") && environmentObj.containsKey("co2_high")) {
            
            getAppCore()->getRelayManager()->setEnvironmentalThresholds(
                environmentObj["humidity_low"].as<float>(),
                environmentObj["humidity_high"].as<float>(),
                environmentObj["temperature_low"].as<float>(),
                environmentObj["temperature_high"].as<float>(),
                environmentObj["co2_low"].as<float>(),
                environmentObj["co2_high"].as<float>()
            );
        }
    }
    
    // Apply timing settings
    if (profileSettings.containsKey("timing") && profileSettings["timing"].is<JsonObject>()) {
        JsonObject timingObj = profileSettings["timing"];
        
        if (timingObj.containsKey("dht_interval") && timingObj.containsKey("scd_interval")) {
            getAppCore()->getSensorManager()->setSensorIntervals(
                timingObj["dht_interval"].as<uint32_t>() * 1000,  // Convert to milliseconds
                timingObj["scd_interval"].as<uint32_t>() * 1000
            );
        }
        
        // Graph settings would be applied elsewhere
    }
    
    // Apply cycle settings
    if (profileSettings.containsKey("cycle") && profileSettings["cycle"].is<JsonObject>()) {
        JsonObject cycleObj = profileSettings["cycle"];
        
        if (cycleObj.containsKey("on_duration") && cycleObj.containsKey("interval")) {
            getAppCore()->getRelayManager()->setCycleConfig(
                cycleObj["on_duration"].as<uint16_t>(),
                cycleObj["interval"].as<uint16_t>()
            );
        }
    }
    
    // Apply relay operating times
    if (profileSettings.containsKey("relay_times") && profileSettings["relay_times"].is<JsonObject>()) {
        JsonObject relayTimesObj = profileSettings["relay_times"];
        
        for (int i = 1; i <= 8; i++) {
            String relayKey = "relay" + String(i);
            if (relayTimesObj.containsKey(relayKey) && relayTimesObj[relayKey].is<JsonObject>()) {
                JsonObject relayObj = relayTimesObj[relayKey];
                
                if (relayObj.containsKey("start_hour") && relayObj.containsKey("start_minute") && 
                    relayObj.containsKey("end_hour") && relayObj.containsKey("end_minute")) {
                    
                    getAppCore()->getRelayManager()->setRelayOperatingTime(
                        i,
                        relayObj["start_hour"].as<uint8_t>(),
                        relayObj["start_minute"].as<uint8_t>(),
                        relayObj["end_hour"].as<uint8_t>(),
                        relayObj["end_minute"].as<uint8_t>()
                    );
                }
            }
        }
    }
    
    // Apply MQTT settings
    if (profileSettings.containsKey("mqtt") && profileSettings["mqtt"].is<JsonObject>()) {
        JsonObject mqttObj = profileSettings["mqtt"];
        
        if (mqttObj.containsKey("enabled")) {
            _mqttEnabled = mqttObj["enabled"].as<bool>();
        }
        
        // Other MQTT settings would be applied to an MQTT client in a real implementation
    }
}