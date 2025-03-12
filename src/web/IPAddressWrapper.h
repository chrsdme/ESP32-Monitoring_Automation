/**
 * @file IPAddressWrapper.h
 * @brief Wrapper for IP address handling with conditional INADDR_NONE definition
 */

#ifndef IP_ADDRESS_WRAPPER_H
#define IP_ADDRESS_WRAPPER_H

#include <Arduino.h>
#include <IPAddress.h>

// Only define INADDR_NONE if it's not already defined by lwip
#ifndef INADDR_NONE
#define INADDR_NONE ((uint32_t)0xffffffffL)
#endif

#endif // IP_ADDRESS_WRAPPER_H