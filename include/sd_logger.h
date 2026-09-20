#pragma once

#include <cstddef>
#include <cstdint>

class SDLogger {
private:
    static constexpr uint8_t FLUSH_THRESHOLD = 10;
    static constexpr size_t MAX_RECORDS = 10;
    static constexpr size_t MAX_RECORD_SIZE = 128;

    static constexpr const char* LOG_PATH = "/mesh_logs.csv";

    uint8_t cs_pin;
    bool initialized;
    uint8_t pending_records;

    char record_buffer[MAX_RECORDS][MAX_RECORD_SIZE];

    bool append_buffer_to_storage();
    bool ensure_log_file();

public:
    explicit SDLogger(uint8_t cs_pin = 5);

    // Initializes the SD card and creates the log file/header if needed.
    // Returns false when the SD card is unavailable.
    bool init();

    // Adds a formatted packet record to RAM only.
    // No SD I/O is performed here.
    bool log_packet(
        uint32_t timestamp,
        uint16_t source_id,
        uint16_t dest_id,
        int8_t rssi,
        uint8_t payload_len,
        bool crc_valid
    );

    // Performs deferred SD/file I/O.
    void service();

    // Forces all pending records to storage.
    bool flush();

    bool is_ready() const;
    uint8_t pending_count() const;
};