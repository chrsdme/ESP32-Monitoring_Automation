/**
 * @file PatchedAsyncWebServer.h
 * @brief Modified version of ESPAsyncWebServer.h with fixes for compilation issues
 */

#ifndef PATCHED_ASYNC_WEB_SERVER_H
#define PATCHED_ASYNC_WEB_SERVER_H

// First, define the problematic stuff before including the actual header
struct AcSSLFile {}; // Empty struct definition
typedef std::function<void(void*, AcSSLFile*, const String&, uint32_t, uint32_t, uint32_t, String)> AcSSlFileHandler;

// Now include the real header
#include <ESPAsyncWebServer.h>

#endif // PATCHED_ASYNC_WEB_SERVER_H