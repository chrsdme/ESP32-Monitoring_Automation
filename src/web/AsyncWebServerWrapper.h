/**
 * @file AsyncWebServerWrapper.h
 * @brief Wrapper for ESPAsyncWebServer to fix compilation issues
 */

 #ifndef ASYNC_WEB_SERVER_WRAPPER_H
 #define ASYNC_WEB_SERVER_WRAPPER_H
 
 // Include our patched version instead of the original
 #include "PatchedAsyncWebServer.h"
 
 #endif // ASYNC_WEB_SERVER_WRAPPER_H