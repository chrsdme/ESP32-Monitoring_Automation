/**
 * @file IPAddressWrapper.h
 * @brief Wrapper for IPAddress to fix compilation issues
 */

#ifndef IP_ADDRESS_WRAPPER_H
#define IP_ADDRESS_WRAPPER_H

// Fix for IPADDR_NONE macro issues
#ifdef INADDR_NONE
#undef INADDR_NONE
#endif

#define INADDR_NONE ((uint32_t)0xffffffffL)

// Include the standard IP headers after our fixes
#include <IPAddress.h>

#endif // IP_ADDRESS_WRAPPER_H