#pragma once
#define MBEDTLS_AES_C
#define MBEDTLS_CTR_DRBG_C
#if defined(PHOENIX_RNG_TESTING)
#define MBEDTLS_SELF_TEST
#endif
#define MBEDTLS_CTR_DRBG_ENTROPY_LEN 48
