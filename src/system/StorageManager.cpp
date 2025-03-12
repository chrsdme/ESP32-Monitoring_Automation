/**
 * @file StorageManager.cpp
 * @brief Implementation of the StorageManager class
 */

 #include "StorageManager.h"
 #include "../core/AppCore.h"
 
 StorageManager::StorageManager() :
     _storageMutex(nullptr),
     _isInitialized(false),
     _factoryResetFlag(false)
 {
 }
 
 StorageManager::~StorageManager() {
     // Clean up RTOS resources
     if (_storageMutex != nullptr) {
         vSemaphoreDelete(_storageMutex);
     }
 }
 
 bool StorageManager::begin() {
     // Create mutex for thread-safe operations
     _storageMutex = xSemaphoreCreateMutex();
     if (_storageMutex == nullptr) {
         Serial.println("Failed to create storage mutex!");
         return false;
     }
     
     // Initialize SPIFFS
     if (!SPIFFS.begin(true)) {
         Serial.println("Failed to mount SPIFFS!");
         return false;
     }
     
     // Create required directories
     ensureDirectory("/config");
     ensureDirectory("/logs");
     
     // Check if default config exists, create if not
     if (!SPIFFS.exists(Constants::DEFAULT_CONFIG_FILE)) {
         saveDefaultConfig();
     }
     
     _isInitialized = true;
     return true;
 }
 
 FilesystemStats StorageManager::getFilesystemStats() {
     FilesystemStats stats;
     stats.totalBytes = SPIFFS.totalBytes();
     stats.usedBytes = SPIFFS.usedBytes();
     stats.freeBytes = stats.totalBytes - stats.usedBytes;
     return stats;
 }
 
 NVSStats StorageManager::getNVSStats() {
     NVSStats stats;
     stats.usedEntries = 0;
     stats.freeEntries = 0;
     stats.totalEntries = 0;
     
     // Try to get NVS stats
     nvs_stats_t nvsStats;
     if (nvs_get_stats(NULL, &nvsStats) == ESP_OK) {
         stats.usedEntries = nvsStats.used_entries;
         stats.freeEntries = nvsStats.free_entries;
         stats.totalEntries = nvsStats.total_entries;
     }
     
     return stats;
 }
 
 bool StorageManager::saveSettings() {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_storageMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         bool success = true;
         
         // Log the action
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Storage", "Saving settings");
         
         // TODO: In a real implementation, collect settings from various managers
         // and save them to the appropriate files or NVS.
         
         // Release mutex
         xSemaphoreGive(_storageMutex);
         return success;
     }
     
     return false;
 }
 
 bool StorageManager::loadSettings() {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_storageMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         bool success = true;
         
         // Log the action
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Storage", "Loading settings");
         
         // TODO: In a real implementation, load settings from files or NVS
         // and apply them to the appropriate managers.
         
         // Release mutex
         xSemaphoreGive(_storageMutex);
         return success;
     }
     
     return false;
 }
 
 bool StorageManager::saveDefaultConfig() {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_storageMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Create config directory if it doesn't exist
         ensureDirectory("/config");
         
         // Create a JSON document for default configuration
         DynamicJsonDocument doc(4096);
         
         // Default network configuration
         JsonObject networkObj = doc.createNestedObject("network");
         networkObj["hostname"] = Constants::DEFAULT_HOSTNAME;
         networkObj["dhcp"] = true;
         
         // Default sensor configuration
         JsonObject sensorObj = doc.createNestedObject("sensors");
         sensorObj["dht1_pin"] = Constants::DEFAULT_DHT1_PIN;
         sensorObj["dht2_pin"] = Constants::DEFAULT_DHT2_PIN;
         sensorObj["scd_sda_pin"] = Constants::DEFAULT_SCD40_SDA_PIN;
         sensorObj["scd_scl_pin"] = Constants::DEFAULT_SCD40_SCL_PIN;
         sensorObj["dht_interval"] = Constants::DEFAULT_DHT_READ_INTERVAL_MS / 1000;
         sensorObj["scd_interval"] = Constants::DEFAULT_SCD40_READ_INTERVAL_MS / 1000;
         sensorObj["graph_interval"] = Constants::DEFAULT_GRAPH_UPDATE_INTERVAL_MS / 1000;
         sensorObj["graph_points"] = Constants::DEFAULT_GRAPH_MAX_POINTS;
         
         // Default relay configuration
         JsonObject relayObj = doc.createNestedObject("relays");
         relayObj["override_duration"] = Constants::DEFAULT_USER_OVERRIDE_TIME_MIN;
         
         // Default environmental thresholds
         JsonObject envObj = doc.createNestedObject("environment");
         envObj["humidity_low"] = Constants::DEFAULT_HUMIDITY_LOW_THRESHOLD;
         envObj["humidity_high"] = Constants::DEFAULT_HUMIDITY_HIGH_THRESHOLD;
         envObj["temperature_low"] = Constants::DEFAULT_TEMPERATURE_LOW_THRESHOLD;
         envObj["temperature_high"] = Constants::DEFAULT_TEMPERATURE_HIGH_THRESHOLD;
         envObj["co2_low"] = Constants::DEFAULT_CO2_LOW_THRESHOLD;
         envObj["co2_high"] = Constants::DEFAULT_CO2_HIGH_THRESHOLD;
         
         // Create config file
         File configFile = SPIFFS.open(Constants::DEFAULT_CONFIG_FILE, FILE_WRITE);
         if (!configFile) {
             // Release mutex
             xSemaphoreGive(_storageMutex);
             return false;
         }
         
         // Write config to file
         serializeJson(doc, configFile);
         configFile.close();
         
         // Log the action
         Serial.println("Default configuration saved");
         
         // Release mutex
         xSemaphoreGive(_storageMutex);
         return true;
     }
     
     return false;
 }
 
 bool StorageManager::getFactoryResetFlag() {
     return _factoryResetFlag;
 }
 
 void StorageManager::setFactoryResetFlag(bool flag) {
     _factoryResetFlag = flag;
     
     if (flag) {
         getAppCore()->getLogManager()->log(LogLevel::WARN, "Storage", "Factory reset flag set");
     }
 }
 
 std::vector<String> StorageManager::listDirectory(const String& directory) {
     std::vector<String> files;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_storageMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Ensure directory path starts with /
         String dirPath = directory;
         if (!dirPath.startsWith("/")) {
             dirPath = "/" + dirPath;
         }
         
         // Make sure directory path ends with /
         if (!dirPath.endsWith("/") && dirPath != "/") {
             dirPath += "/";
         }
         
         // Open directory
         File dir = SPIFFS.open(dirPath);
         if (!dir || !dir.isDirectory()) {
             // Release mutex
             xSemaphoreGive(_storageMutex);
             return files;
         }
         
         // Read directory entries
         File file = dir.openNextFile();
         while (file) {
             if (!file.isDirectory()) {
                 // Add file name to list
                 String fileName = file.name();
                 
                 // Remove directory prefix if present
                 if (fileName.startsWith(dirPath)) {
                     fileName = fileName.substring(dirPath.length());
                 }
                 
                 files.push_back(fileName);
             }
             file = dir.openNextFile();
         }
         
         // Release mutex
         xSemaphoreGive(_storageMutex);
     }
     
     return files;
 }
 
 String StorageManager::readFile(const String& path) {
     String content = "";
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_storageMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Ensure path starts with /
         String filePath = path;
         if (!filePath.startsWith("/")) {
             filePath = "/" + filePath;
         }
         
         // Open file
         File file = SPIFFS.open(filePath, FILE_READ);
         if (!file || file.isDirectory()) {
             // Release mutex
             xSemaphoreGive(_storageMutex);
             return content;
         }
         
         // Read file content
         while (file.available()) {
             content += (char)file.read();
         }
         file.close();
         
         // Release mutex
         xSemaphoreGive(_storageMutex);
     }
     
     return content;
 }
 
 bool StorageManager::writeFile(const String& path, const String& content) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_storageMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Ensure path starts with /
         String filePath = path;
         if (!filePath.startsWith("/")) {
             filePath = "/" + filePath;
         }
         
         // Create directory if needed
         String directory = filePath.substring(0, filePath.lastIndexOf('/'));
         if (directory.length() > 0) {
             ensureDirectory(directory);
         }
         
         // Open file
         File file = SPIFFS.open(filePath, FILE_WRITE);
         if (!file) {
             // Release mutex
             xSemaphoreGive(_storageMutex);
             return false;
         }
         
         // Write content
         size_t written = file.print(content);
         file.close();
         
         // Check if all bytes were written
         bool success = (written == content.length());
         
         // Release mutex
         xSemaphoreGive(_storageMutex);
         return success;
     }
     
     return false;
 }
 
 bool StorageManager::deleteFile(const String& path) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_storageMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Ensure path starts with /
         String filePath = path;
         if (!filePath.startsWith("/")) {
             filePath = "/" + filePath;
         }
         
         // Delete file
         bool success = SPIFFS.remove(filePath);
         
         // Log the action
         if (success) {
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Storage", "Deleted file: " + filePath);
         } else {
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "Storage", "Failed to delete file: " + filePath);
         }
         
         // Release mutex
         xSemaphoreGive(_storageMutex);
         return success;
     }
     
     return false;
 }
 
 bool StorageManager::formatFilesystem() {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_storageMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Format SPIFFS
         bool success = SPIFFS.format();
         
         // Log the action
         if (success) {
             Serial.println("SPIFFS formatted successfully");
         } else {
             Serial.println("Failed to format SPIFFS");
         }
         
         // Release mutex
         xSemaphoreGive(_storageMutex);
         return success;
     }
     
     return false;
 }
 
 bool StorageManager::ensureDirectory(const String& dir) {
     // Ensure path starts with /
     String dirPath = dir;
     if (!dirPath.startsWith("/")) {
         dirPath = "/" + dirPath;
     }
     
     // Check if directory exists
     if (SPIFFS.exists(dirPath) || dirPath == "/") {
         return true;
     }
     
     // Create directory (SPIFFS doesn't support directories,
     // but we can create an empty file with a .dir extension)
     File file = SPIFFS.open(dirPath + "/.dir", FILE_WRITE);
     if (!file) {
         return false;
     }
     
     file.close();
     return true;
 }