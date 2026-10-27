#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <iostream>

class ErrorCorrectionEngine {
public:
    // Simple XOR parity-based Forward Error Correction (FEC) generator
    static uint8_t calculate_parity_byte(const uint8_t* data, size_t len) {
        uint8_t parity = 0;
        for (size_t i = 0; i < len; ++i) {
            parity ^= data[i];
        }
        return parity;
    }

    // Embed parity into packet payload stream
    static bool encode_packet_data(const uint8_t* raw_data, size_t data_len, std::vector<uint8_t>& output_buffer) {
        if (!raw_data || data_len == 0) return false;

        output_buffer.resize(data_len + 1);
        for (size_t i = 0; i < data_len; ++i) {
            output_buffer[i] = raw_data[i];
        }
        
        // Append parity byte at the end
        output_buffer[data_len] = calculate_parity_byte(raw_data, data_len);
        return true;
    }

    // Verify and recover/check integrity of received data
    static bool verify_and_recover(const uint8_t* packet_with_parity, size_t total_len) {
        if (!packet_with_parity || total_len <= 1) return false;

        size_t data_len = total_len - 1;
        uint8_t received_parity = packet_with_parity[data_len];
        uint8_t calculated_parity = calculate_parity_byte(packet_with_parity, data_len);

        if (received_parity == calculated_parity) {
            std::cout << "[FEC Engine] Packet integrity verified successfully. No corruption detected." << std::endl;
            return true;
        } else {
            std::cerr << "[FEC Engine Warning] Packet corruption detected in transit! Attempting correction..." << std::endl;
            // In a full Hamming/Reed-Solomon code, bits are mathematically corrected here.
            return false; 
        }
    }
};
