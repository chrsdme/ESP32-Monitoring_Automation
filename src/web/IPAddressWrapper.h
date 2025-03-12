/**
 * @file IPAddressWrapper.h
 * @brief Wrapper for IP address handling with conditional INADDR_NONE definition
 */

 #ifndef IP_ADDRESS_WRAPPER_H
 #define IP_ADDRESS_WRAPPER_H
 
 #include <Arduino.h>
 #include <IPAddress.h>
 
 // Check if IPADDR_NONE is already defined (from lwip)
 #ifndef IPADDR_NONE
   // Only define INADDR_NONE if it's not already defined
   #ifndef INADDR_NONE
     #define INADDR_NONE ((uint32_t)0xffffffffL)
   #endif
 #else
   // If IPADDR_NONE is defined, make sure INADDR_NONE uses that value
   #ifdef INADDR_NONE
     #undef INADDR_NONE
   #endif
   #define INADDR_NONE IPADDR_NONE
 #endif
 
 #endif // IP_ADDRESS_WRAPPER_H