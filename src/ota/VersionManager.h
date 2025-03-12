/**
 * @file VersionManager.h
 * @brief Manages firmware and filesystem versioning
 */

 #ifndef VERSION_MANAGER_H
 #define VERSION_MANAGER_H
 
 #include <Arduino.h>
 #include <freertos/FreeRTOS.h>
 #include <freertos/semphr.h>
 #include <SPIFFS.h>
 #include "../utils/Constants.h"
 
 // Forward declarations
 class AppCore;
 
 /**
  * @struct Version
  * @brief Structure to hold version information
  */
 struct Version {
     uint8_t major;
     uint8_t minor;
     uint8_t patch;
     String buildId;
     uint32_t timestamp;
     
     Version() : major(0), minor(0), patch(0), buildId(""), timestamp(0) {}
     
     Version(uint8_t major, uint8_t minor, uint8_t patch) 
         : major(major), minor(minor), patch(patch), buildId(""), timestamp(0) {}
     
     Version(uint8_t major, uint8_t minor, uint8_t patch, const String& buildId, uint32_t timestamp) 
         : major(major), minor(minor), patch(patch), buildId(buildId), timestamp(timestamp) {}
     
     String toString() const {
         return String(major) + "." + String(minor) + "." + String(patch) + 
                (buildId.isEmpty() ? "" : "-" + buildId);
     }
     
     bool isNewerThan(const Version& other) const {
         if (major > other.major) return true;
         if (major < other.major) return false;
         
         if (minor > other.minor) return true;
         if (minor < other.minor) return false;
         
         if (patch > other.patch) return true;
         return false;
     }
     
     static Version fromString(const String& versionStr) {
         Version version;
         
         // Parse basic version (major.minor.patch)
         int firstDot = versionStr.indexOf('.');
         if (firstDot > 0) {
             version.major = versionStr.substring(0, firstDot).toInt();
             
             int secondDot = versionStr.indexOf('.', firstDot + 1);
             if (secondDot > 0) {
                 version.minor = versionStr.substring(firstDot + 1, secondDot).toInt();
                 
                 // Check for build ID or just patch
                 int dashPos = versionStr.indexOf('-', secondDot + 1);
                 if (dashPos > 0) {
                     version.patch = versionStr.substring(secondDot + 1, dashPos).toInt();
                     version.buildId = versionStr.substring(dashPos + 1);
                 } else {
                     version.patch = versionStr.substring(secondDot + 1).toInt();
                 }
             }
         }
         
         return version;
     }
 };
 
 /**
  * @class VersionManager
  * @brief Manages firmware and filesystem versioning and update checks
  */
 class VersionManager {
 public:
     VersionManager();
     ~VersionManager();
     
     /**
      * @brief Initialize the version manager
      * @return True if initialized successfully
      */
     bool begin();
     
     /**
      * @brief Get current firmware version
      * @return Current firmware version
      */
     Version getFirmwareVersion();
     
     /**
      * @brief Get current filesystem version
      * @return Current filesystem version
      */
     Version getFilesystemVersion();
     
     /**
      * @brief Set firmware version
      * @param version Version to set
      * @return True if version set successfully
      */
     bool setFirmwareVersion(const Version& version);
     
     /**
      * @brief Set filesystem version
      * @param version Version to set
      * @return True if version set successfully
      */
     bool setFilesystemVersion(const Version& version);
     
     /**
      * @brief Check if a new version is available
      * @param checkUrl URL to check for updates
      * @return Update information JSON as a string
      */
     String checkForUpdates(const String& checkUrl = "");
     
     /**
      * @brief Get changelog between current version and target version
      * @param targetVersion Target version
      * @return Changelog as a string
      */
     String getChangelog(const Version& targetVersion);
     
     /**
      * @brief Store version history
      * @param version Version that was installed
      * @param type 0 for firmware, 1 for filesystem
      * @param status Installation status
      * @return True if history stored successfully
      */
     bool storeVersionHistory(const Version& version, uint8_t type, bool status);
     
     /**
      * @brief Get version history
      * @return Version history as JSON string
      */
     String getVersionHistory();
     
 private:
     // Current versions
     Version _firmwareVersion;
     Version _filesystemVersion;
     
     // RTOS resources
     SemaphoreHandle_t _versionMutex;
     
     // Helper methods
     bool loadVersions();
     bool saveVersions();
     String getVersionFile();
     bool parseVersionFile(const String& content);
 };
 
 #endif // VERSION_MANAGER_H