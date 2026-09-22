#pragma once

#include <cstddef>
#include <cstdint>

bool sd_logger_init(uint8_t cs_pin);

void sd_logger_log_packet(
    uint32_t timestamp,
    uint32_t source_id,
    uint32_t dest_id,
    int8_t rssi,
    size_t payload_len,
    bool crc_status
);

void sd_logger_flush();
