/**
 * @file OTAManager.cpp
 * @brief Implementation of the OTAManager class
 */

 #include "OTAManager.h"
 #include "../core/AppCore.h"
 
 OTAManager::OTAManager() :
     _otaMutex(nullptr),
     _updatesEnabled(false),
     _updateProgress(0),
     _updateStatus(OTAStatus::IDLE),
     _lastError(""),
     _totalSize(0),
     _currentSize(0)
 {
 }
 
 OTAManager::~OTAManager() {
     // Clean up RTOS resources
     if (_otaMutex != nullptr) {
         vSemaphoreDelete(_otaMutex);
     }
     
     // Disable OTA updates
     disableUpdates();
 }
 
 bool OTAManager::begin() {
     // Create mutex for thread-safe operations
     _otaMutex = xSemaphoreCreateMutex();
     if (_otaMutex == nullptr) {
         Serial.println("Failed to create OTA mutex!");
         return false;
     }
     
     // Setup ArduinoOTA handlers
     ArduinoOTA.onStart([this]() { this->onStart(); });
     ArduinoOTA.onEnd([this]() { this->onEnd(); });
     ArduinoOTA.onProgress([this](uint progress, uint total) { this->onProgress(progress, total); });
     ArduinoOTA.onError([this](ota_error_t error) { this->onError(error); });
     
     return true;
 }
 
 bool OTAManager::enableUpdates(uint16_t port, const String& password) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_otaMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Set port
         ArduinoOTA.setPort(port);
         
         // Set password if provided
         if (password.length() > 0) {
             ArduinoOTA.setPassword(password.c_str());
         }
         
         // Set hostname
         String hostname = getAppCore()->getNetworkManager()->getHostname();
         if (hostname.length() > 0) {
             ArduinoOTA.setHostname(hostname.c_str());
         } else {
             ArduinoOTA.setHostname(Constants::DEFAULT_HOSTNAME);
         }
         
         // Begin OTA
         ArduinoOTA.begin();
         _updatesEnabled = true;
         
         // Log the action
         getAppCore()->getLogManager()->log(LogLevel::INFO, "OTA", 
             "OTA updates enabled on port " + String(port));
         
         // Release mutex
         xSemaphoreGive(_otaMutex);
         return true;
     }
     
     return false;
 }
 
 void OTAManager::disableUpdates() {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_otaMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Disable OTA
         if (_updatesEnabled) {
             _updatesEnabled = false;
             
             // Log the action
             getAppCore()->getLogManager()->log(LogLevel::INFO, "OTA", "OTA updates disabled");
         }
         
         // Release mutex
         xSemaphoreGive(_otaMutex);
     }
 }
 
 bool OTAManager::areUpdatesEnabled() {
     return _updatesEnabled;
 }
 
 String OTAManager::getFirmwareVersion() {
     return Constants::APP_VERSION;
 }
 
 String OTAManager::getFilesystemVersion() {
     return Constants::FS_VERSION;
 }
 
 bool OTAManager::handleFirmwareUpdate(uint8_t* data, size_t len, bool final) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_otaMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         bool success = true;
         
         // Start update if this is the first chunk
         if (_updateStatus == OTAStatus::IDLE) {
             success = beginUpdate(OTAType::FIRMWARE, len);
             if (success) {
                 _updateStatus = OTAStatus::UPDATING_FIRMWARE;
             }
         }
         
         // Write update data if in progress
         if (success && _updateStatus == OTAStatus::UPDATING_FIRMWARE) {
             success = writeUpdate(data, len);
         }
         
         // Finish update if this is the final chunk
         if (success && final && _updateStatus == OTAStatus::UPDATING_FIRMWARE) {
             success = endUpdate();
             if (success) {
                 _updateStatus = OTAStatus::UPDATE_COMPLETE;
             } else {
                 _updateStatus = OTAStatus::UPDATE_FAILED;
             }
         }
         
         // Release mutex
         xSemaphoreGive(_otaMutex);
         return success;
     }
     
     return false;
 }
 
 bool OTAManager::handleFilesystemUpdate(uint8_t* data, size_t len, bool final) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_otaMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         bool success = true;
         
         // Start update if this is the first chunk
         if (_updateStatus == OTAStatus::IDLE) {
             success = beginUpdate(OTAType::FILESYSTEM, len);
             if (success) {
                 _updateStatus = OTAStatus::UPDATING_FILESYSTEM;
             }
         }
         
         // Write update data if in progress
         if (success && _updateStatus == OTAStatus::UPDATING_FILESYSTEM) {
             success = writeUpdate(data, len);
         }
         
         // Finish update if this is the final chunk
         if (success && final && _updateStatus == OTAStatus::UPDATING_FILESYSTEM) {
             success = endUpdate();
             if (success) {
                 _updateStatus = OTAStatus::UPDATE_COMPLETE;
             } else {
                 _updateStatus = OTAStatus::UPDATE_FAILED;
             }
         }
         
         // Release mutex
         xSemaphoreGive(_otaMutex);
         return success;
     }
     
     return false;
 }
 
 int OTAManager::getUpdateProgress() {
     int progress = 0;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_otaMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         progress = _updateProgress;
         
         // Release mutex
         xSemaphoreGive(_otaMutex);
     }
     
     return progress;
 }
 
 OTAStatus OTAManager::getUpdateStatus() {
     OTAStatus status = OTAStatus::IDLE;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_otaMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         status = _updateStatus;
         
         // Release mutex
         xSemaphoreGive(_otaMutex);
     }
     
     return status;
 }
 
 String OTAManager::getLastError() {
     String error = "";
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_otaMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         error = _lastError;
         
         // Release mutex
         xSemaphoreGive(_otaMutex);
     }
     
     return error;
 }
 
 void OTAManager::onStart() {
     String type;
     if (ArduinoOTA.getCommand() == U_FLASH) {
         type = "firmware";
         _updateStatus = OTAStatus::UPDATING_FIRMWARE;
     } else {
         type = "filesystem";
         _updateStatus = OTAStatus::UPDATING_FILESYSTEM;
     }
     
     // Log the action
     getAppCore()->getLogManager()->log(LogLevel::INFO, "OTA", 
         "OTA update started: " + type);
     
     // Reset progress
     _updateProgress = 0;
     _currentSize = 0;
     _totalSize = 0;
 }
 
 void OTAManager::onEnd() {
     // Log the action
     getAppCore()->getLogManager()->log(LogLevel::INFO, "OTA", "OTA update complete");
     
     // Set status
     _updateStatus = OTAStatus::UPDATE_COMPLETE;
     _updateProgress = 100;
 }
 
 void OTAManager::onProgress(uint progress, uint total) {
     _currentSize = progress;
     _totalSize = total;
     
     // Calculate percentage
     if (total > 0) {
         _updateProgress = (progress * 100) / total;
     }
 }
 
 void OTAManager::onError(ota_error_t error) {
     String errorMsg;
     
     switch (error) {
         case OTA_AUTH_ERROR:
             errorMsg = "Authentication failed";
             break;
         case OTA_BEGIN_ERROR:
             errorMsg = "Begin failed";
             break;
         case OTA_CONNECT_ERROR:
             errorMsg = "Connect failed";
             break;
         case OTA_RECEIVE_ERROR:
             errorMsg = "Receive failed";
             break;
         case OTA_END_ERROR:
             errorMsg = "End failed";
             break;
         default:
             errorMsg = "Unknown error";
     }
     
     // Log the error
     getAppCore()->getLogManager()->log(LogLevel::ERROR, "OTA", 
         "OTA update failed: " + errorMsg);
     
     // Set status and error
     _updateStatus = OTAStatus::UPDATE_FAILED;
     _lastError = errorMsg;
 }
 
 bool OTAManager::beginUpdate(OTAType type, size_t size) {
     // Check if we're already updating
     if (_updateStatus != OTAStatus::IDLE) {
         _lastError = "Update already in progress";
         return false;
     }
     
     // Begin the update process
     if (!Update.begin(size, type == OTAType::FIRMWARE ? U_FLASH : U_SPIFFS)) {
         _lastError = Update.errorString();
         return false;
     }
     
     // Reset counters
     _currentSize = 0;
     _totalSize = size;
     
     // Log the action
     getAppCore()->getLogManager()->log(LogLevel::INFO, "OTA", 
         "Starting " + String(type == OTAType::FIRMWARE ? "firmware" : "filesystem") + 
         " update (" + String(size) + " bytes)");
     
     return true;
 }
 
 bool OTAManager::writeUpdate(uint8_t* data, size_t len) {
     // Check if update is in progress
     if (_updateStatus == OTAStatus::IDLE) {
         _lastError = "No update in progress";
         return false;
     }
     
     // Write data
     size_t written = Update.write(data, len);
     if (written != len) {
         _lastError = Update.errorString();
         return false;
     }
     
     // Update progress
     _currentSize += written;
     if (_totalSize > 0) {
         _updateProgress = (_currentSize * 100) / _totalSize;
     }
     
     return true;
 }
 
 bool OTAManager::endUpdate() {
     // Check if update is in progress
     if (_updateStatus == OTAStatus::IDLE) {
         _lastError = "No update in progress";
         return false;
     }
     
     // Finish the update
     if (!Update.end(true)) {
         _lastError = Update.errorString();
         return false;
     }
     
     // Set progress to 100%
     _updateProgress = 100;
     
     // Log the action
     getAppCore()->getLogManager()->log(LogLevel::INFO, "OTA", "Update completed successfully");
     
     return true;
 }