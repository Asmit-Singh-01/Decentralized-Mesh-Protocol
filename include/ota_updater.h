#pragma once

#include <cstddef>
#include <cstdint>

void ota_updater_handle_packet(
    const uint8_t* data,
    size_t length
);

void ota_updater_rx_callback(
    const uint8_t* src_mac,
    const uint8_t* data,
    size_t length,
    int8_t rssi
);
