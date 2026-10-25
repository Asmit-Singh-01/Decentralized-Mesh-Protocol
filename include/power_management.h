#pragma once

#include <cstdint>
#include <iostream>
#include <thread>
#include <chrono>

enum class PowerState {
    ACTIVE_TRANSMIT_RECEIVE = 0,
    LOW_POWER_SLEEP = 1,
    EMERGENCY_LOW_BATTERY = 2
};

class PowerManager {
private:
    uint16_t node_id;
    PowerState current_state;
    uint8_t battery_percentage;

public:
    explicit PowerManager(uint16_t id, uint8_t initial_battery = 100) 
        : node_id(id), current_state(PowerState::ACTIVE_TRANSMIT_RECEIVE), battery_percentage(initial_battery) {}

    void set_power_state(PowerState state) {
        current_state = state;
        std::cout << "[PowerManager] Node 0x" << std::hex << node_id 
                  << " power state changed. Battery: " << std::dec << static_cast<int>(battery_percentage) << "%" << std::endl;
    }

    // Simulate duty cycling for power conservation
    void execute_duty_cycle_sleep(int sleep_duration_ms = 100) {
        if (battery_percentage < 20) {
            set_power_state(PowerState::EMERGENCY_LOW_BATTERY);
            std::cout << "[Power Warning] Node 0x" << std::hex << node_id << " entering deep conservation sleep!\n";
        } else {
            set_power_state(PowerState::LOW_POWER_SLEEP);
        }

        // Simulate sleep delay
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_duration_ms));
        
        set_power_state(PowerState::ACTIVE_TRANSMIT_RECEIVE);
    }

    void consume_battery(uint8_t amount) {
        if (battery_percentage > amount) {
            battery_percentage -= amount;
        } else {
            battery_percentage = 0;
        }
    }

    PowerState get_current_state() const { return current_state; }
    uint8_t get_battery_level() const { return battery_percentage; }
};
