/**
 * @file SensorManager.h
 * @brief Manages all sensor operations, including DHT22 and SCD40 sensors
 */

 #ifndef SENSOR_MANAGER_H
 #define SENSOR_MANAGER_H
 
 #include <Arduino.h>
 #include <DHT.h>
 #include <SensirionI2cScd4x.h>
 #include <Wire.h>
 #include <freertos/FreeRTOS.h>
 #include <freertos/task.h>
 #include <freertos/semphr.h>
 #include <freertos/queue.h>
 #include <vector>
 #include "../utils/Constants.h"
 
 // Forward declarations
 class AppCore;
 
 /**
  * @struct SensorReading
  * @brief Structure to hold sensor data
  */
 struct SensorReading {
     float temperature;
     float humidity;
     float co2;
     uint32_t timestamp;
     bool valid;
 };
 
 /**
  * @class SensorManager
  * @brief Manages and coordinates all sensor operations
  */
 class SensorManager {
 public:
     SensorManager();
     ~SensorManager();
     
     /**
      * @brief Initialize the sensor manager
      * @return True if initialized successfully
      */
     bool begin();
     
     /**
      * @brief Perform full initialization of all sensors
      * @return True if all sensors initialized successfully
      */
     bool fullInitialization();
     
     /**
      * @brief Set sensor pins
      * @param dht1Pin Upper DHT22 pin
      * @param dht2Pin Lower DHT22 pin
      * @param scdSdaPin SCD40 SDA pin
      * @param scdSclPin SCD40 SCL pin
      * @return True if pin settings saved successfully
      */
     bool setSensorPins(uint8_t dht1Pin, uint8_t dht2Pin, uint8_t scdSdaPin, uint8_t scdSclPin);
     
     /**
      * @brief Get sensor pins
      * @param dht1Pin Output parameter for upper DHT22 pin
      * @param dht2Pin Output parameter for lower DHT22 pin
      * @param scdSdaPin Output parameter for SCD40 SDA pin
      * @param scdSclPin Output parameter for SCD40 SCL pin
      */
     void getSensorPins(uint8_t& dht1Pin, uint8_t& dht2Pin, uint8_t& scdSdaPin, uint8_t& scdSclPin);
     
     /**
      * @brief Set sensor reading intervals
      * @param dhtInterval DHT22 reading interval in milliseconds
      * @param scdInterval SCD40 reading interval in milliseconds
      */
     void setSensorIntervals(uint32_t dhtInterval, uint32_t scdInterval);
     
     /**
      * @brief Get sensor reading intervals
      * @param dhtInterval Output parameter for DHT22 reading interval
      * @param scdInterval Output parameter for SCD40 reading interval
      */
     void getSensorIntervals(uint32_t& dhtInterval, uint32_t& scdInterval);
     
     /**
      * @brief Get the most recent sensor readings
      * @param upperDht Output parameter for upper DHT22 reading
      * @param lowerDht Output parameter for lower DHT22 reading
      * @param scd Output parameter for SCD40 reading
      * @return True if readings are valid
      */
     bool getSensorReadings(SensorReading& upperDht, SensorReading& lowerDht, SensorReading& scd);
     
     /**
      * @brief Get graph data for a specified time period
      * @param dataType 0 for temperature, 1 for humidity, 2 for CO2
      * @param maxPoints Maximum number of data points to return
      * @return Vector of data points
      */
     std::vector<std::vector<float>> getGraphData(uint8_t dataType, uint16_t maxPoints);
     
     /**
      * @brief Test a specific sensor
      * @param sensorType 0 for upper DHT22, 1 for lower DHT22, 2 for SCD40
      * @return True if sensor test passed
      */
     bool testSensor(uint8_t sensorType);
     
     /**
      * @brief Reset a specific sensor
      * @param sensorType 0 for upper DHT22, 1 for lower DHT22, 2 for SCD40
      * @return True if sensor reset successful
      */
     bool resetSensor(uint8_t sensorType);
     
     /**
      * @brief Create RTOS tasks for sensor operations
      */
     void createTasks();
     
 private:
     // Sensor instances
     DHT _upperDht;
     DHT _lowerDht;
     SensirionI2CScd4x _scd40;
     
     // Sensor pins
     uint8_t _dht1Pin;
     uint8_t _dht2Pin;
     uint8_t _scdSdaPin;
     uint8_t _scdSclPin;
     
     // Sensor reading intervals
     uint32_t _dhtInterval;
     uint32_t _scdInterval;
     
     // Sensor status
     bool _isDht1Initialized;
     bool _isDht2Initialized;
     bool _isScdInitialized;
     
     // Error counters
     uint8_t _dht1ErrorCount;
     uint8_t _dht2ErrorCount;
     uint8_t _scdErrorCount;
     uint8_t _maxErrorCount;
     
     // Latest readings
     SensorReading _upperDhtReading;
     SensorReading _lowerDhtReading;
     SensorReading _scdReading;
     
     // Graph data storage
     std::vector<SensorReading> _upperDhtHistory;
     std::vector<SensorReading> _lowerDhtHistory;
     std::vector<SensorReading> _scdHistory;
     uint16_t _maxHistoryPoints;
     
     // RTOS resources
     SemaphoreHandle_t _sensorMutex;
     TaskHandle_t _dhtTaskHandle;
     TaskHandle_t _scdTaskHandle;
     
     // Private methods
     bool initializeDhtSensors();
     bool initializeScdSensor();
     bool readDhtSensor(DHT& sensor, SensorReading& reading, uint8_t& errorCount, const char* sensorName);
     bool readScdSensor();
     void addReadingToHistory(const SensorReading& reading, std::vector<SensorReading>& history);
     
     // Task functions
     static void dhtReadTask(void* parameter);
     static void scdReadTask(void* parameter);
 };
 
 #endif // SENSOR_MANAGER_H