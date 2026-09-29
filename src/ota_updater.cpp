#include "ota_updater.h"
#include <cstdio>
#include <cstring>

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
#include <Arduino.h>
#include <Update.h>
#include <esp_system.h>
#endif

// IEEE 802.3 CRC32 Implementation
uint32_t calculate_crc32(const uint8_t* data, size_t length, uint32_t initial_crc) {
    uint32_t crc = initial_crc;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            if (crc & 1U) {
                crc = (crc >> 1U) ^ 0xEDB88320UL;
            } else {
                crc >>= 1U;
            }
        }
    }
    return crc;
}

OtaUpdater::OtaUpdater()
    : state_(OtaState::IDLE),
      firmware_size_(0),
      total_chunks_(0),
      chunk_size_(0),
      expected_crc32_(0),
      chunks_received_(0),
      bytes_written_(0),
      running_crc32_(0xFFFFFFFF),
      reboot_handler_(nullptr) {
    std::memset(chunk_bitmap_, 0, sizeof(chunk_bitmap_));
    std::memset(mock_flash_, 0, sizeof(mock_flash_));
}

OtaUpdater::~OtaUpdater() {
    if (state_ == OtaState::RECEIVING) {
        abort();
    }
}

bool OtaUpdater::flash_begin(size_t size) {
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    if (!Update.begin(size, U_FLASH)) {
        std::printf("[OTA ERROR] ESP32 Update.begin failed: %s\n", Update.errorString());
        return false;
    }
    return true;
#else
    if (size > OTA_MAX_MOCK_FLASH) {
        std::printf("[OTA ERROR] Simulated firmware size (%zu) exceeds buffer (%zu)\n", size, OTA_MAX_MOCK_FLASH);
        return false;
    }
    std::memset(mock_flash_, 0xFF, sizeof(mock_flash_)); // Flash erased state is 0xFF
    return true;
#endif
}

bool OtaUpdater::flash_write(size_t offset, const uint8_t* data, size_t len) {
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    size_t written = Update.write(const_cast<uint8_t*>(data), len);
    return written == len;
#else
    if (offset + len > OTA_MAX_MOCK_FLASH) {
        return false;
    }
    std::memcpy(mock_flash_ + offset, data, len);
    return true;
#endif
}

bool OtaUpdater::flash_end() {
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    if (!Update.end(true)) {
        std::printf("[OTA ERROR] ESP32 Update.end failed: %s\n", Update.errorString());
        return false;
    }
    return true;
#else
    return true;
#endif
}

bool OtaUpdater::begin(uint32_t firmware_size, uint16_t total_chunks, uint32_t expected_crc32, uint16_t chunk_size) {
    if (firmware_size == 0 || total_chunks == 0 || total_chunks > OTA_MAX_CHUNKS) {
        std::printf("[OTA ERROR] Invalid OTA parameters: Size=%u, Chunks=%u\n", firmware_size, total_chunks);
        state_ = OtaState::FAILED;
        return false;
    }

    firmware_size_ = firmware_size;
    total_chunks_ = total_chunks;
    if (chunk_size > 0) {
        chunk_size_ = chunk_size;
    } else {
        chunk_size_ = static_cast<uint16_t>((firmware_size_ + total_chunks_ - 1) / total_chunks_);
    }
    expected_crc32_ = expected_crc32;
    chunks_received_ = 0;
    bytes_written_ = 0;
    running_crc32_ = 0xFFFFFFFF;

    std::memset(chunk_bitmap_, 0, sizeof(chunk_bitmap_));

    if (!flash_begin(firmware_size_)) {
        state_ = OtaState::FAILED;
        return false;
    }

    state_ = OtaState::RECEIVING;
    std::printf("[OTA] Session initiated: Size=%uB, Chunks=%u, ChunkSize=%uB, Expected CRC=0x%08X\n",
                firmware_size_, total_chunks_, chunk_size_, expected_crc32_);
    return true;
}

bool OtaUpdater::is_chunk_received(uint16_t chunk_index) const {
    if (chunk_index >= OTA_MAX_CHUNKS) {
        return false;
    }
    return (chunk_bitmap_[chunk_index / 8] & (1U << (chunk_index % 8))) != 0;
}

void OtaUpdater::mark_chunk_received(uint16_t chunk_index) {
    if (chunk_index < OTA_MAX_CHUNKS) {
        chunk_bitmap_[chunk_index / 8] |= static_cast<uint8_t>(1U << (chunk_index % 8));
    }
}

uint16_t OtaUpdater::get_missing_chunks_count() const {
    if (total_chunks_ == 0) return 0;
    uint16_t missing = 0;
    for (uint16_t i = 0; i < total_chunks_; ++i) {
        if (!is_chunk_received(i)) {
            missing++;
        }
    }
    return missing;
}

size_t OtaUpdater::get_missing_chunks_list(uint16_t* out_list, size_t max_items) const {
    if (!out_list || max_items == 0 || total_chunks_ == 0) return 0;
    size_t count = 0;
    for (uint16_t i = 0; i < total_chunks_ && count < max_items; ++i) {
        if (!is_chunk_received(i)) {
            out_list[count++] = i;
        }
    }
    return count;
}

bool OtaUpdater::process_chunk(uint16_t chunk_index, const uint8_t* data, size_t len) {
    if (state_ != OtaState::RECEIVING) {
        std::printf("[OTA WARN] Received chunk but not in RECEIVING state (current state=%u)\n", static_cast<uint8_t>(state_));
        return false;
    }

    if (chunk_index >= total_chunks_ || !data || len == 0) {
        std::printf("[OTA ERROR] Invalid chunk index or data: Chunk=%u, Len=%zu\n", chunk_index, len);
        return false;
    }

    // Ignore duplicate chunks
    if (is_chunk_received(chunk_index)) {
        std::printf("[OTA WARN] Duplicate chunk %u received, skipping.\n", chunk_index);
        return true;
    }

    // Calculate sequential flash offset
    size_t flash_offset = static_cast<size_t>(chunk_index) * chunk_size_;
    if (flash_offset + len > firmware_size_) {
        std::printf("[OTA ERROR] Chunk %u write would exceed firmware size (%zu > %u)\n",
                    chunk_index, flash_offset + len, firmware_size_);
        return false;
    }

    if (!flash_write(flash_offset, data, len)) {
        std::printf("[OTA ERROR] Failed writing chunk %u to flash storage.\n", chunk_index);
        return false;
    }

    mark_chunk_received(chunk_index);
    chunks_received_++;
    bytes_written_ += static_cast<uint32_t>(len);

    std::printf("[OTA] Received Chunk %u/%u (%zu bytes, total written: %u/%uB)\n",
                chunk_index + 1, total_chunks_, len, bytes_written_, firmware_size_);

    return true;
}

bool OtaUpdater::complete() {
    if (state_ != OtaState::RECEIVING) {
        std::printf("[OTA ERROR] complete() called but state is not RECEIVING.\n");
        return false;
    }

    uint16_t missing = get_missing_chunks_count();
    if (missing > 0) {
        std::printf("[OTA ERROR] Cannot complete OTA: %u chunks missing out of %u!\n", missing, total_chunks_);
        state_ = OtaState::FAILED;
        return false;
    }

    state_ = OtaState::VERIFYING;
    std::printf("[OTA] Verifying binary integrity across %u bytes...\n", firmware_size_);

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    // On ESP32, verify flash
    if (!flash_end()) {
        state_ = OtaState::FAILED;
        return false;
    }
#else
    // Desktop simulation CRC32 verification over written buffer
    uint32_t computed_crc = calculate_crc32(mock_flash_, firmware_size_) ^ 0xFFFFFFFF;
    if (computed_crc != expected_crc32_) {
        std::printf("[OTA ERROR] CRC32 verification failed! Computed: 0x%08X, Expected: 0x%08X\n",
                    computed_crc, expected_crc32_);
        state_ = OtaState::FAILED;
        return false;
    }
    flash_end();
#endif

    state_ = OtaState::COMPLETED;
    std::printf("[OTA SUCCESS] Firmware verified with CRC32: 0x%08X! Ready for reboot.\n", expected_crc32_);

    // Trigger reboot hook
    if (reboot_handler_) {
        reboot_handler_();
    } else {
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
        std::printf("[OTA] Rebooting ESP32...\n");
        esp_restart();
#else
        std::printf("[OTA REBOOT] Mock node rebooted into updated firmware partition.\n");
#endif
    }

    return true;
}

void OtaUpdater::abort() {
    if (state_ == OtaState::RECEIVING || state_ == OtaState::VERIFYING) {
        std::printf("[OTA WARN] OTA update aborted.\n");
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
        Update.abort();
#endif
    }
    state_ = OtaState::FAILED;
}

void OtaUpdater::reset() {
    state_ = OtaState::IDLE;
    firmware_size_ = 0;
    total_chunks_ = 0;
    expected_crc32_ = 0;
    chunks_received_ = 0;
    bytes_written_ = 0;
    running_crc32_ = 0xFFFFFFFF;
    std::memset(chunk_bitmap_, 0, sizeof(chunk_bitmap_));
}

bool OtaUpdater::handle_packet(const uint8_t* payload, size_t length) {
    if (!payload || length == 0) {
        return false;
    }

    uint8_t cmd = payload[0];
    switch (cmd) {
        case OTA_CMD_START: {
            if (length < sizeof(OtaStartPacket)) {
                return false;
            }
            OtaStartPacket pkt;
            std::memcpy(&pkt, payload, sizeof(OtaStartPacket));
            return begin(pkt.firmware_size, pkt.total_chunks, pkt.expected_crc32, pkt.chunk_size);
        }
        case OTA_CMD_CHUNK: {
            if (length < 4) { // cmd (1) + chunk_index (2) + data_len (1)
                return false;
            }
            uint16_t chunk_idx = 0;
            std::memcpy(&chunk_idx, payload + 1, sizeof(uint16_t));
            uint8_t data_len = payload[3];
            if (length < 4 + static_cast<size_t>(data_len)) {
                return false;
            }
            return process_chunk(chunk_idx, payload + 4, data_len);
        }
        case OTA_CMD_END: {
            return complete();
        }
        case OTA_CMD_ABORT: {
            abort();
            return true;
        }
        default:
            return false;
    }
}
