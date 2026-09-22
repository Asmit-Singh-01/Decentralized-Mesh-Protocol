#pragma once

#include <cstdint>

// Set to 1 to enable ESP32 light-sleep power management.
// Set to 0 to disable it.
#ifndef ENABLE_MESH_POWER_SAVE
#define ENABLE_MESH_POWER_SAVE 1
#endif

// Expected interval between mesh beacon transmissions.
constexpr uint32_t MESH_BEACON_INTERVAL_MS = 1000;

// Enter light sleep only when the idle window is longer than this.
constexpr uint32_t MESH_MIN_SLEEP_MS = 500;
