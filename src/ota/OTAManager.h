/**
 * @file OTAManager.h
 * @brief Manages over-the-air firmware updates
 */

 #ifndef OTA_MANAGER_H
 #define OTA_MANAGER_H
 
 #include <Arduino.h>
 #include <ArduinoOTA.h>
 #include <Update.h>
 #include <freertos/FreeRTOS.h>
 #include <freertos/semphr.h>
 #include "../utils/Constants.h"
 
 // Forward declarations
 class AppCore;
 
 // OTA update types
 enum class OTAType {
     FIRMWARE,
     FILESYSTEM,
     BOTH
 };
 
 // OTA update status
 enum class OTAStatus {
     IDLE,
     UPDATING_FIRMWARE,
     UPDATING_FILESYSTEM,
     UPDATE_COMPLETE,
     UPDATE_FAILED
 };
 
 /**
  * @class OTAManager
  * @brief Manages over-the-air firmware and file system updates
  */
 class OTAManager {
 public:
     OTAManager();
     ~OTAManager();
     
     /**
      * @brief Initialize the OTA manager
      * @return True if initialized successfully
      */
     bool begin();
     
     /**
      * @brief Enable OTA updates
      * @param port Port number for OTA updates
      * @param password OTA password
      * @return True if OTA enabled successfully
      */
     bool enableUpdates(uint16_t port = Constants::DEFAULT_OTA_PORT, const String& password = "");
     
     /**
      * @brief Disable OTA updates
      */
     void disableUpdates();
     
     /**
      * @brief Check if OTA updates are enabled
      * @return True if OTA updates are enabled
      */
     bool areUpdatesEnabled();
     
     /**
      * @brief Get current firmware version
      * @return Firmware version
      */
     String getFirmwareVersion();
     
     /**
      * @brief Get current file system version
      * @return File system version
      */
     String getFilesystemVersion();
     
     /**
      * @brief Handle firmware update
      * @param data Update data
      * @param len Data length
      * @param final Whether this is the final chunk
      * @return True if update handled successfully
      */
     bool handleFirmwareUpdate(uint8_t* data, size_t len, bool final);
     
     /**
      * @brief Handle file system update
      * @param data Update data
      * @param len Data length
      * @param final Whether this is the final chunk
      * @return True if update handled successfully
      */
     bool handleFilesystemUpdate(uint8_t* data, size_t len, bool final);
     
     /**
      * @brief Get update progress
      * @return Update progress (0-100)
      */
     int getUpdateProgress();
     
     /**
      * @brief Get update status
      * @return Current update status
      */
     OTAStatus getUpdateStatus();
     
     /**
      * @brief Get last update error
      * @return Error message
      */
     String getLastError();
     
 private:
     // RTOS resources
     SemaphoreHandle_t _otaMutex;
     
     // Internal state
     bool _updatesEnabled;
     int _updateProgress;
     OTAStatus _updateStatus;
     String _lastError;
     size_t _totalSize;
     size_t _currentSize;
     
     // OTA handlers
     void onStart();
     void onEnd();
     void onProgress(uint progress, uint total);
     void onError(ota_error_t error);
     
     // Helper methods
     bool beginUpdate(OTAType type, size_t size);
     bool writeUpdate(uint8_t* data, size_t len);
     bool endUpdate();
 };
 
 #endif // OTA_MANAGER_H