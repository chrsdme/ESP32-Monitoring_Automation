/**
 * @file VersionManager.h
 * @brief Manages application version information and updates
 */

 #ifndef VERSION_MANAGER_H
 #define VERSION_MANAGER_H
 
 #include <Arduino.h>
 #include <HTTPClient.h>
 #include "../utils/Constants.h"
 
 /**
  * @class VersionManager
  * @brief Manages version information for the application and filesystem
  */
 class VersionManager {
 public:
     VersionManager();
     ~VersionManager();
 
     /**
      * @brief Check for available updates
      * @param currentVersion Current version to compare
      * @return Update information or empty string if no update available
      */
     String checkForUpdates(const String& currentVersion);
 
     /**
      * @brief Get current application version
      * @return Version string
      */
     String getCurrentVersion() const;
 
     /**
      * @brief Get current filesystem version
      * @return Version string
      */
     String getCurrentFsVersion() const;
 
     /**
      * @brief Perform version update if available
      * @return True if update successful, false otherwise
      */
     bool performUpdate();
 
     /**
      * @brief Get major version
      * @return Major version number
      */
     uint16_t getMajorVersion() const;
 
     /**
      * @brief Get minor version
      * @return Minor version number
      */
     uint16_t getMinorVersion() const;
 
     /**
      * @brief Get patch version
      * @return Patch version number
      */
     uint16_t getPatchVersion() const;
 
     /**
      * @brief Get build version
      * @return Build version number
      */
     uint16_t getBuildVersion() const;
 
     /**
      * @brief Get version timestamp
      * @return Version timestamp
      */
     uint32_t getVersionTimestamp() const;
 
 private:
     // Version parsing and storage
     uint16_t _majorVersion;
     uint16_t _minorVersion;
     uint16_t _patchVersion;
     uint16_t _buildVersion;
     uint32_t _versionTimestamp;
 
     // Filesystem version components
     uint16_t _fsMajorVersion;
     uint16_t _fsMinorVersion;
     uint16_t _fsPatchVersion;
     uint16_t _fsBuildVersion;
     uint32_t _fsVersionTimestamp;
 
     // HTTP client for update checks
     HTTPClient _httpClient;
 
     /**
      * @brief Parse version string
      * @param version Version string to parse
      * @param major Output major version
      * @param minor Output minor version
      * @param patch Output patch version
      * @param build Output build version
      * @param timestamp Output version timestamp
      */
     void parseVersion(const String& version, 
                       uint16_t& major, 
                       uint16_t& minor, 
                       uint16_t& patch, 
                       uint16_t& build, 
                       uint32_t& timestamp);
 
     /**
      * @brief Download update file
      * @param url URL of update file
      * @return True if download successful, false otherwise
      */
     bool downloadUpdateFile(const String& url);
 
     /**
      * @brief Validate downloaded update
      * @return True if update is valid, false otherwise
      */
     bool validateUpdate();
 };
 
 #endif // VERSION_MANAGER_H