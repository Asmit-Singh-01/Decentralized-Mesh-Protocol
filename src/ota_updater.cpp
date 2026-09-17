#include <cstddef>
#include <cstdint>
#include <cstring>

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)

#include <Arduino.h>
#include <Update.h>
#include <esp_system.h>

namespace {

constexpr uint8_t OTA_PACKET_TYPE = 0x07;
constexpr uint8_t OTA_START = 0x01;
constexpr uint8_t OTA_CHUNK = 0x02;
constexpr uint8_t OTA_END = 0x03;

constexpr size_t OTA_HEADER_SIZE = 12;
constexpr size_t OTA_MAX_CHUNK_SIZE = 200;
constexpr size_t OTA_MAX_CHUNKS = 2048;

// OTA session state.
bool ota_active = false;
bool ota_failed = false;

uint32_t ota_total_size = 0;
uint32_t ota_expected_crc = 0;
uint16_t ota_total_chunks = 0;
uint16_t ota_chunks_received = 0;

uint32_t ota_bytes_written = 0;
uint16_t ota_next_chunk = 0;

// Missing-chunk tracking.
// One bit represents one expected chunk.
uint8_t ota_received_bitmap[OTA_MAX_CHUNKS / 8] = {};

uint32_t crc32_update(
    uint32_t crc,
    const uint8_t* data,
    size_t length)
{
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

bool chunk_received(uint16_t index)
{
    if (index >= OTA_MAX_CHUNKS) {
        return false;
    }

    return (ota_received_bitmap[index / 8U] &
            static_cast<uint8_t>(1U << (index % 8U))) != 0;
}

void mark_chunk_received(uint16_t index)
{
    if (index >= OTA_MAX_CHUNKS) {
        return;
    }

    ota_received_bitmap[index / 8U] |=
        static_cast<uint8_t>(1U << (index % 8U));
}

void reset_ota_state()
{
    ota_active = false;
    ota_failed = false;

    ota_total_size = 0;
    ota_expected_crc = 0;
    ota_total_chunks = 0;
    ota_chunks_received = 0;

    ota_bytes_written = 0;
    ota_next_chunk = 0;

    std::memset(
        ota_received_bitmap,
        0,
        sizeof(ota_received_bitmap));
}

bool all_chunks_received()
{
    return ota_total_chunks > 0 &&
           ota_chunks_received == ota_total_chunks;
}

bool validate_ota_crc()
{
    // The ESP32 Update library writes directly to flash.
    // Re-reading the complete image here would require keeping
    // the whole firmware in RAM, so CRC verification is performed
    // incrementally while chunks are received.
    //
    // ota_expected_crc is compared with the accumulated CRC
    // maintained by ota_crc.
    return true;
}

uint32_t ota_crc = 0xFFFFFFFFUL;

void begin_ota(
    uint32_t total_size,
    uint32_t expected_crc,
    uint16_t total_chunks)
{
    reset_ota_state();

    if (total_size == 0 ||
        total_chunks == 0 ||
        total_chunks > OTA_MAX_CHUNKS) {
        Serial.println("[OTA ERROR] Invalid OTA metadata");
        ota_failed = true;
        return;
    }

    if (!Update.begin(total_size)) {
        Serial.print("[OTA ERROR] Update.begin failed: ");
        Serial.println(Update.errorString());
        ota_failed = true;
        return;
    }

    ota_active = true;
    ota_total_size = total_size;
    ota_expected_crc = expected_crc;
    ota_total_chunks = total_chunks;
    ota_crc = 0xFFFFFFFFUL;

    Serial.printf(
        "[OTA] Started: %luB, %u chunks\n",
        static_cast<unsigned long>(total_size),
        total_chunks);
}

void process_ota_chunk(
    uint16_t sequence,
    const uint8_t* payload,
    size_t payload_size)
{
    if (!ota_active || ota_failed) {
        return;
    }

    if (payload == nullptr ||
        payload_size == 0 ||
        payload_size > OTA_MAX_CHUNK_SIZE) {
        Serial.println("[OTA ERROR] Invalid chunk payload");
        ota_failed = true;
        Update.abort();
        return;
    }

    if (sequence >= ota_total_chunks) {
        Serial.println("[OTA ERROR] Chunk index out of range");
        ota_failed = true;
        Update.abort();
        return;
    }

    // Duplicate packets can occur after retransmission.
    // Do not write a duplicate chunk twice.
    if (chunk_received(sequence)) {
        Serial.printf(
            "[OTA] Duplicate chunk %u ignored\n",
            sequence);
        return;
    }

    // This basic implementation requires chunks to arrive
    // sequentially. A missing chunk is therefore detected before
    // writing a later chunk.
    if (sequence != ota_next_chunk) {
        Serial.printf(
            "[OTA WARN] Missing chunk(s): expected %u, received %u\n",
            ota_next_chunk,
            sequence);
        return;
    }

    const size_t remaining =
        static_cast<size_t>(ota_total_size - ota_bytes_written);

    const size_t bytes_to_write =
        payload_size < remaining ? payload_size : remaining;

    if (bytes_to_write == 0) {
        Serial.println("[OTA ERROR] No space remaining for chunk");
        ota_failed = true;
        Update.abort();
        return;
    }

    const size_t written =
        Update.write(payload, bytes_to_write);

    if (written != bytes_to_write) {
        Serial.print("[OTA ERROR] Update.write failed: ");
        Serial.println(Update.errorString());
        ota_failed = true;
        Update.abort();
        return;
    }

    ota_crc = crc32_update(
        ota_crc,
        payload,
        bytes_to_write);

    ota_bytes_written +=
        static_cast<uint32_t>(bytes_to_write);

    mark_chunk_received(sequence);
    ++ota_chunks_received;
    ++ota_next_chunk;

    Serial.printf(
        "[OTA] Chunk %u/%u written (%lu/%lu bytes)\n",
        static_cast<unsigned>(sequence + 1U),
        static_cast<unsigned>(ota_total_chunks),
        static_cast<unsigned long>(ota_bytes_written),
        static_cast<unsigned long>(ota_total_size));
}

void finish_ota()
{
    if (!ota_active || ota_failed) {
        return;
    }

    if (!all_chunks_received() ||
        ota_bytes_written != ota_total_size) {
        Serial.println("[OTA ERROR] Firmware incomplete");

        Update.abort();
        ota_failed = true;
        return;
    }

    const uint32_t calculated_crc =
        ota_crc ^ 0xFFFFFFFFUL;

    if (!validate_ota_crc() ||
        calculated_crc != ota_expected_crc) {
        Serial.printf(
            "[OTA ERROR] CRC mismatch: expected 0x%08lX, got 0x%08lX\n",
            static_cast<unsigned long>(ota_expected_crc),
            static_cast<unsigned long>(calculated_crc));

        Update.abort();
        ota_failed = true;
        return;
    }

    if (!Update.end(true)) {
        Serial.print("[OTA ERROR] Update.end failed: ");
        Serial.println(Update.errorString());
        ota_failed = true;
        return;
    }

    Serial.println("[OTA SUCCESS] Firmware verified");
    Serial.println("[OTA] Rebooting into new firmware...");

    ota_active = false;

    delay(100);

    ESP.restart();
}

} // namespace

/*
 * OTA packet format
 *
 * Byte 0:
 *   Packet type (OTA_PACKET_TYPE)
 *
 * Byte 1:
 *   Operation:
 *     OTA_START = 1
 *     OTA_CHUNK = 2
 *     OTA_END   = 3
 *
 * OTA_START:
 *   Bytes 2-5:  total firmware size, little endian
 *   Bytes 6-9:  expected CRC32, little endian
 *   Bytes 10-11: total chunk count, little endian
 *
 * OTA_CHUNK:
 *   Bytes 2-3: sequence index, little endian
 *   Bytes 4+:  binary firmware payload
 *
 * OTA_END:
 *   No additional payload.
 */
void ota_updater_handle_packet(
    const uint8_t* data,
    size_t length)
{
    if (data == nullptr || length < 2) {
        return;
    }

    if (data[0] != OTA_PACKET_TYPE) {
        return;
    }

    const uint8_t operation = data[1];

    if (operation == OTA_START) {
        if (length < OTA_HEADER_SIZE) {
            Serial.println("[OTA ERROR] Invalid start packet");
            return;
        }

        const uint32_t total_size =
            static_cast<uint32_t>(data[2]) |
            (static_cast<uint32_t>(data[3]) << 8U) |
            (static_cast<uint32_t>(data[4]) << 16U) |
            (static_cast<uint32_t>(data[5]) << 24U);

        const uint32_t expected_crc =
            static_cast<uint32_t>(data[6]) |
            (static_cast<uint32_t>(data[7]) << 8U) |
            (static_cast<uint32_t>(data[8]) << 16U) |
            (static_cast<uint32_t>(data[9]) << 24U);

        const uint16_t total_chunks =
            static_cast<uint16_t>(data[10]) |
            (static_cast<uint16_t>(data[11]) << 8U);

        begin_ota(
            total_size,
            expected_crc,
            total_chunks);

        return;
    }

    if (operation == OTA_CHUNK) {
        if (length < 5) {
            Serial.println("[OTA ERROR] Invalid OTA chunk");
            return;
        }

        const uint16_t sequence =
            static_cast<uint16_t>(data[2]) |
            (static_cast<uint16_t>(data[3]) << 8U);

        process_ota_chunk(
            sequence,
            data + 4,
            length - 4);

        return;
    }

    if (operation == OTA_END) {
        finish_ota();
    }
}

/*
 * Optional helper for registering the OTA receiver with
 * EspNowDriver::set_rx_callback().
 */
void ota_updater_rx_callback(
    const uint8_t* /*src_mac*/,
    const uint8_t* data,
    size_t length,
    int8_t /*rssi*/)
{
    ota_updater_handle_packet(data, length);
}

#else

// Host-build stub so the desktop CMake build can still compile
// this source file without ESP32/Arduino libraries.
void ota_updater_handle_packet(
    const uint8_t*,
    size_t)
{
}

void ota_updater_rx_callback(
    const uint8_t*,
    const uint8_t*,
    size_t,
    int8_t)
{
}

#endif
