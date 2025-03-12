/**
 * @file Helpers.h
 * @brief Helper functions for common tasks
 */

 #ifndef HELPERS_H
 #define HELPERS_H
 
 #include <Arduino.h>
 #include <vector>
 
 /**
  * @namespace Helpers
  * @brief Contains utility functions for common tasks
  */
 namespace Helpers {
     /**
      * @brief Convert a byte array to a hexadecimal string
      * @param data Byte array
      * @param len Length of array
      * @param separator Separator character (default: none)
      * @return Hexadecimal string
      */
     String bytesToHex(const uint8_t* data, size_t len, char separator = '\0');
     
     /**
      * @brief Convert a hexadecimal string to a byte array
      * @param hexString Hexadecimal string
      * @param data Output buffer for byte array
      * @param maxLen Maximum length of output buffer
      * @return Number of bytes written to output buffer
      */
     size_t hexToBytes(const String& hexString, uint8_t* data, size_t maxLen);
     
     /**
      * @brief Calculate CRC32 checksum
      * @param data Data buffer
      * @param len Length of data
      * @return CRC32 checksum
      */
     uint32_t calculateCRC32(const uint8_t* data, size_t len);
     
     /**
      * @brief Encode a string to Base64
      * @param input Input string
      * @return Base64 encoded string
      */
     String base64Encode(const String& input);
     
     /**
      * @brief Decode a Base64 string
      * @param input Base64 encoded string
      * @return Decoded string
      */
     String base64Decode(const String& input);
     
     /**
      * @brief URL-encode a string
      * @param input Input string
      * @return URL-encoded string
      */
     String urlEncode(const String& input);
     
     /**
      * @brief URL-decode a string
      * @param input URL-encoded string
      * @return Decoded string
      */
     String urlDecode(const String& input);
     
     /**
      * @brief Calculate MD5 hash
      * @param input Input string
      * @return MD5 hash as a hexadecimal string
      */
     String calculateMD5(const String& input);
     
     /**
      * @brief Calculate SHA256 hash
      * @param input Input string
      * @return SHA256 hash as a hexadecimal string
      */
     String calculateSHA256(const String& input);
     
     /**
      * @brief Encrypt a string using AES-128
      * @param input Input string
      * @param key Encryption key (16 bytes)
      * @param iv Initialization vector (16 bytes)
      * @return Encrypted data as a Base64 string
      */
     String encryptAES128(const String& input, const String& key, const String& iv);
     
     /**
      * @brief Decrypt a string using AES-128
      * @param input Base64 encoded encrypted data
      * @param key Encryption key (16 bytes)
      * @param iv Initialization vector (16 bytes)
      * @return Decrypted string
      */
     String decryptAES128(const String& input, const String& key, const String& iv);
     
     /**
      * @brief Get a random string
      * @param length Length of the random string
      * @param includeSpecialChars Whether to include special characters
      * @return Random string
      */
     String getRandomString(size_t length, bool includeSpecialChars = false);
     
     /**
      * @brief Split a string into an array of substrings
      * @param input Input string
      * @param delimiter Delimiter character
      * @return Vector of substrings
      */
     std::vector<String> splitString(const String& input, char delimiter);
     
     /**
      * @brief Join a vector of strings into a single string
      * @param parts Vector of strings
      * @param delimiter Delimiter string
      * @return Joined string
      */
     String joinStrings(const std::vector<String>& parts, const String& delimiter);
     
     /**
      * @brief Convert string to float with error checking
      * @param input Input string
      * @param defaultValue Default value to return if conversion fails
      * @return Converted float value
      */
     float toFloat(const String& input, float defaultValue = 0.0f);
     
     /**
      * @brief Convert string to integer with error checking
      * @param input Input string
      * @param defaultValue Default value to return if conversion fails
      * @return Converted integer value
      */
     int toInt(const String& input, int defaultValue = 0);
     
     /**
      * @brief Format a float value with specified precision
      * @param value Float value
      * @param precision Number of decimal places
      * @return Formatted string
      */
     String formatFloat(float value, int precision);
     
     /**
      * @brief Format a time duration in milliseconds to a human-readable string
      * @param milliseconds Time duration in milliseconds
      * @return Human-readable duration string
      */
     String formatDuration(uint64_t milliseconds);
     
     /**
      * @brief Format a file size in bytes to a human-readable string
      * @param bytes File size in bytes
      * @return Human-readable size string
      */
     String formatFileSize(size_t bytes);
     
     /**
      * @brief Check if a string starts with a prefix
      * @param str String to check
      * @param prefix Prefix to look for
      * @return True if string starts with prefix
      */
     bool startsWith(const String& str, const String& prefix);
     
     /**
      * @brief Check if a string ends with a suffix
      * @param str String to check
      * @param suffix Suffix to look for
      * @return True if string ends with suffix
      */
     bool endsWith(const String& str, const String& suffix);
     
     /**
      * @brief Trim whitespace from the beginning and end of a string
      * @param str String to trim
      * @return Trimmed string
      */
     String trim(const String& str);
     
     /**
      * @brief Replace all occurrences of a substring in a string
      * @param str Input string
      * @param from Substring to replace
      * @param to Replacement string
      * @return Modified string
      */
     String replaceAll(const String& str, const String& from, const String& to);
     
     /**
      * @brief Get IP address from hostname
      * @param hostname Hostname to resolve
      * @return IP address as string
      */
     String resolveHostname(const String& hostname);
     
     /**
      * @brief Parse a JSON string
      * @param json JSON string
      * @param key Key to extract
      * @param defaultValue Default value if key is not found
      * @return Value of the key
      */
     String parseJsonValue(const String& json, const String& key, const String& defaultValue = "");
 }
 
 #endif // HELPERS_H