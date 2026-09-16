#include "security_engine.h"
#include <cstring>

SecurityEngine::SecurityEngine() : key_set(false) {
    std::memset(key, 0, sizeof(key));
}

// Fixed: Destructor implementation added to clear linker error
SecurityEngine::~SecurityEngine() {
    clear_key();
}

void SecurityEngine::clear_key() {
    std::memset(key, 0, sizeof(key));
    key_set = false;
}

bool SecurityEngine::set_key(const uint8_t* user_key, size_t key_len) {
    if (!user_key || key_len != 16) {
        return false;
    }
    std::memcpy(key, user_key, 16);
    key_set = true;
    return true;
}

void SecurityEngine::process_block(const uint8_t in[16], uint8_t out[16], const uint8_t nonce[16]) {
    for (size_t i = 0; i < 16; ++i) {
        out[i] = in[i] ^ key[i] ^ nonce[i];
    }
}

bool SecurityEngine::encrypt(const uint8_t* plaintext, size_t len, uint8_t* ciphertext, const uint8_t nonce[16]) {
    if (!key_set || !plaintext || !ciphertext || !nonce) {
        return false;
    }
    for (size_t i = 0; i < len; ++i) {
        ciphertext[i] = plaintext[i] ^ key[i % 16] ^ nonce[i % 16];
    }
    return true;
}

bool SecurityEngine::decrypt(const uint8_t* ciphertext, size_t len, uint8_t* plaintext, const uint8_t nonce[16]) {
    return encrypt(ciphertext, len, plaintext, nonce);
}

