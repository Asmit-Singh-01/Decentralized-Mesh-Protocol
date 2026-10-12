#pragma once

#include <cstdint>
#include <cstddef>
#include <unordered_map>

class SecurityEngine {
private:
    uint8_t encryption_key[16];
    bool key_set;
    // Anti-replay tracker: maps sender_id -> last highest sequence number
    std::unordered_map<uint16_t, uint32_t> last_seen_sequences;

public:
    SecurityEngine();

    void set_key(const uint8_t* key, size_t len);
    void clear_key();

    bool encrypt_payload(uint8_t* payload, size_t len, uint32_t nonce) const;
    bool decrypt_payload(uint8_t* payload, size_t len, uint32_t nonce) const;

    // Anti-replay validation
    bool validate_sequence(uint16_t sender_id, uint32_t seq_num);
    void reset_trackers();
};
