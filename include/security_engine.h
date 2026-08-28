#pragma once
#include <stdint.h>
#include <stddef.h>

class SecurityEngine {
private:
    uint8_t key[16];
    bool key_set;

    void process_block(const uint8_t in[16], uint8_t out[16], const uint8_t nonce[16]);

public:
    SecurityEngine();
    
    bool set_key(const uint8_t* user_key, size_t key_len);
    bool encrypt(const uint8_t* plaintext, size_t len, uint8_t* ciphertext, const uint8_t nonce[16]);
    bool decrypt(const uint8_t* ciphertext, size_t len, uint8_t* plaintext, const uint8_t nonce[16]);
};
