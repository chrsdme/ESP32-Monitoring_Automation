/**
 * @file Helpers.cpp
 * @brief Implementation of helper functions
 */

 #include "Helpers.h"
 #include <mbedtls/md.h>
 #include <mbedtls/base64.h>
 #include <mbedtls/aes.h>
 #include <WiFi.h>
 #include <ArduinoJson.h>
 
 namespace Helpers {
 
 String bytesToHex(const uint8_t* data, size_t len, char separator) {
     String result;
     for (size_t i = 0; i < len; i++) {
         char hex[3];
         sprintf(hex, "%02X", data[i]);
         result += hex;
         if (separator != '\0' && i < len - 1) {
             result += separator;
         }
     }
     return result;
 }
 
 size_t hexToBytes(const String& hexString, uint8_t* data, size_t maxLen) {
     String hexCopy = hexString;
     
     // Remove separator characters if present
     hexCopy.replace(":", "");
     hexCopy.replace("-", "");
     hexCopy.replace(" ", "");
     
     // Check if string is valid hex
     for (size_t i = 0; i < hexCopy.length(); i++) {
         if (!isxdigit(hexCopy.charAt(i))) {
             return 0;
         }
     }
     
     // Make sure string length is even
     if (hexCopy.length() % 2 != 0) {
         return 0;
     }
     
     // Calculate number of bytes
     size_t numBytes = hexCopy.length() / 2;
     
     // Check if buffer is large enough
     if (numBytes > maxLen) {
         numBytes = maxLen;
     }
     
     // Convert hex to bytes
     for (size_t i = 0; i < numBytes; i++) {
         char byteStr[3] = { hexCopy.charAt(i*2), hexCopy.charAt(i*2+1), '\0' };
         data[i] = strtol(byteStr, NULL, 16);
     }
     
     return numBytes;
 }
 
 uint32_t calculateCRC32(const uint8_t* data, size_t len) {
     uint32_t crc = 0xFFFFFFFF;
     
     for (size_t i = 0; i < len; i++) {
         uint8_t byte = data[i];
         crc ^= byte;
         
         for (int j = 0; j < 8; j++) {
             uint32_t mask = -(crc & 1);
             crc = (crc >> 1) ^ (0xEDB88320 & mask);
         }
     }
     
     return ~crc;
 }
 
 String base64Encode(const String& input) {
     size_t inputLen = input.length();
     size_t outputLen = 0;
     
     // Calculate required output buffer size
     mbedtls_base64_encode(NULL, 0, &outputLen, (const unsigned char*)input.c_str(), inputLen);
     
     // Allocate buffer for encoded data
     unsigned char* buffer = new unsigned char[outputLen + 1];
     
     // Encode data
     mbedtls_base64_encode(buffer, outputLen, &outputLen, (const unsigned char*)input.c_str(), inputLen);
     buffer[outputLen] = '\0';
     
     // Create string from buffer
     String result = (char*)buffer;
     
     // Clean up
     delete[] buffer;
     
     return result;
 }
 
 String base64Decode(const String& input) {
     size_t inputLen = input.length();
     size_t outputLen = 0;
     
     // Calculate required output buffer size
     mbedtls_base64_decode(NULL, 0, &outputLen, (const unsigned char*)input.c_str(), inputLen);
     
     // Allocate buffer for decoded data
     unsigned char* buffer = new unsigned char[outputLen + 1];
     
     // Decode data
     mbedtls_base64_decode(buffer, outputLen, &outputLen, (const unsigned char*)input.c_str(), inputLen);
     buffer[outputLen] = '\0';
     
     // Create string from buffer
     String result = (char*)buffer;
     
     // Clean up
     delete[] buffer;
     
     return result;
 }
 } // End of namespace Helpers