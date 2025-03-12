/**
 * @file SensorManager.cpp
 * @brief Implementation of the SensorManager class
 */

 #include "SensorManager.h"
 #include "../core/AppCore.h"
 #include "../system/LogManager.h"
 
 SensorManager::SensorManager() :
     _upperDht(Constants::DEFAULT_DHT1_PIN, DHT22),
     _lowerDht(Constants::DEFAULT_DHT2_PIN, DHT22),
     _dht1Pin(Constants::DEFAULT_DHT1_PIN),
     _dht2Pin(Constants::DEFAULT_DHT2_PIN),
     _scdSdaPin(Constants::DEFAULT_SCD40_SDA_PIN),
     _scdSclPin(Constants::DEFAULT_SCD40_SCL_PIN),
     _dhtInterval(Constants::DEFAULT_DHT_READ_INTERVAL_MS),
     _scdInterval(Constants::DEFAULT_SCD40_READ_INTERVAL_MS),
     _isDht1Initialized(false),
     _isDht2Initialized(false),
     _isScdInitialized(false),
     _dht1ErrorCount(0),
     _dht2ErrorCount(0),
     _scdErrorCount(0),
     _maxErrorCount(5),
     _maxHistoryPoints(Constants::DEFAULT_GRAPH_MAX_POINTS),
     _sensorMutex(nullptr),
     _dhtTaskHandle(nullptr),
     _scdTaskHandle(nullptr)
 {
     // Initialize sensor readings with invalid data
     _upperDhtReading = {0.0f, 0.0f, 0.0f, 0, false};
     _lowerDhtReading = {0.0f, 0.0f, 0.0f, 0, false};
     _scdReading = {0.0f, 0.0f, 0.0f, 0, false};
 }
 
 SensorManager::~SensorManager() {
     // Clean up RTOS resources
     if (_sensorMutex != nullptr) {
         vSemaphoreDelete(_sensorMutex);
     }
     
     if (_dhtTaskHandle != nullptr) {
         vTaskDelete(_dhtTaskHandle);
     }
     
     if (_scdTaskHandle != nullptr) {
         vTaskDelete(_scdTaskHandle);
     }
 }
 
 bool SensorManager::begin() {
     // Create mutex for thread-safe operations
     _sensorMutex = xSemaphoreCreateMutex();
     if (_sensorMutex == nullptr) {
         Serial.println("Failed to create sensor mutex!");
         return false;
     }
     
     // Basic initialization here, full initialization will be done in fullInitialization()
     return true;
 }
 
 bool SensorManager::fullInitialization() {
     getAppCore()->getLogManager()->log(LogLevel::INFO, "Sensors", "Starting sensor initialization");
     
     // Initialize DHT sensors
     bool dhtInitialized = initializeDhtSensors();
     
     // Initialize SCD40 sensor
     bool scdInitialized = initializeScdSensor();
     
     if (dhtInitialized && scdInitialized) {
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Sensors", "All sensors initialized successfully");
         return true;
     } else {
         if (!dhtInitialized) {
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "Sensors", "DHT sensors initialization failed");
         }
         if (!scdInitialized) {
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "Sensors", "SCD40 sensor initialization failed");
         }
         return false;
     }
 }
 
 bool SensorManager::setSensorPins(uint8_t dht1Pin, uint8_t dht2Pin, uint8_t scdSdaPin, uint8_t scdSclPin) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_sensorMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Store new pin values
         _dht1Pin = dht1Pin;
         _dht2Pin = dht2Pin;
         _scdSdaPin = scdSdaPin;
         _scdSclPin = scdSclPin;
         
         // Release mutex
         xSemaphoreGive(_sensorMutex);
         
         // Log the change
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Sensors", 
             "Sensor pins updated: DHT1=" + String(dht1Pin) + 
             ", DHT2=" + String(dht2Pin) + 
             ", SCD_SDA=" + String(scdSdaPin) + 
             ", SCD_SCL=" + String(scdSclPin));
         
         // Reinitialize sensors with new pins
         return fullInitialization();
     }
     
     return false;
 }
 
 void SensorManager::getSensorPins(uint8_t& dht1Pin, uint8_t& dht2Pin, uint8_t& scdSdaPin, uint8_t& scdSclPin) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_sensorMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Return current pin values
         dht1Pin = _dht1Pin;
         dht2Pin = _dht2Pin;
         scdSdaPin = _scdSdaPin;
         scdSclPin = _scdSclPin;
         
         // Release mutex
         xSemaphoreGive(_sensorMutex);
     }
 }
 
 void SensorManager::setSensorIntervals(uint32_t dhtInterval, uint32_t scdInterval) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_sensorMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Store new interval values
         _dhtInterval = dhtInterval;
         _scdInterval = scdInterval;
         
         // Release mutex
         xSemaphoreGive(_sensorMutex);
         
         // Log the change
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Sensors", 
             "Sensor intervals updated: DHT=" + String(dhtInterval) + "ms, SCD=" + String(scdInterval) + "ms");
     }
 }
 
 void SensorManager::getSensorIntervals(uint32_t& dhtInterval, uint32_t& scdInterval) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_sensorMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Return current interval values
         dhtInterval = _dhtInterval;
         scdInterval = _scdInterval;
         
         // Release mutex
         xSemaphoreGive(_sensorMutex);
     }
 }
 
 bool SensorManager::getSensorReadings(SensorReading& upperDht, SensorReading& lowerDht, SensorReading& scd) {
     bool hasValidReadings = false;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_sensorMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Copy current readings
         upperDht = _upperDhtReading;
         lowerDht = _lowerDhtReading;
         scd = _scdReading;
         
         // Check if any readings are valid
         hasValidReadings = _upperDhtReading.valid || _lowerDhtReading.valid || _scdReading.valid;
         
         // Release mutex
         xSemaphoreGive(_sensorMutex);
     }
     
     return hasValidReadings;
 }
 
 std::vector<std::vector<float>> SensorManager::getGraphData(uint8_t dataType, uint16_t maxPoints) {
     std::vector<std::vector<float>> result;
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_sensorMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Prepare data vectors
         std::vector<float> upperDhtData;
         std::vector<float> lowerDhtData;
         std::vector<float> scdData;
         std::vector<float> timestamps;
         
         // Calculate how many points to include
         size_t pointCount = min(min(min(_upperDhtHistory.size(), _lowerDhtHistory.size()), _scdHistory.size()), (size_t)maxPoints);
         
         // If we have no data, return empty vectors
         if (pointCount == 0) {
             xSemaphoreGive(_sensorMutex);
             result.push_back(upperDhtData);
             result.push_back(lowerDhtData);
             result.push_back(scdData);
             result.push_back(timestamps);
             return result;
         }
         
         // Get starting index to include only the latest points
         size_t startIdx = 0;
         if (_upperDhtHistory.size() > maxPoints) {
             startIdx = _upperDhtHistory.size() - maxPoints;
         }
         
         // Extract the relevant data
         for (size_t i = startIdx; i < _upperDhtHistory.size(); i++) {
             switch (dataType) {
                 case 0: // Temperature
                     upperDhtData.push_back(_upperDhtHistory[i].temperature);
                     lowerDhtData.push_back(_lowerDhtHistory[i].temperature);
                     scdData.push_back(_scdHistory[i].temperature);
                     break;
                 case 1: // Humidity
                     upperDhtData.push_back(_upperDhtHistory[i].humidity);
                     lowerDhtData.push_back(_lowerDhtHistory[i].humidity);
                     scdData.push_back(_scdHistory[i].humidity);
                     break;
                 case 2: // CO2 (only for SCD40)
                     upperDhtData.push_back(0);
                     lowerDhtData.push_back(0);
                     scdData.push_back(_scdHistory[i].co2);
                     break;
             }
             
             // Add timestamp
             timestamps.push_back(_upperDhtHistory[i].timestamp / 1000.0f); // Convert to seconds
         }
         
         // Release mutex
         xSemaphoreGive(_sensorMutex);
         
         // Add data to result
         result.push_back(upperDhtData);
         result.push_back(lowerDhtData);
         result.push_back(scdData);
         result.push_back(timestamps);
     }
     
     return result;
 }
 
 bool SensorManager::testSensor(uint8_t sensorType) {
     bool testResult = false;
     
     switch (sensorType) {
         case 0: // Upper DHT22
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Sensors", "Testing Upper DHT22 sensor");
             testResult = readDhtSensor(_upperDht, _upperDhtReading, _dht1ErrorCount, "Upper DHT");
             break;
         case 1: // Lower DHT22
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Sensors", "Testing Lower DHT22 sensor");
             testResult = readDhtSensor(_lowerDht, _lowerDhtReading, _dht2ErrorCount, "Lower DHT");
             break;
         case 2: // SCD40
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Sensors", "Testing SCD40 sensor");
             testResult = readScdSensor();
             break;
         default:
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "Sensors", "Invalid sensor type for testing");
             return false;
     }
     
     if (testResult) {
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Sensors", "Sensor test passed");
     } else {
         getAppCore()->getLogManager()->log(LogLevel::ERROR, "Sensors", "Sensor test failed");
     }
     
     return testResult;
 }
 
 bool SensorManager::resetSensor(uint8_t sensorType) {
     bool resetResult = false;
     
     switch (sensorType) {
         case 0: // Upper DHT22
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Sensors", "Resetting Upper DHT22 sensor");
             _upperDht = DHT(_dht1Pin, DHT22);
             _upperDht.begin();
             _dht1ErrorCount = 0;
             resetResult = true;
             break;
         case 1: // Lower DHT22
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Sensors", "Resetting Lower DHT22 sensor");
             _lowerDht = DHT(_dht2Pin, DHT22);
             _lowerDht.begin();
             _dht2ErrorCount = 0;
             resetResult = true;
             break;
         case 2: // SCD40
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Sensors", "Resetting SCD40 sensor");
             Wire.end();
             delay(100);
             Wire.begin(_scdSdaPin, _scdSclPin);
             _scd40.begin(Wire);
             _scd40.stopPeriodicMeasurement();
             delay(500);
             _scd40.startPeriodicMeasurement();
             _scdErrorCount = 0;
             resetResult = true;
             break;
         default:
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "Sensors", "Invalid sensor type for reset");
             return false;
     }
     
     if (resetResult) {
         getAppCore()->getLogManager()->log(LogLevel::INFO, "Sensors", "Sensor reset successful");
     } else {
         getAppCore()->getLogManager()->log(LogLevel::ERROR, "Sensors", "Sensor reset failed");
     }
     
     return resetResult;
 }
 
 void SensorManager::createTasks() {
     // Create DHT reading task
     BaseType_t result = xTaskCreatePinnedToCore(
         dhtReadTask,                  // Task function
         "DHTReadTask",                // Task name
         Constants::STACK_SIZE_SENSORS,// Stack size (words)
         this,                         // Task parameters
         Constants::PRIORITY_SENSORS,  // Priority
         &_dhtTaskHandle,              // Task handle
         1                             // Core ID (1 - application core)
     );
     
     if (result != pdPASS) {
         getAppCore()->getLogManager()->log(LogLevel::ERROR, "Sensors", "Failed to create DHT reading task");
     }
     
     // Create SCD reading task
     result = xTaskCreatePinnedToCore(
         scdReadTask,                  // Task function
         "SCDReadTask",                // Task name
         Constants::STACK_SIZE_SENSORS,// Stack size (words)
         this,                         // Task parameters
         Constants::PRIORITY_SENSORS,  // Priority
         &_scdTaskHandle,              // Task handle
         1                             // Core ID (1 - application core)
     );
     
     if (result != pdPASS) {
         getAppCore()->getLogManager()->log(LogLevel::ERROR, "Sensors", "Failed to create SCD reading task");
     }
 }
 
 bool SensorManager::initializeDhtSensors() {
     getAppCore()->getLogManager()->log(LogLevel::INFO, "Sensors", "Initializing DHT sensors");
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_sensorMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Initialize upper DHT sensor
         _upperDht = DHT(_dht1Pin, DHT22);
         _upperDht.begin();
         _isDht1Initialized = true;
         
         // Initialize lower DHT sensor
         _lowerDht = DHT(_dht2Pin, DHT22);
         _lowerDht.begin();
         _isDht2Initialized = true;
         
         // Reset error counters
         _dht1ErrorCount = 0;
         _dht2ErrorCount = 0;
         
         // Release mutex
         xSemaphoreGive(_sensorMutex);
         
         // Wait 2 seconds for DHT sensors to stabilize
         vTaskDelay(pdMS_TO_TICKS(2000));
         
         // Try to read from sensors to verify they're working
         SensorReading upperReading, lowerReading;
         bool upperReadSuccess = readDhtSensor(_upperDht, upperReading, _dht1ErrorCount, "Upper DHT");
         bool lowerReadSuccess = readDhtSensor(_lowerDht, lowerReading, _dht2ErrorCount, "Lower DHT");
         
         if (upperReadSuccess) {
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Sensors", 
                 "Upper DHT initialized: Temp=" + String(upperReading.temperature, 1) + "°C, Humidity=" + String(upperReading.humidity, 1) + "%");
         } else {
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "Sensors", "Upper DHT initialization failed");
             _isDht1Initialized = false;
         }
         
         if (lowerReadSuccess) {
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Sensors", 
                 "Lower DHT initialized: Temp=" + String(lowerReading.temperature, 1) + "°C, Humidity=" + String(lowerReading.humidity, 1) + "%");
         } else {
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "Sensors", "Lower DHT initialization failed");
             _isDht2Initialized = false;
         }
         
         return _isDht1Initialized || _isDht2Initialized;
     }
     
     return false;
 }
 
 bool SensorManager::initializeScdSensor() {
     getAppCore()->getLogManager()->log(LogLevel::INFO, "Sensors", "Initializing SCD40 sensor");
     
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_sensorMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Initialize I2C
         Wire.begin(_scdSdaPin, _scdSclPin);
         
         // Initialize SCD40 sensor
         _scd40.begin(Wire);
         
         // Stop any existing measurement
         _scd40.stopPeriodicMeasurement();
         vTaskDelay(pdMS_TO_TICKS(500));
         
         // Start periodic measurement
         uint16_t error = _scd40.startPeriodicMeasurement();
         if (error) {
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "Sensors", 
                 "SCD40 start measurement failed with error: " + String(error));
             _isScdInitialized = false;
         } else {
             _isScdInitialized = true;
             _scdErrorCount = 0;
         }
         
         // Release mutex
         xSemaphoreGive(_sensorMutex);
         
         // Wait 5 seconds for the SCD40 to take first measurement
         vTaskDelay(pdMS_TO_TICKS(5000));
         
         // Try to read from sensor to verify it's working
         if (readScdSensor()) {
             getAppCore()->getLogManager()->log(LogLevel::INFO, "Sensors", 
                 "SCD40 initialized: Temp=" + String(_scdReading.temperature, 1) + 
                 "°C, Humidity=" + String(_scdReading.humidity, 1) + 
                 "%, CO2=" + String(_scdReading.co2, 0) + "ppm");
             return true;
         } else {
             getAppCore()->getLogManager()->log(LogLevel::ERROR, "Sensors", "SCD40 initialization failed");
             _isScdInitialized = false;
             return false;
         }
     }
     
     return false;
 }
 
 bool SensorManager::readDhtSensor(DHT& sensor, SensorReading& reading, uint8_t& errorCount, const char* sensorName) {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_sensorMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Read temperature and humidity
         float temperature = sensor.readTemperature();
         float humidity = sensor.readHumidity();
         
         // Check if readings are valid
         if (isnan(temperature) || isnan(humidity)) {
             errorCount++;
             if (errorCount > _maxErrorCount) {
                 getAppCore()->getLogManager()->log(LogLevel::ERROR, "Sensors", 
                     String(sensorName) + " read failed too many times, resetting sensor");
                 // Release mutex before reset to avoid deadlock
                 xSemaphoreGive(_sensorMutex);
                 // Determine sensor type from name and reset
                 if (strcmp(sensorName, "Upper DHT") == 0) {
                     resetSensor(0);
                 } else if (strcmp(sensorName, "Lower DHT") == 0) {
                     resetSensor(1);
                 }
                 return false;
             }
             
             getAppCore()->getLogManager()->log(LogLevel::WARN, "Sensors", 
                 String(sensorName) + " read failed, error count: " + String(errorCount));
             
             // Release mutex
             xSemaphoreGive(_sensorMutex);
             return false;
         }
         
         // Update reading
         reading.temperature = temperature;
         reading.humidity = humidity;
         reading.timestamp = millis();
         reading.valid = true;
         
         // Reset error counter on successful read
         errorCount = 0;
         
         // Add to history if this is not a test read
         if (strcmp(sensorName, "Upper DHT") == 0) {
             addReadingToHistory(reading, _upperDhtHistory);
         } else if (strcmp(sensorName, "Lower DHT") == 0) {
             addReadingToHistory(reading, _lowerDhtHistory);
         }
         
         // Release mutex
         xSemaphoreGive(_sensorMutex);
         return true;
     }
     
     return false;
 }
 
 bool SensorManager::readScdSensor() {
     // Take mutex to ensure thread safety
     if (xSemaphoreTake(_sensorMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
         // Check if data is available
         uint16_t error;
         bool dataReady;
         error = _scd40.getDataReadyFlag(dataReady);
         
         if (error || !dataReady) {
             _scdErrorCount++;
             if (_scdErrorCount > _maxErrorCount) {
                 getAppCore()->getLogManager()->log(LogLevel::ERROR, "Sensors", 
                     "SCD40 read failed too many times, resetting sensor");
                 // Release mutex before reset to avoid deadlock
                 xSemaphoreGive(_sensorMutex);
                 resetSensor(2);
                 return false;
             }
             
             getAppCore()->getLogManager()->log(LogLevel::WARN, "Sensors", 
                 "SCD40 data not ready or error: " + String(error) + ", error count: " + String(_scdErrorCount));
             
             // Release mutex
             xSemaphoreGive(_sensorMutex);
             return false;
         }
         
         // Read measurement
         uint16_t co2;
         float temperature;
         float humidity;
         error = _scd40.readMeasurement(co2, temperature, humidity);
         
         if (error) {
             _scdErrorCount++;
             getAppCore()->getLogManager()->log(LogLevel::WARN, "Sensors", 
                 "SCD40 read failed with error: " + String(error) + ", error count: " + String(_scdErrorCount));
             
             // Release mutex
             xSemaphoreGive(_sensorMutex);
             return false;
         }
         
         // Update reading
         _scdReading.temperature = temperature;
         _scdReading.humidity = humidity;
         _scdReading.co2 = co2;
         _scdReading.timestamp = millis();
         _scdReading.valid = true;
         
         // Reset error counter on successful read
         _scdErrorCount = 0;
         
         // Add to history
         addReadingToHistory(_scdReading, _scdHistory);
         
         // Release mutex
         xSemaphoreGive(_sensorMutex);
         return true;
     }
     
     return false;
 }
 
 void SensorManager::addReadingToHistory(const SensorReading& reading, std::vector<SensorReading>& history) {
     // Add reading to history
     history.push_back(reading);
     
     // Limit history size
     if (history.size() > _maxHistoryPoints) {
         history.erase(history.begin());
     }
 }
 
 void SensorManager::dhtReadTask(void* parameter) {
     SensorManager* sensorManager = static_cast<SensorManager*>(parameter);
     TickType_t lastWakeTime = xTaskGetTickCount();
     
     while (true) {
         // Read upper DHT sensor
         if (sensorManager->_isDht1Initialized) {
             sensorManager->readDhtSensor(sensorManager->_upperDht, sensorManager->_upperDhtReading, 
                                          sensorManager->_dht1ErrorCount, "Upper DHT");
         }
         
         // Small delay between readings to avoid overwhelming the bus
         vTaskDelay(pdMS_TO_TICKS(100));
         
         // Read lower DHT sensor
         if (sensorManager->_isDht2Initialized) {
             sensorManager->readDhtSensor(sensorManager->_lowerDht, sensorManager->_lowerDhtReading, 
                                          sensorManager->_dht2ErrorCount, "Lower DHT");
         }
         
         // Wait for the next reading period
         vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(sensorManager->_dhtInterval));
     }
 }
 
 void SensorManager::scdReadTask(void* parameter) {
     SensorManager* sensorManager = static_cast<SensorManager*>(parameter);
     TickType_t lastWakeTime = xTaskGetTickCount();
     
     // Initial delay to stagger readings
     vTaskDelay(pdMS_TO_TICKS(1000));
     
     while (true) {
         // Read SCD40 sensor
         if (sensorManager->_isScdInitialized) {
             sensorManager->readScdSensor();
         }
         
         // Wait for the next reading period
         vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(sensorManager->_scdInterval));
     }
 }