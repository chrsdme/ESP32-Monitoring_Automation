/**
 * @file AsyncWebServerWrapper.h
 * @brief Wrapper for ESPAsyncWebServer to fix compilation issues
 */

#ifndef ASYNC_WEB_SERVER_WRAPPER_H
#define ASYNC_WEB_SERVER_WRAPPER_H

// Fix for undefined AcSSlFileHandler issue
#define AcSSlFileHandler void*

// Include the actual ESPAsyncWebServer header
#include <ESPAsyncWebServer.h>

#endif // ASYNC_WEB_SERVER_WRAPPER_H