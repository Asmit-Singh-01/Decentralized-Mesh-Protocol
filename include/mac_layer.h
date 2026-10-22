#pragma once

#include <cstdint>
#include <random>
#include <thread>
#include <chrono>
#include <iostream>

class MacLayer {
private:
    uint16_t node_id;
    int max_retries;
    std::mt19937 rng;

public:
    explicit MacLayer(uint16_t id, int retries = 3) 
        : node_id(id), max_retries(retries) {
        rng.seed(std::random_device{}());
    }

    // Simulate carrier sensing and random backoff before transmission
    bool channel_access_with_backoff() {
        std::uniform_int_distribution<int> backoff_ms(5, 50);

        for (int attempt = 1; attempt <= max_retries; ++attempt) {
            // Simulate checking if channel is busy (Carrier Sense)
            bool channel_busy = (rng() % 10 == 0); // 10% simulated busy chance

            if (!channel_busy) {
                return true; // Channel is clear, proceed to transmit
            }

            int wait_time = backoff_ms(rng) * attempt; // Exponential backoff scaling
            std::cout << "[MAC Layer] Node 0x" << std::hex << node_id 
                      << " detected channel busy. Backing off for " << std::dec << wait_time << "ms (Attempt " << attempt << "/" << max_retries << ")" << std::endl;
            
            std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));
        }

        std::cerr << "[MAC Layer Error] Node 0x" << std::hex << node_id 
                  << " channel access failed after max retries due to persistent collisions!" << std::endl;
        return false;
    }
};
