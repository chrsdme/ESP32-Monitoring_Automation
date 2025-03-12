/**
 * @file main.cpp
 * @brief Main entry point for the Mushroom Tent Controller application
 */

 #include <Arduino.h>
 #include "core/AppCore.h"
 #include "utils/Constants.h"
 
 // Single global instance of the application core
 AppCore appCore;
 
 extern "C" void app_main() {
     // Initialize Arduino framework
     initArduino();
     
     // Start the application core
     appCore.begin();
     
     // This is not expected to return
     while (true) {
         // The main application logic is handled by RTOS tasks
         vTaskDelay(pdMS_TO_TICKS(1000));
     }
 }
 
 void setup() {
     // Arduino setup - this is called by initArduino() in app_main
     // Most initialization is handled by AppCore::begin()
 }
 
 void loop() {
     // Arduino loop - most work is done in RTOS tasks
     // This is still needed for some Arduino libraries
     // that expect to be called from loop()
     delay(1000);
 }