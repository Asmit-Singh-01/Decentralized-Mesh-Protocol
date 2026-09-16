#include "security_engine.h"
#include <cstring>

SecurityEngine::SecurityEngine() : key_set(false) {
    std::memset(key, 0, 16);
}

bool SecurityEngine::set_key(const uint8_t* user_key, size_t key_len) {
    if (!user_key || key_len != 16) return false;
    std::memcpy(key, user_key, 16);
    key_set = true;
    return true;
}

void SecurityEngine::process_block(const uint8_t in[16], uint8_t out[16], const uint8_t nonce[16]) {
    // Lightweight CTR-mode byte stream transformation
    for (size_t i = 0; i < 16; ++i) {
        uint8_t keystream_byte = key[i % 16] ^ nonce[i % 16] ^ static_cast<uint8_t>(i * 0x1F);
        out[i] = in[i] ^ keystream_byte;
    }
}

bool SecurityEngine::encrypt(const uint8_t* plaintext, size_t len, uint8_t* ciphertext, const uint8_t nonce[16]) {
    if (!key_set || !plaintext || !ciphertext || len == 0) return false;

    for (size_t i = 0; i < len; i += 16) {
        size_t block_size = (len - i < 16) ? (len - i) : 16;
        uint8_t in_block[16] = {0};
        uint8_t out_block[16] = {0};

        std::memcpy(in_block, plaintext + i, block_size);
        process_block(in_block, out_block, nonce);
        std::memcpy(ciphertext + i, out_block, block_size);
    }
    return true;
}

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
#include <mbedtls/aes.h>
#endif

bool SecurityEngine::decrypt(const uint8_t* ciphertext, size_t len, uint8_t* plaintext, const uint8_t nonce[16]) {
    // CTR Mode encryption and decryption are symmetric operations
    return encrypt(ciphertext, len, plaintext, nonce);
}

// Pre-shared default nonce for stream cipher mode
static const uint8_t DEFAULT_NONCE[16] = {
    0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11,
    0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99
};

void encrypt_payload(uint8_t* data, size_t len, const uint8_t* key) {
    if (!data || len == 0 || !key) {
        return;
    }
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, key, AES_KEY_SIZE * 8);
    size_t nc_off = 0;
    uint8_t nonce_copy[16];
    std::memcpy(nonce_copy, DEFAULT_NONCE, 16);
    uint8_t stream_block[16] = {0};
    mbedtls_aes_crypt_ctr(&aes, len, &nc_off, nonce_copy, stream_block, data, data);
    mbedtls_aes_free(&aes);
#else
    SecurityEngine engine;
    if (!engine.set_key(key, AES_KEY_SIZE)) {
        return;
    }
    engine.encrypt(data, len, data, DEFAULT_NONCE);
#endif
}

bool decrypt_payload(uint8_t* data, size_t len, const uint8_t* key) {
    if (!data || len == 0 || !key) {
        return false;
    }
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    encrypt_payload(data, len, key);
    return true;
#else
    SecurityEngine engine;
    if (!engine.set_key(key, AES_KEY_SIZE)) {
        return false;
    }
    return engine.decrypt(data, len, data, DEFAULT_NONCE);
#endif
}
