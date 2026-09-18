#include "power_manager.h"
#include <iostream>

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
#include "esp_sleep.h"
#include "esp_wifi.h"
#endif

PowerManager::PowerManager(bool enabled, uint32_t min_window_ms)
    : power_save_enabled(enabled),
      min_sleep_window_ms(min_window_ms),
      total_sleep_cycles(0),
      total_sleep_time_ms(0),
      radio_paused(false) {}

uint32_t PowerManager::calculate_sleep_window(uint32_t current_time_ms, 
                                             uint32_t next_beacon_time_ms, 
                                             uint32_t guard_band_ms) const {
    if (next_beacon_time_ms <= current_time_ms + guard_band_ms) {
        return 0;
    }
    return (next_beacon_time_ms - current_time_ms) - guard_band_ms;
}

void PowerManager::prepare_radio_sleep(MeshNode* node) {
    (void)node; // Preserves node routing table without memory loss
    radio_paused = true;
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    // Safely pause active Wi-Fi / ESP-NOW radio activity before sleep
    esp_wifi_stop();
#endif
    std::cout << "[POWER] Radio paused safely. Routing table state preserved." << std::endl;
}

void PowerManager::resume_radio_wakeup(MeshNode* node) {
    (void)node; // Routing table intact
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    // Re-initialize radio peripheral seamlessly
    esp_wifi_start();
#endif
    radio_paused = false;
    std::cout << "[POWER] Woke from Light-Sleep. Radio resumed. Routing table intact (" 
              << (node ? node->get_routing_table().size() : 0) << " peers active)." << std::endl;
}

bool PowerManager::enter_light_sleep(uint32_t duration_ms, MeshNode* node) {
    if (!power_save_enabled) {
        std::cout << "[POWER INFO] Power saving disabled. Skipping sleep." << std::endl;
        return false;
    }

    if (duration_ms < min_sleep_window_ms) {
        std::cout << "[POWER INFO] Idle window (" << duration_ms 
                  << " ms) below minimum threshold (" << min_sleep_window_ms 
                  << " ms). Skipping sleep." << std::endl;
        return false;
    }

    std::cout << "[POWER] Entering ESP32 Light-Sleep mode for " << duration_ms << " ms..." << std::endl;
    prepare_radio_sleep(node);

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    // Configure ESP32 hardware timer wake source (microseconds)
    esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(duration_ms) * 1000ULL);
    esp_light_sleep_start();
#endif

    resume_radio_wakeup(node);

    ++total_sleep_cycles;
    total_sleep_time_ms += duration_ms;
    std::cout << "[POWER SUCCESS] Completed sleep cycle #" << total_sleep_cycles 
              << " (Total sleep: " << total_sleep_time_ms << " ms)." << std::endl;
    return true;
}

bool PowerManager::is_power_save_enabled() const {
    return power_save_enabled;
}

void PowerManager::set_power_save_enabled(bool enabled) {
    power_save_enabled = enabled;
}

bool PowerManager::is_radio_paused() const {
    return radio_paused;
}

uint32_t PowerManager::get_total_sleep_cycles() const {
    return total_sleep_cycles;
}

uint64_t PowerManager::get_total_sleep_time_ms() const {
    return total_sleep_time_ms;
}

void PowerManager::reset_metrics() {
    total_sleep_cycles = 0;
    total_sleep_time_ms = 0;
}
