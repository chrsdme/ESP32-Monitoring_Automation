/**
 * @file SecurityManager.cpp
 * @brief Implementation of the SecurityManager class
 */

 #include "SecurityManager.h"
 #include "../core/AppCore.h"
 
 SecurityManager::SecurityManager() :
     _securityMutex(nullptr),
     _isInitialized(false),
     _otaPassword("")
 {
 }
 
 SecurityManager::~SecurityManager() {
     // Clean up RTOS resources
     if (_securityMutex != nullptr) {
         vSemaphoreDelete(_securityMutex);
     }
 }
 
 bool SecurityManager::begin() {
     // Create mutex for thread-safe operations
     _securityMutex = xSemaphoreCreateMutex();
     if (_securityMutex == nullptr) {
         Serial.println("Failed to create security mutex!");
         return false;
     }
     
     // Load stored credentials
     _otaPassword = loadFromNvs("ota_pass");
     
     _isInitialized = true;
     return true;
 }
 
 bool SecurityManager::setHttpCredentials(const String& username, const String& password) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_securityMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Hash the password for secure storage
         String hashedPassword = hashPassword(password);
         
         // Save to NVS
         bool success = saveToNvs(Constants::NVS_HTTP_USER_KEY, username) &&
                       saveToNvs(Constants::NVS_HTTP_PASS_KEY, hashedPassword);
         
         // Release mutex
         xSemaphoreGive(_securityMutex);
         
         if (success) {
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Security", "HTTP credentials updated");
         } else {
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "Security", "Failed to update HTTP credentials");
         }
         
         return success;
     }
     
     return false;
 }
 
 bool SecurityManager::getHttpCredentials(String& username, String& password) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_securityMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         username = loadFromNvs(Constants::NVS_HTTP_USER_KEY);
         password = loadFromNvs(Constants::NVS_HTTP_PASS_KEY);
         
         // Release mutex
         xSemaphoreGive(_securityMutex);
         
         return !username.isEmpty() && !password.isEmpty();
     }
     
     return false;
 }
 
 bool SecurityManager::setOtaPassword(const String& password) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_securityMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Store password (consider hashing it for secure storage)
         _otaPassword = password;
         
         // Save to NVS
         bool success = saveToNvs("ota_pass", password);
         
         // Release mutex
         xSemaphoreGive(_securityMutex);
         
         if (success) {
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Security", "OTA password updated");
         } else {
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "Security", "Failed to update OTA password");
         }
         
         return success;
     }
     
     return false;
 }
 
 String SecurityManager::getOtaPassword() {
     String password = "";
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_securityMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         password = _otaPassword;
         
         // Release mutex
         xSemaphoreGive(_securityMutex);
     }
     
     return password;
 }
 
 bool SecurityManager::validatePassword(const String& password, const String& storedHash) {
     // Calculate hash of the provided password
     String calculatedHash = hashPassword(password);
     
     // Compare with stored hash
     return calculatedHash.equals(storedHash);
 }
 
 String SecurityManager::hashPassword(const String& password) {
     // Use SHA-256 for password hashing
     uint8_t hash[32];
     mbedtls_sha256_context ctx;
     
     mbedtls_sha256_init(&ctx);
     mbedtls_sha256_starts_ret(&ctx, 0); // 0 for SHA-256, 1 for SHA-224
     mbedtls_sha256_update_ret(&ctx, (const unsigned char*)password.c_str(), password.length());
     mbedtls_sha256_finish_ret(&ctx, hash);
     
     // Convert hash to base64 string for storage
     return base64Encode(hash, sizeof(hash));
 }
 
 String SecurityManager::generateRandomToken(size_t length) {
     static const char* charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_";
     String token = "";
     
     for (size_t i = 0; i < length; i++) {
         uint8_t randomValue = esp_random() % 64; // 64 is the size of the charset
         token += charset[randomValue];
     }
     
     return token;
 }
 
 String SecurityManager::encrypt(const String& data, const String& key) {
     // Simple XOR encryption for demonstration purposes
     // In a real application, use a proper encryption algorithm like AES
     
     uint8_t* encrypted = new uint8_t[data.length()];
     
     for (size_t i = 0; i < data.length(); i++) {
         encrypted[i] = data[i] ^ key[i % key.length()];
     }
     
     String result = base64Encode(encrypted, data.length());
     
     delete[] encrypted;
     
     return result;
 }
 
 String SecurityManager::decrypt(const String& data, const String& key) {
     // Simple XOR decryption for demonstration purposes
     // In a real application, use a proper encryption algorithm like AES
     
     size_t decodedLength = data.length() * 3 / 4; // Approximate base64 decoded length
     uint8_t* decoded = new uint8_t[decodedLength];
     
     if (!base64Decode(data, decoded, &decodedLength)) {
         delete[] decoded;
         return "";
     }
     
     char* decrypted = new char[decodedLength + 1];
     
     for (size_t i = 0; i < decodedLength; i++) {
         decrypted[i] = decoded[i] ^ key[i % key.length()];
     }
     
     decrypted[decodedLength] = '\0';
     String result = String(decrypted);
     
     delete[] decoded;
     delete[] decrypted;
     
     return result;
 }
 
 bool SecurityManager::saveToNvs(const char* key, const String& value) {
     nvs_handle_t nvsHandle;
     esp_err_t err = nvs_open(Constants::NVS_CONFIG_NAMESPACE, NVS_READWRITE, &nvsHandle);
     
     if (err != ESP_OK) {
         return false;
     }
     
     err = nvs_set_str(nvsHandle, key, value.c_str());
     
     if (err != ESP_OK) {
         nvs_close(nvsHandle);
         return false;
     }
     
     err = nvs_commit(nvsHandle);
     nvs_close(nvsHandle);
     
     return (err == ESP_OK);
 }
 
 String SecurityManager::loadFromNvs(const char* key) {
     nvs_handle_t nvsHandle;
     esp_err_t err = nvs_open(Constants::NVS_CONFIG_NAMESPACE, NVS_READONLY, &nvsHandle);
     
     if (err != ESP_OK) {
         return "";
     }
     
     size_t required_size;
     err = nvs_get_str(nvsHandle, key, nullptr, &required_size);
     
     if (err != ESP_OK) {
         nvs_close(nvsHandle);
         return "";
     }
     
     char* buffer = new char[required_size];
     err = nvs_get_str(nvsHandle, key, buffer, &required_size);
     
     String result = "";
     if (err == ESP_OK) {
         result = String(buffer);
     }
     
     delete[] buffer;
     nvs_close(nvsHandle);
     
     return result;
 }
 
 String SecurityManager::base64Encode(const uint8_t* data, size_t length) {
     size_t outputLength;
     mbedtls_base64_encode(nullptr, 0, &outputLength, data, length);
     
     uint8_t* buffer = new uint8_t[outputLength + 1];
     mbedtls_base64_encode(buffer, outputLength, &outputLength, data, length);
     buffer[outputLength] = '\0';
     
     String result = String((char*)buffer);
     delete[] buffer;
     
     return result;
 }
 
 bool SecurityManager::base64Decode(const String& input, uint8_t* output, size_t* outputLength) {
     int ret = mbedtls_base64_decode(output, *outputLength, outputLength, 
                                    (const unsigned char*)input.c_str(), input.length());
     
     return (ret == 0);
 }