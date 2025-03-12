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
         for (const auto& profile : _profiles) {
             names.push_back(profile.name);
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
 
 String ProfileManager::getProfileJson(const String& name) {
     String json;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_profileMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Check if profile exists
         int idx = findProfileIndex(name);
         if (idx >= 0) {
             // Serialize profile to JSON
             serializeJson(_profiles[idx].doc, json);
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
        
        for (const auto& profile : _profiles) {
            JsonObject profileObj = profilesObj.createNestedObject(profile.name);
            
            // Manually copy all properties from the profile document to avoid type conversion issues
            JsonObject src = profile.doc.as<JsonObject>();
            for (JsonPair kv : src) {
                // For each key-value pair in the source object
                String key = kv.key().c_str();
                JsonVariant value = kv.value();
                
                if (value.is<JsonObject>()) {
                    // If the value is an object, create a nested object and copy its properties
                    JsonObject nestedObj = profileObj.createNestedObject(key);
                    JsonObject srcNestedObj = value.as<JsonObject>();
                    
                    for (JsonPair nestedKv : srcNestedObj) {
                        nestedObj[nestedKv.key()] = nestedKv.value();
                    }
                } else if (value.is<JsonArray>()) {
                    // If the value is an array, create a nested array and copy its elements
                    JsonArray nestedArr = profileObj.createNestedArray(key);
                    JsonArray srcNestedArr = value.as<JsonArray>();
                    
                    for (size_t i = 0; i < srcNestedArr.size(); i++) {
                        nestedArr.add(srcNestedArr[i]);
                    }
                } else {
                    // For primitive values, just copy directly
                    profileObj[key] = value;
                }
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
         int idx = findProfileIndex(name);
         
         if (idx < 0) {
             // Create new profile
             ProfileEntry newProfile(name);
             _profiles.push_back(newProfile);
             idx = _profiles.size() - 1;                                
         }
         
         // Update profile settings
         _profiles[idx].doc.clear();
         JsonObject dest = _profiles[idx].doc.to<JsonObject>();
         for (JsonPair kv : settings) {
             dest[kv.key()] = kv.value();
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
         // Check if profile exists
         int idx = findProfileIndex(name);
         if (idx < 0) {
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "Profiles", 
                 "Profile not found: " + name);
             
             // Release mutex
             xSemaphoreGive(_profileMutex);
             return false;
         }
         
         // Apply profile settings
         applyProfileSettings(_profiles[idx].doc);
         
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
         // Check if old profile exists and new name doesn't
         int oldIdx = findProfileIndex(oldName);
         int newIdx = findProfileIndex(newName);
         
         if (oldIdx < 0 || newIdx >= 0) {
             // Release mutex
             xSemaphoreGive(_profileMutex);
             return false;
         }
         
         // Rename profile
         _profiles[oldIdx].name = newName;
         
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
         // Check if profile exists
         int idx = findProfileIndex(name);
         if (idx < 0) {
             // Release mutex
             xSemaphoreGive(_profileMutex);
             return false;
         }
         
         // If deleting current profile, switch to another one
         if (_currentProfile == name) {
             for (const auto& profile : _profiles) {
                 if (profile.name != name) {
                     _currentProfile = profile.name;
                     break;
                 }
             }
         }
         
         // Remove profile
         _profiles.erase(_profiles.begin() + idx);
         
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
    if (!json.is<JsonObject>() || !json.as<JsonObject>().containsKey("profiles")) {
        return false;
    }
    
    // Take mutex to ensure thread safety
    if (xSemaphoreTake(_profileMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        // Get profiles object
        JsonObject profilesObj = json.as<JsonObject>()["profiles"].as<JsonObject>();
        
        // Clear existing profiles
        _profiles.clear();
        
        // Import profiles
        for (JsonPair kv : profilesObj) {
            String name = kv.key().c_str();
            JsonObject settings = kv.value().as<JsonObject>();
            
            DynamicJsonDocument profile(8192);
            JsonObject profileObj = profile.to<JsonObject>();
            profileObj.set(settings);
            
            _profiles[name] = profile;
        }
        
        // Get current profile
        if (json.as<JsonObject>().containsKey("current_profile")) {
            String currentProfile = json.as<JsonObject>()["current_profile"].as<String>();
            
            // Check if the profile exists
            auto it = _profiles.find(currentProfile);
            if (it != _profiles.end()) {
                _currentProfile = currentProfile;
            } else if (!_profiles.empty()) {
                _currentProfile = _profiles.begin()->first;
            }
        } else if (!_profiles.empty()) {
            _currentProfile = _profiles.begin()->first;
        }
        
        // Save changes and load current profile
        bool success = saveProfilesToFile();
        if (success && !_currentProfile.isEmpty()) {
            applyProfileSettings(_profiles[_currentProfile]);
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
         ProfileEntry defaultProfile("Default");
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
         ProfileEntry testProfile("Test");
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
         
         // Create Colonization profile
         ProfileEntry colonizationProfile("Colonization");
         JsonObject colonizationObj = colonizationProfile.doc.to<JsonObject>();
         
         // Colonization profile settings
         colonizationObj["name"] = "Colonization";
         
         JsonObject colonizationEnvObj = colonizationObj.createNestedObject("environment");
         colonizationEnvObj["humidity_low"] = 70.0f;
         colonizationEnvObj["humidity_high"] = 90.0f;
         colonizationEnvObj["temperature_low"] = 21.0f;
         colonizationEnvObj["temperature_high"] = 24.0f;
         colonizationEnvObj["co2_low"] = 1000.0f;
         colonizationEnvObj["co2_high"] = 2000.0f;
         
         JsonObject colonizationTimingObj = colonizationObj.createNestedObject("timing");
         colonizationTimingObj["dht_interval"] = 15;  // 15 seconds
         colonizationTimingObj["scd_interval"] = 30;  // 30 seconds
         colonizationTimingObj["graph_interval"] = 60;  // 1 minute
         colonizationTimingObj["graph_points"] = 120;
         
         JsonObject colonizationCycleObj = colonizationObj.createNestedObject("cycle");
         colonizationCycleObj["on_duration"] = 2;
         colonizationCycleObj["interval"] = 120;
         
         JsonObject colonizationMqttObj = colonizationObj.createNestedObject("mqtt");
         colonizationMqttObj["enabled"] = false;
         colonizationMqttObj["broker"] = Constants::DEFAULT_MQTT_BROKER;
         colonizationMqttObj["port"] = Constants::DEFAULT_MQTT_PORT;
         colonizationMqttObj["topic"] = "colonization/mushroom/tent";
         colonizationMqttObj["username"] = Constants::DEFAULT_MQTT_USERNAME;
         colonizationMqttObj["password"] = Constants::DEFAULT_MQTT_PASSWORD;
         
         JsonObject colonizationRelayTimesObj = colonizationObj.createNestedObject("relay_times");
         for (int i = 1; i <= 8; i++) {
             JsonObject relayObj = colonizationRelayTimesObj.createNestedObject("relay" + String(i));
             if (i == 2) {  // UV Light
                 relayObj["start_hour"] = 0;
                 relayObj["start_minute"] = 0;
                 relayObj["end_hour"] = 0;
                 relayObj["end_minute"] = 0;
             } else if (i == 3) {  // Grow Light
                 relayObj["start_hour"] = 6;
                 relayObj["start_minute"] = 0;
                 relayObj["end_hour"] = 18;
                 relayObj["end_minute"] = 0;
             } else {
                 relayObj["start_hour"] = 0;
                 relayObj["start_minute"] = 0;
                 relayObj["end_hour"] = 23;
                 relayObj["end_minute"] = 59;
             }
         }
         
         _profiles.push_back(colonizationProfile);
         
         // Create Fruiting profile
         ProfileEntry fruitingProfile("Fruiting");
         JsonObject fruitingObj = fruitingProfile.doc.to<JsonObject>();
         
         // Fruiting profile settings
         fruitingObj["name"] = "Fruiting";
         
         JsonObject fruitingEnvObj = fruitingObj.createNestedObject("environment");
         fruitingEnvObj["humidity_low"] = 80.0f;
         fruitingEnvObj["humidity_high"] = 95.0f;
         fruitingEnvObj["temperature_low"] = 18.0f;
         fruitingEnvObj["temperature_high"] = 22.0f;
         fruitingEnvObj["co2_low"] = 600.0f;
         fruitingEnvObj["co2_high"] = 1000.0f;
         
         JsonObject fruitingTimingObj = fruitingObj.createNestedObject("timing");
         fruitingTimingObj["dht_interval"] = 15;  // 15 seconds
         fruitingTimingObj["scd_interval"] = 30;  // 30 seconds
         fruitingTimingObj["graph_interval"] = 60;  // 1 minute
         fruitingTimingObj["graph_points"] = 120;
         
         JsonObject fruitingCycleObj = fruitingObj.createNestedObject("cycle");
         fruitingCycleObj["on_duration"] = 10;
         fruitingCycleObj["interval"] = 30;
         
         JsonObject fruitingMqttObj = fruitingObj.createNestedObject("mqtt");
         fruitingMqttObj["enabled"] = false;
         fruitingMqttObj["broker"] = Constants::DEFAULT_MQTT_BROKER;
         fruitingMqttObj["port"] = Constants::DEFAULT_MQTT_PORT;
         fruitingMqttObj["topic"] = "fruiting/mushroom/tent";
         fruitingMqttObj["username"] = Constants::DEFAULT_MQTT_USERNAME;
         fruitingMqttObj["password"] = Constants::DEFAULT_MQTT_PASSWORD;
         
         JsonObject fruitingRelayTimesObj = fruitingObj.createNestedObject("relay_times");
         for (int i = 1; i <= 8; i++) {
             JsonObject relayObj = fruitingRelayTimesObj.createNestedObject("relay" + String(i));
             if (i == 2) {  // UV Light
                 relayObj["start_hour"] = 10;
                 relayObj["start_minute"] = 0;
                 relayObj["end_hour"] = 14;
                 relayObj["end_minute"] = 0;
             } else if (i == 3) {  // Grow Light
                 relayObj["start_hour"] = 6;
                 relayObj["start_minute"] = 0;
                 relayObj["end_hour"] = 18;
                 relayObj["end_minute"] = 0;
             } else {
                 relayObj["start_hour"] = 0;
                 relayObj["start_minute"] = 0;
                 relayObj["end_hour"] = 23;
                 relayObj["end_minute"] = 59;
             }
         }
         
         _profiles.push_back(fruitingProfile);
         
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
    JsonObject profilesObj = doc["profiles"].as<JsonObject>();
    for (JsonPair kv : profilesObj) {
        String name = kv.key().c_str();
             
             // Copy settings to profile document
             for (JsonPair settingKv : settings) {
                 dest[settingKv.key()] = settingKv.value();
             }
             
             _profiles.push_back(entry