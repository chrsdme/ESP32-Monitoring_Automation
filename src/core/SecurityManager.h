/**
 * @file SecurityManager.h
 * @brief Manages security features including authentication and encryption
 */

 #ifndef SECURITY_MANAGER_H
 #define SECURITY_MANAGER_H
 
 #include <Arduino.h>
 #include <freertos/FreeRTOS.h>
 #include <freertos/semphr.h>
 #include <nvs.h>
 #include <nvs_flash.h>
 #include <mbedtls/md.h>
 #include <mbedtls/sha256.h>
 #include <mbedtls/base64.h>
 #include "../utils/Constants.h"
 
 /**
  * @class SecurityManager
  * @brief Manages security-related functions including authentication, encryption, and key management
  */
 class SecurityManager {
 public:
     SecurityManager();
     ~SecurityManager();
     
     /**
      * @brief Initialize the security manager
      * @return True if initialized successfully
      */
     bool begin();
     
     /**
      * @brief Set HTTP authentication credentials
      * @param username Username
      * @param password Password
      * @return True if credentials set successfully
      */
     bool setHttpCredentials(const String& username, const String& password);
     
     /**
      * @brief Get HTTP authentication credentials
      * @param username Output parameter for username
      * @param password Output parameter for password
      * @return True if credentials retrieved successfully
      */
     bool getHttpCredentials(String& username, String& password);
     
     /**
      * @brief Set OTA update password
      * @param password OTA password
      * @return True if password set successfully
      */
     bool setOtaPassword(const String& password);
     
     /**
      * @brief Get OTA update password
      * @return OTA password
      */
     String getOtaPassword();
     
     /**
      * @brief Validate password against stored hash
      * @param password Password to validate
      * @param storedHash Stored password hash
      * @return True if password is valid
      */
     bool validatePassword(const String& password, const String& storedHash);
     
     /**
      * @brief Generate a password hash
      * @param password Password to hash
      * @return Hashed password
      */
     String hashPassword(const String& password);
     
     /**
      * @brief Generate a random token
      * @param length Token length
      * @return Random token
      */
     String generateRandomToken(size_t length = 32);
     
     /**
      * @brief Encrypt a string
      * @param data String to encrypt
      * @param key Encryption key
      * @return Encrypted string (Base64 encoded)
      */
     String encrypt(const String& data, const String& key);
     
     /**
      * @brief Decrypt a string
      * @param data Encrypted string (Base64 encoded)
      * @param key Encryption key
      * @return Decrypted string
      */
     String decrypt(const String& data, const String& key);
     
 private:
     // RTOS resources
     SemaphoreHandle_t _securityMutex;
     
     // Internal state
     bool _isInitialized;
     String _otaPassword;
     
     // Helper methods
     bool saveToNvs(const char* key, const String& value);
     String loadFromNvs(const char* key);
     String base64Encode(const uint8_t* data, size_t length);
     bool base64Decode(const String& input, uint8_t* output, size_t* outputLength);
 };
 
 #endif // SECURITY_MANAGER_H