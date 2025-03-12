/**
 * @file hwcrypto/sha.h
 * @brief Compatibility header for hardware crypto SHA
 */

#ifndef HWCRYPTO_SHA_H
#define HWCRYPTO_SHA_H

#include <mbedtls/sha256.h>

// Placeholder definitions to satisfy the library requirements
typedef struct {
    mbedtls_sha256_context ctx;
} sha_context;

inline void sha_init(sha_context* ctx) {
    mbedtls_sha256_init(&ctx->ctx);
}

inline void sha_update(sha_context* ctx, const void* data, size_t len) {
    mbedtls_sha256_update(&ctx->ctx, (const unsigned char*)data, len);
}

inline void sha_final(sha_context* ctx, unsigned char* digest) {
    mbedtls_sha256_finish(&ctx->ctx, digest);
}

inline void sha_free(sha_context* ctx) {
    mbedtls_sha256_free(&ctx->ctx);
}

#endif // HWCRYPTO_SHA_H