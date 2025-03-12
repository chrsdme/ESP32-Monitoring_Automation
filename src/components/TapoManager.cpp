/**
 * @file TapoManager.cpp
 * @brief Implementation of the TapoManager class
 */

 #include "TapoManager.h"
 #include "../core/AppCore.h"
 #include "../system/LogManager.h"
 #include "../utils/Helpers.h"
 #include <SPIFFS.h>
 #include <WiFi.h>
 #include <mbedtls/base64.h>
 #include <mbedtls/md.h>
 #include <mbedtls/aes.h>
 
 TapoManager::TapoManager() :
     _username(""),
     _password(""),
     _token(""),
     _tokenExpiry(0),
     _tapoMutex(nullptr),
     _tapoTaskHandle(nullptr)
 {
 }
 
 TapoManager::~TapoManager() {
     // Clean up RTOS resources
     if (_tapoMutex != nullptr) {
         vSemaphoreDelete(_tapoMutex);
     }
     
     if (_tapoTaskHandle != nullptr) {
         vTaskDelete(_tapoTaskHandle);
     }
 }
 
 bool TapoManager::begin() {
     // Create mutex for thread-safe operations
     _tapoMutex = xSemaphoreCreateMutex();
     if (_tapoMutex == nullptr) {
         Serial.println("Failed to create Tapo mutex!");
         return false;
     }
     
     // Load saved devices
     loadDevices();
     
     // Load credentials from NVS
     nvs_handle_t nvsHandle;
     esp_err_t err = nvs_open(Constants::NVS_CONFIG_NAMESPACE, NVS_READONLY, &nvsHandle);
     if (err == ESP_OK) {
         size_t usernameLen = 0;
         err = nvs_get_str(nvsHandle, "tapo_username", nullptr, &usernameLen);
         if (err == ESP_OK && usernameLen > 0) {
             char* usernameBuf = new char[usernameLen];
             nvs_get_str(nvsHandle, "tapo_username", usernameBuf, &usernameLen);
             _username = String(usernameBuf);
             delete[] usernameBuf;
         }
         
         size_t passwordLen = 0;
         err = nvs_get_str(nvsHandle, "tapo_password", nullptr, &passwordLen);
         if (err == ESP_OK && passwordLen > 0) {
             char* passwordBuf = new char[passwordLen];
             nvs_get_str(nvsHandle, "tapo_password", passwordBuf, &passwordLen);
             _password = String(passwordBuf);
             delete[] passwordBuf;
         }
         
         nvs_close(nvsHandle);
     }
     
     getAppCore()->getLogManager()->log(LogLevel::INFO, "Tapo", 
         "Tapo manager initialized with " + String(_devices.size()) + " devices");
     
     return true;
 }
 
 bool TapoManager::setCredentials(const String& username, const String& password) {
     if (username.isEmpty() || password.isEmpty()) {
         return false;
     }
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_tapoMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         _username = username;
         _password = password;
         _token = "";  // Invalidate token, will be refreshed on next use
         
         // Save credentials to NVS
         nvs_handle_t nvsHandle;
         esp_err_t err = nvs_open(Constants::NVS_CONFIG_NAMESPACE, NVS_READWRITE, &nvsHandle);
         if (err == ESP_OK) {
             nvs_set_str(nvsHandle, "tapo_username", username.c_str());
             nvs_set_str(nvsHandle, "tapo_password", password.c_str());
             nvs_commit(nvsHandle);
             nvs_close(nvsHandle);
         }
         
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Tapo", 
             "Tapo credentials updated for user: " + username);
         
         // Test authentication
         bool authResult = authenticate();
         
         xSemaphoreGive(_tapoMutex);
         
         return authResult;
     }
     
     return false;
 }
 
 bool TapoManager::getCredentials(String& username, String& password) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_tapoMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         username = _username;
         password = _password;
         
         xSemaphoreGive(_tapoMutex);
         return !username.isEmpty() && !password.isEmpty();
     }
     
     return false;
 }
 
 bool TapoManager::addDevice(const TapoDevice& device) {
     if (device.id.isEmpty() || device.ipAddress.isEmpty()) {
         return false;
     }
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_tapoMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Add or update device
         _devices[device.id] = device;
         
         // Save to storage
         bool success = saveDevices();
         
         if (success) {
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Tapo", 
                 "Added Tapo device: " + device.name + " (" + device.id + ")");
         } else {
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "Tapo", 
                 "Failed to save Tapo device: " + device.id);
         }
         
         xSemaphoreGive(_tapoMutex);
         return success;
     }
     
     return false;
 }