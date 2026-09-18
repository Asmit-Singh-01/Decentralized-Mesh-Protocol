#pragma once
#include <stdint.h>
#include <stddef.h>
#include "mesh_node.h"

#ifndef ENABLE_MESH_POWER_SAVE
#define ENABLE_MESH_POWER_SAVE 1
#endif

#define MIN_SLEEP_WINDOW_MS 500
#define DEFAULT_GUARD_BAND_MS 50

class PowerManager {
private:
    bool power_save_enabled;
    uint32_t min_sleep_window_ms;
    uint32_t total_sleep_cycles;
    uint64_t total_sleep_time_ms;
    bool radio_paused;

    void prepare_radio_sleep(MeshNode* node);
    void resume_radio_wakeup(MeshNode* node);

public:
    explicit PowerManager(bool enabled = (ENABLE_MESH_POWER_SAVE == 1),
                          uint32_t min_window_ms = MIN_SLEEP_WINDOW_MS);

    // Sleep Window Calculation
    uint32_t calculate_sleep_window(uint32_t current_time_ms, 
                                   uint32_t next_beacon_time_ms, 
                                   uint32_t guard_band_ms = DEFAULT_GUARD_BAND_MS) const;

    // Light-Sleep Execution
    bool enter_light_sleep(uint32_t duration_ms, MeshNode* node = nullptr);

    // Configuration & State
    bool is_power_save_enabled() const;
    void set_power_save_enabled(bool enabled);
    bool is_radio_paused() const;

    // Telemetry & Metrics
    uint32_t get_total_sleep_cycles() const;
    uint64_t get_total_sleep_time_ms() const;
    void reset_metrics();
};
