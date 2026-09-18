#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string>
#include "packet_format.h"

#define DEFAULT_SD_CS_PIN 5
#define DEFAULT_LOG_FILENAME "mesh_logs.csv"
#define DEFAULT_FLUSH_INTERVAL 10

class SDCardLogger {
private:
    uint8_t cs_pin;
    std::string log_filename;
    bool card_available;
    size_t unflushed_writes;
    size_t total_writes;
    size_t flush_interval;

public:
    explicit SDCardLogger(uint8_t pin = DEFAULT_SD_CS_PIN, 
                          const char* filename = DEFAULT_LOG_FILENAME,
                          size_t flush_thresh = DEFAULT_FLUSH_INTERVAL);
    ~SDCardLogger();

    // Initialization & Hardware Check
    bool init();
    bool is_card_available() const;

    // Logging Methods
    bool log_packet(uint32_t timestamp_ms, uint16_t source_id, uint16_t dest_id,
                    int8_t rssi, uint8_t payload_len, bool crc_valid);
    bool log_packet(uint32_t timestamp_ms, const MeshPacket& packet, int8_t rssi, bool crc_valid);

    // Buffer Management
    void flush();
    void close();

    // Diagnostics
    size_t get_unflushed_count() const;
    size_t get_total_writes() const;
    const std::string& get_filename() const;
};
