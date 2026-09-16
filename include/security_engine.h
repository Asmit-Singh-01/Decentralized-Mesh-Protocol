#pragma once
#include <stdint.h>
#include <stddef.h>
#define DEFAULT_MESH_KEY {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, \
                        0x09, 0x10, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F}
#define AES_KEY_SIZE 16

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

void encrypt_payload(uint8_t* data, size_t len, const uint8_t* key);
bool decrypt_payload(uint8_t* data, size_t len, const uint8_t* key);
