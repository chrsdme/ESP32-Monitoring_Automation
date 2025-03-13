#ifndef ESP_ASYNC_WEBSERVER_FIX_H
#define ESP_ASYNC_WEBSERVER_FIX_H

// Forward declarations for missing types
struct AcSSLFile;
typedef std::function<void(void*, AcSSLFile*, const String&, uint32_t, uint32_t, uint32_t, String)> AcSSlFileHandler;

#endif