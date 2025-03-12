/**
 * @file StorageManager.h
 * @brief Manages file system and non-volatile storage operations
 */

 #ifndef STORAGE_MANAGER_H
 #define STORAGE_MANAGER_H
 
 #include <Arduino.h>
 #include <SPIFFS.h>
 #include <nvs.h>
 #include <nvs_flash.h>
 #include <esp_partition.h>
 #include <ArduinoJson.h>
 #include <freertos/FreeRTOS.h>
 #include <freertos/semphr.h>
 #include "../utils/Constants.h"
 
 /**
  * @struct FilesystemStats
  * @brief Structure to hold filesystem statistics
  */
 struct FilesystemStats {
     size_t totalBytes;
     size_t usedBytes;
     size_t freeBytes;
 };
 
 /**
  * @struct NVSStats
  * @brief Structure to hold NVS statistics
  */
 struct NVSStats {
     size_t usedEntries;
     size_t freeEntries;
     size_t totalEntries;
 };
 
 /**
  * @class StorageManager
  * @brief Manages file system and NVS operations
  */
 class StorageManager {
 public:
     StorageManager();
     ~StorageManager();
     
     /**
      * @brief Initialize the storage manager
      * @return True if initialized successfully
      */
     bool begin();
     
     /**
      * @brief Get filesystem statistics
      * @return Filesystem statistics
      */
     FilesystemStats getFilesystemStats();
     
     /**
      * @brief Get NVS statistics
      * @return NVS statistics
      */
     NVSStats getNVSStats();
     
     /**
      * @brief Save settings to storage
      * @return True if settings saved successfully
      */
     bool saveSettings();
     
     /**
      * @brief Load settings from storage
      * @return True if settings loaded successfully
      */
     bool loadSettings();
     
     /**
      * @brief Save the default configuration
      * @return True if configuration saved successfully
      */
     bool saveDefaultConfig();
     
     /**
      * @brief Check if the factory reset flag is set
      * @return True if factory reset flag is set
      */
     bool getFactoryResetFlag();
     
     /**
      * @brief Set the factory reset flag
      * @param flag Flag value
      */
     void setFactoryResetFlag(bool flag);
     
     /**
      * @brief Get a list of files in a directory
      * @param directory Directory path
      * @return Vector of file names
      */
     std::vector<String> listDirectory(const String& directory = "/");
     
     /**
      * @brief Read a file as a string
      * @param path File path
      * @return File contents as string
      */
     String readFile(const String& path);
     
     /**
      * @brief Write a string to a file
      * @param path File path
      * @param content File content
      * @return True if file written successfully
      */
     bool writeFile(const String& path, const String& content);
     
     /**
      * @brief Delete a file
      * @param path File path
      * @return True if file deleted successfully
      */
     bool deleteFile(const String& path);
     
     /**
      * @brief Format the filesystem
      * @return True if format successful
      */
     bool formatFilesystem();
     
 private:
     // RTOS resources
     SemaphoreHandle_t _storageMutex;
     
     // Internal state
     bool _isInitialized;
     bool _factoryResetFlag;
     
     // Helper methods
     bool ensureDirectory(const String& dir);
 };
 
 #endif // STORAGE_MANAGER_H