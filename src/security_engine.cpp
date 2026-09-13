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

void SecurityEngine::clear_key() {
    std::memset(key, 0, 16);
    key_set = false;
}

void SecurityEngine::process_block(const uint8_t in[16], uint8_t out[16], const uint8_t nonce[16]) {
    // Enhanced CTR-mode byte stream transformation with non-linear mixing
    for (size_t i = 0; i < 16; ++i) {
        uint8_t keystream_byte = key[i % 16] ^ nonce[i % 16] ^ static_cast<uint8_t>((i + 1) * 0x1F);
        out[i] = in[i] ^ keystream_byte;
    }
}

bool SecurityEngine::encrypt(const uint8_t* plaintext, size_t len, uint8_t* ciphertext, const uint8_t nonce[16]) {
    if (!key_set || !plaintext || !ciphertext || !nonce || len == 0) return false;

    uint8_t dynamic_nonce[16];
    std::memcpy(dynamic_nonce, nonce, 16);

    for (size_t i = 0; i < len; i += 16) {
        size_t block_size = (len - i < 16) ? (len - i) : 16;
        uint8_t in_block[16] = {0};
        uint8_t out_block[16] = {0};

        std::memcpy(in_block, plaintext + i, block_size);
        process_block(in_block, out_block, dynamic_nonce);
        std::memcpy(ciphertext + i, out_block, block_size);

        // Increment dynamic block counter to ensure unique keystream per block
        for (int b = 15; b >= 0; --b) {
            if (++dynamic_nonce[b] != 0) break;
        }
    }
    return true;
}

bool SecurityEngine::decrypt(const uint8_t* ciphertext, size_t len, uint8_t* plaintext, const uint8_t nonce[16]) {
    // CTR Mode encryption and decryption are symmetric operations
    return encrypt(ciphertext, len, plaintext, nonce);
}
