#include "packet_format.h"
#include "compression.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

uint16_t calculate_crc16(const uint8_t* data, size_t length)
{
    if (data == nullptr || length == 0) {
        return 0;
    }

    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < length; ++i) {
        crc ^= static_cast<uint16_t>(data[i]) << 8;

        for (int bit = 0; bit < 8; ++bit) {
            if (crc & 0x8000U) {
                crc = static_cast<uint16_t>(
                    (crc << 1) ^ 0x1021U
                );
            } else {
                crc = static_cast<uint16_t>(crc << 1);
            }
        }
    }

    return crc;
}

bool serialize_packet(
    const MeshPacket& packet,
    uint8_t* buffer,
    size_t& out_len)
{
    if (buffer == nullptr ||
        packet.header.payload_len > MAX_PAYLOAD_SIZE) {
        return false;
    }

    PacketHeader header = packet.header;

    const size_t original_size = packet.header.payload_len;
    const uint8_t* payload = packet.payload;
    size_t payload_size = original_size;

    uint8_t compressed_payload[MAX_PAYLOAD_SIZE] = {};
    bool compressed = false;

    /*
     * Automatically compress swarm telemetry when:
     * - the packet is telemetry
     * - it is not already marked as compressed
     * - the payload contains complete int16_t samples
     * - compression actually makes the payload smaller
     */
    if (header.type ==
            static_cast<uint8_t>(PacketType::TELEMETRY_SWARM) &&
        (header.flags & PACKET_FLAG_COMPRESSED) == 0 &&
        original_size > 0 &&
        original_size <= MAX_PAYLOAD_SIZE &&
        (original_size % sizeof(int16_t)) == 0) {

        int16_t samples[MAX_PAYLOAD_SIZE / sizeof(int16_t)] = {};

        std::memcpy(
            samples,
            packet.payload,
            original_size
        );

        const size_t sample_count =
            original_size / sizeof(int16_t);

        const size_t compressed_size =
            compress_telemetry(
                samples,
                sample_count,
                compressed_payload
            );

        if (compressed_size > 0 &&
            compressed_size < original_size &&
            compressed_size <= MAX_PAYLOAD_SIZE) {

            payload = compressed_payload;
            payload_size = compressed_size;
            header.flags |= PACKET_FLAG_COMPRESSED;
            compressed = true;

            const size_t saved_percent =
                ((original_size - compressed_size) * 100) /
                original_size;

            std::printf(
                "[MESH COMPRESS] Original: %zuB -> "
                "Compressed: %zuB (%zu%% saved)\n",
                original_size,
                compressed_size,
                saved_percent
            );
        } else {
            header.flags &= static_cast<uint8_t>(
                ~PACKET_FLAG_COMPRESSED
            );
        }
    }

    header.payload_len =
        static_cast<uint8_t>(payload_size);

    const size_t header_size = sizeof(PacketHeader);

    std::memcpy(
        buffer,
        &header,
        header_size
    );

    if (payload_size > 0) {
        std::memcpy(
            buffer + header_size,
            payload,
            payload_size
        );
    }

    const uint16_t crc =
        calculate_crc16(
            buffer,
            header_size + payload_size
        );

    std::memcpy(
        buffer + header_size + payload_size,
        &crc,
        sizeof(uint16_t)
    );

    out_len =
        header_size +
        payload_size +
        sizeof(uint16_t);

    (void)compressed;

    return true;
}

bool deserialize_packet(
    const uint8_t* buffer,
    size_t length,
    MeshPacket& out_packet)
{
    const size_t min_size =
        sizeof(PacketHeader) + sizeof(uint16_t);

    if (buffer == nullptr || length < min_size) {
        return false;
    }

    PacketHeader header;

    std::memcpy(
        &header,
        buffer,
        sizeof(PacketHeader)
    );

    if (header.magic != PROTOCOL_MAGIC_BYTE) {
        return false;
    }

    if (header.payload_len > MAX_PAYLOAD_SIZE) {
        return false;
    }

    const size_t expected_len =
        sizeof(PacketHeader) +
        header.payload_len +
        sizeof(uint16_t);

    if (length < expected_len) {
        return false;
    }

    uint16_t received_crc = 0;

    std::memcpy(
        &received_crc,
        buffer + sizeof(PacketHeader) +
            header.payload_len,
        sizeof(uint16_t)
    );

    const uint16_t computed_crc =
        calculate_crc16(
            buffer,
            sizeof(PacketHeader) +
                header.payload_len
        );

    if (received_crc != computed_crc) {
        return false;
    }

    out_packet.header = header;
    out_packet.crc16 = received_crc;

    const uint8_t* wire_payload =
        buffer + sizeof(PacketHeader);

    /*
     * Decompress telemetry payloads after CRC validation.
     */
    if ((header.flags & PACKET_FLAG_COMPRESSED) != 0) {

        if (header.type !=
            static_cast<uint8_t>(
                PacketType::TELEMETRY_SWARM)) {
            return false;
        }

        int16_t samples[MAX_PAYLOAD_SIZE /
                        sizeof(int16_t)] = {};

        const size_t sample_count =
            decompress_telemetry(
                wire_payload,
                header.payload_len,
                samples,
                MAX_PAYLOAD_SIZE /
                    sizeof(int16_t)
            );

        if (sample_count == 0) {
            return false;
        }

        const size_t decompressed_size =
            sample_count * sizeof(int16_t);

        if (decompressed_size > MAX_PAYLOAD_SIZE) {
            return false;
        }

        std::memcpy(
            out_packet.payload,
            samples,
            decompressed_size
        );

        out_packet.header.payload_len =
            static_cast<uint8_t>(
                decompressed_size
            );

        /*
         * The packet is now represented in decoded form,
         * so clear the wire-level compression flag.
         */
        out_packet.header.flags &=
            static_cast<uint8_t>(
                ~PACKET_FLAG_COMPRESSED
            );

        return true;
    }

    if (header.payload_len > 0) {
        std::memcpy(
            out_packet.payload,
            wire_payload,
            header.payload_len
        );
    }

    return true;
}
