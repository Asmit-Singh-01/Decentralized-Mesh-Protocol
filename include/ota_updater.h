#pragma once

#include <cstdint>
#include <cstddef>
#include "packet_format.h"

// OTA Protocol Commands
constexpr uint8_t OTA_CMD_START       = 0x01;
constexpr uint8_t OTA_CMD_CHUNK       = 0x02;
constexpr uint8_t OTA_CMD_END         = 0x03;
constexpr uint8_t OTA_CMD_STATUS_REQ  = 0x04;
constexpr uint8_t OTA_CMD_STATUS_RESP = 0x05;
constexpr uint8_t OTA_CMD_ABORT       = 0x06;

// Maximum firmware chunks trackable in bitmap
constexpr size_t OTA_MAX_CHUNKS = 1024;
constexpr size_t OTA_MAX_PAYLOAD_CHUNK = 56;
constexpr size_t OTA_MAX_MOCK_FLASH = 65536;

enum class OtaState : uint8_t {
    IDLE = 0,
    RECEIVING = 1,
    VERIFYING = 2,
    COMPLETED = 3,
    FAILED = 4
};

#pragma pack(push, 1)
struct OtaStartPacket {
    uint8_t cmd;             // OTA_CMD_START
    uint16_t total_chunks;   // Number of binary chunks
    uint16_t chunk_size;     // Size of individual payload chunks
    uint32_t firmware_size;  // Total binary size in bytes
    uint32_t expected_crc32; // Expected IEEE 802.3 CRC32
};

struct OtaChunkPacket {
    uint8_t cmd;             // OTA_CMD_CHUNK
    uint16_t chunk_index;    // 0-indexed chunk sequence
    uint8_t data_len;        // Length of payload bytes
    uint8_t data[OTA_MAX_PAYLOAD_CHUNK]; // Firmware chunk bytes
};

struct OtaEndPacket {
    uint8_t cmd;             // OTA_CMD_END
    uint32_t expected_crc32; // CRC32 for final verification
};

struct OtaStatusPacket {
    uint8_t cmd;             // OTA_CMD_STATUS_RESP
    uint8_t state;           // Current OtaState
    uint16_t received_chunks;// Count of chunks received
    uint16_t total_chunks;   // Total chunks expected
    uint16_t missing_chunks; // Number of missing chunks
};
#pragma pack(pop)

// Utility IEEE 802.3 CRC32 Calculation
uint32_t calculate_crc32(const uint8_t* data, size_t length, uint32_t initial_crc = 0xFFFFFFFF);

class OtaUpdater {
public:
    OtaUpdater();
    ~OtaUpdater();

    // Session Management
    bool begin(uint32_t firmware_size, uint16_t total_chunks, uint32_t expected_crc32, uint16_t chunk_size = 0);
    bool process_chunk(uint16_t chunk_index, const uint8_t* data, size_t len);
    bool complete();
    void abort();
    void reset();

    // Mesh Packet Hook Handler
    bool handle_packet(const uint8_t* payload, size_t length);

    // Missing-Chunk Tracking
    bool is_chunk_received(uint16_t chunk_index) const;
    void mark_chunk_received(uint16_t chunk_index);
    uint16_t get_missing_chunks_count() const;
    size_t get_missing_chunks_list(uint16_t* out_list, size_t max_items) const;

    // State Inspection
    OtaState get_state() const { return state_; }
    uint32_t get_bytes_written() const { return bytes_written_; }
    uint16_t get_chunks_received_count() const { return chunks_received_; }
    uint16_t get_total_chunks() const { return total_chunks_; }
    uint16_t get_chunk_size() const { return chunk_size_; }
    uint32_t get_calculated_crc32() const { return running_crc32_ ^ 0xFFFFFFFF; }

    // Reboot Callback Hook
    void set_reboot_handler(void (*handler)()) { reboot_handler_ = handler; }

    // Mock Flash Buffer Access (for desktop simulation verification)
    const uint8_t* get_mock_flash() const { return mock_flash_; }

private:
    OtaState state_;
    uint32_t firmware_size_;
    uint16_t total_chunks_;
    uint16_t chunk_size_;
    uint32_t expected_crc32_;
    uint16_t chunks_received_;
    uint32_t bytes_written_;
    uint32_t running_crc32_;

    // Bitmap for tracking received chunks (1 bit per chunk)
    uint8_t chunk_bitmap_[OTA_MAX_CHUNKS / 8];

    // Hardware reboot callback
    void (*reboot_handler_)();

    // Simulation flash storage buffer
    uint8_t mock_flash_[OTA_MAX_MOCK_FLASH];

    bool flash_write(size_t offset, const uint8_t* data, size_t len);
    bool flash_begin(size_t size);
    bool flash_end();
};
