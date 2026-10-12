#include "security_engine.h"
#include <cstring>
#include <algorithm>

SecurityEngine::SecurityEngine() : key_set(false) {
    std::memset(encryption_key, 0, sizeof(encryption_key));
}

void SecurityEngine::set_key(const uint8_t* key, size_t len) {
    if (!key) return;
    size_t copy_len = std::min(len, sizeof(encryption_key));
    std::memcpy(encryption_key, key, copy_len);
    key_set = true;
}

void SecurityEngine::clear_key() {
    std::memset(encryption_key, 0, sizeof(encryption_key));
    key_set = false;
    last_seen_sequences.clear();
}

bool SecurityEngine::encrypt_payload(uint8_t* payload, size_t len, uint32_t nonce) const {
    if (!payload || !key_set) return false;
    
    // Lightweight XOR-CTR stream cipher simulation using key and nonce
    for (size_t i = 0; i < len; ++i) {
        uint8_t keystream_byte = encryption_key[i % 16] ^ static_cast<uint8_t>((nonce >> (8 * (i % 4))) & 0xFF);
        payload[i] ^= keystream_byte;
    }
    return true;
}

bool SecurityEngine::decrypt_payload(uint8_t* payload, size_t len, uint32_t nonce) const {
    // CTR mode encryption is symmetric; decryption is identical to encryption
    return encrypt_payload(payload, len, nonce);
}

bool SecurityEngine::validate_sequence(uint16_t sender_id, uint32_t seq_num) {
    auto it = last_seen_sequences.find(sender_id);
    if (it == last_seen_sequences.end()) {
        last_seen_sequences[sender_id] = seq_num;
        return true;
    }

    // Check if packet sequence is newer than the last recorded one
    if (seq_num > it->second) {
        it->second = seq_num;
        return true;
    }

    // Duplicate or old sequence detected (Replay attack vector)
    return false;
}

void SecurityEngine::reset_trackers() {
    last_seen_sequences.clear();
}

