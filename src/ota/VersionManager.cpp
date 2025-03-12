/**
 * @file VersionManager.cpp
 * @brief Implementation of the VersionManager class
 */

 #include "VersionManager.h"
 #include "../core/AppCore.h"
 #include "../system/LogManager.h"
 #include <HTTPClient.h>
 #include <ArduinoJson.h>
 
 VersionManager::VersionManager() : 
     _versionMutex(nullptr)
 {
     // Initialize with constants from build
     _firmwareVersion = Version(
         Constants::APP_VERSION_MAJOR,
         Constants::APP_VERSION_MINOR,
         Constants::APP_VERSION_PATCH,
         Constants::APP_VERSION_BUILD,
         Constants::APP_VERSION_TIMESTAMP
     );
     
     _filesystemVersion = Version(
         Constants::FS_VERSION_MAJOR,
         Constants::FS_VERSION_MINOR,
         Constants::FS_VERSION_PATCH,
         Constants::FS_VERSION_BUILD,
         Constants::FS_VERSION_TIMESTAMP
     );
 }
 
 VersionManager::~VersionManager() {
     // Clean up RTOS resources
     if (_versionMutex != nullptr) {
         vSemaphoreDelete(_versionMutex);
     }
 }
 
 bool VersionManager::begin() {
     // Create mutex for thread-safe operations
     _versionMutex = xSemaphoreCreateMutex();
     if (_versionMutex == nullptr) {
         Serial.println("Failed to create version mutex!");
         return false;
     }
     
     // Load any saved versions
     loadVersions();
     
     getAppCore()->getLogManager()->log(LogLevel::INFO, "Version", 
         "Firmware version: " + _firmwareVersion.toString() + 
         ", FS version: " + _filesystemVersion.toString());
     
     return true;
 }
 
 Version VersionManager::getFirmwareVersion() {
     Version version;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_versionMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         version = _firmwareVersion;
         xSemaphoreGive(_versionMutex);
     }
     
     return version;
 }
 
 Version VersionManager::getFilesystemVersion() {
     Version version;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_versionMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         version = _filesystemVersion;
         xSemaphoreGive(_versionMutex);
     }
     
     return version;
 }
 
 bool VersionManager::setFirmwareVersion(const Version& version) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_versionMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         _firmwareVersion = version;
         
         // Save updated versions
         bool success = saveVersions();
         
         if (success) {
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Version", 
                 "Firmware version updated to " + version.toString());
         } else {
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "Version", 
                 "Failed to save firmware version");
         }
         
         xSemaphoreGive(_versionMutex);
         return success;
     }
     
     return false;
 }
 
 bool VersionManager::setFilesystemVersion(const Version& version) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_versionMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         _filesystemVersion = version;
         
         // Save updated versions
         bool success = saveVersions();
         
         if (success) {
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Version", 
                 "Filesystem version updated to " + version.toString());
         } else {
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "Version", 
                 "Failed to save filesystem version");
         }
         
         xSemaphoreGive(_versionMutex);
         return success;
     }
     
     return false;
 }
 
 String VersionManager::checkForUpdates(const String& checkUrl) {
     HTTPClient http;
     String url = checkUrl;
     
     // Use default URL if none provided
     if (url.isEmpty()) {
         url = Constants::DEFAULT_VERSION_CHECK_URL;
     }
     
     // Add current version info to the URL
     url += "?fw=" + _firmwareVersion.toString() + "&fs=" + _filesystemVersion.toString();
     
     // Send request
     getAppCore()->getLogManager()->log(LogLevel::INFO, "Version", 
         "Checking for updates at " + url);
     
     http.begin(url);
     int httpCode = http.GET();
     
     DynamicJsonDocument doc(1024);
     
     // Check if request was successful
     if (httpCode == 200) {
         String response = http.getString();
         
         DeserializationError error = deserializeJson(doc, response);
         if (error) {
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "Version", 
                 "Failed to parse update check response: " + String(error.c_str()));
             
             doc["success"] = false;
             doc["message"] = "Failed to parse response";
         } else {
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Version", 
                 "Update check completed");
         }
     } else {
         getAppCore()->getLogManager()->log(LogLevel::ERROR, "Version", 
             "Update check failed with HTTP code: " + String(httpCode));
         
         doc["success"] = false;
         doc["message"] = "HTTP request failed: " + String(httpCode);
     }
     
     http.end();
     
     // Add current versions to response
     if (!doc.containsKey("current")) {
         JsonObject current = doc.createNestedObject("current");
         current["firmware"] = _firmwareVersion.toString();
         current["filesystem"] = _filesystemVersion.toString();
     }
     
     String result;
     serializeJson(doc, result);
     return result;
 }
 
 String VersionManager::getChangelog(const Version& targetVersion) {
     // In a real implementation, this might fetch the changelog from a server
     // or read it from a local file. For now, we'll just return a placeholder.
     
     String currentVersion = _firmwareVersion.toString();
     String targetVersionStr = targetVersion.toString();
     
     return "Changelog from " + currentVersion + " to " + targetVersionStr + ":\n" +
            "- This is a placeholder changelog.\n" +
            "- Real changelog would be fetched or generated based on versions.";
 }
 
 bool VersionManager::storeVersionHistory(const Version& version, uint8_t type, bool status) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_versionMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Read existing history
         File file = SPIFFS.open("/config/version_history.json", FILE_READ);
         DynamicJsonDocument doc(4096);
         
         if (file) {
             DeserializationError error = deserializeJson(doc, file);
             file.close();
             
             if (error) {
                 getAppCore()->getLogManager()->log(LogLevel::ERROR, "Version", 
                     "Failed to parse version history: " + String(error.c_str()));
                 
                 // Create a new document if parsing failed
                 doc = DynamicJsonDocument(4096);
                 JsonArray history = doc.createNestedArray("history");
             }
         } else {
             // Create a new document if file doesn't exist
             JsonArray history = doc.createNestedArray("history");
         }
         
         // Add new entry to history
         JsonArray history = doc["history"].as<JsonArray>();
         if (history.size() >= 10) {
             // Limit history to last 10 entries
             history.remove(0);
         }
         
         JsonObject entry = history.createNestedObject();
         entry["version"] = version.toString();
         entry["type"] = type;
         entry["status"] = status;
         entry["timestamp"] = (uint32_t)time(nullptr);
         
         // Save updated history
         file = SPIFFS.open("/config/version_history.json", FILE_WRITE);
         if (!file) {
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "Version", 
                 "Failed to open version history for writing");
             
             xSemaphoreGive(_versionMutex);
             return false;
         }
         
         if (serializeJson(doc, file) == 0) {
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "Version", 
                 "Failed to write version history");
             
             file.close();
             xSemaphoreGive(_versionMutex);
             return false;
         }
         
         file.close();
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Version", 
             "Version history updated with " + version.toString());
         
         xSemaphoreGive(_versionMutex);
         return true;
     }
     
     return false;
 }
 
 String VersionManager::getVersionHistory() {
     // Read version history
     String result = "{}";
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_versionMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         File file = SPIFFS.open("/config/version_history.json", FILE_READ);
         
         if (file) {
             result = file.readString();
             file.close();
         } else {
             // Create a default response if file doesn't exist
             DynamicJsonDocument doc(128);
             doc["history"] = JsonArray();
             serializeJson(doc, result);
         }
         
         xSemaphoreGive(_versionMutex);
     }
     
     return result;
 }
 
 bool VersionManager::loadVersions() {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_versionMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         String content = getVersionFile();
         bool success = parseVersionFile(content);
         
         xSemaphoreGive(_versionMutex);
         return success;
     }
     
     return false;
 }
 
 bool VersionManager::saveVersions() {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_versionMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Create JSON document
         DynamicJsonDocument doc(512);
         
         doc["firmware_version"] = _firmwareVersion.toString();
         doc["firmware_timestamp"] = _firmwareVersion.timestamp;
         
         doc["filesystem_version"] = _filesystemVersion.toString();
         doc["filesystem_timestamp"] = _filesystemVersion.timestamp;
         
         // Save to file
         File file = SPIFFS.open("/config/versions.json", FILE_WRITE);
         if (!file) {
             xSemaphoreGive(_versionMutex);
             return false;
         }
         
         size_t written = serializeJson(doc, file);
         file.close();
         
         xSemaphoreGive(_versionMutex);
         return (written > 0);
     }
     
     return false;
 }
 
 String VersionManager::getVersionFile() {
     // Read version file
     File file = SPIFFS.open("/config/versions.json", FILE_READ);
     
     if (!file) {
         // File doesn't exist yet
         return "";
     }
     
     String content = file.readString();
     file.close();
     
     return content;
 }
 
 bool VersionManager::parseVersionFile(const String& content) {
     if (content.isEmpty()) {
         // No file content, keep defaults
         return false;
     }
     
     DynamicJsonDocument doc(512);
     DeserializationError error = deserializeJson(doc, content);
     
     if (error) {
         getAppCore()->getLogManager()->log(LogLevel::ERROR, "Version", 
             "Failed to parse version file: " + String(error.c_str()));
         return false;
     }
     
     // Parse firmware version
     if (doc.containsKey("firmware_version")) {
         _firmwareVersion = Version::fromString(doc["firmware_version"].as<String>());
         
         if (doc.containsKey("firmware_timestamp")) {
             _firmwareVersion.timestamp = doc["firmware_timestamp"].as<uint32_t>();
         }
     }
     
     // Parse filesystem version
     if (doc.containsKey("filesystem_version")) {
         _filesystemVersion = Version::fromString(doc["filesystem_version"].as<String>());
         
         if (doc.containsKey("filesystem_timestamp")) {
             _filesystemVersion.timestamp = doc["filesystem_timestamp"].as<uint32_t>();
         }
     }
     
     return true;
 }