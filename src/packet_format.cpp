#include "packet_format.h"
#include "compression.h"
#include <cstring>
#include <cstdio>

uint16_t calculate_crc16(const uint8_t* data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; ++j) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

bool serialize_packet(const MeshPacket& packet, uint8_t* buffer, size_t& out_len) {
    if (!buffer || packet.header.payload_len > MAX_PAYLOAD_SIZE) {
        return false;
    }

    PacketHeader hdr = packet.header;
    const uint8_t* payload_src = packet.payload;
    size_t payload_size = hdr.payload_len;

    uint8_t compressed_buf[MAX_PAYLOAD_SIZE] = {0};

    // Automatically compress swarm telemetry if output is strictly smaller than original data
    if (hdr.type == static_cast<uint8_t>(PacketType::TELEMETRY_SWARM) &&
        (hdr.is_compressed == 0) &&
        (payload_size >= sizeof(int16_t)) &&
        (payload_size % sizeof(int16_t) == 0)) {

        int16_t samples[MAX_PAYLOAD_SIZE / sizeof(int16_t)] = {0};
        size_t sample_count = payload_size / sizeof(int16_t);
        std::memcpy(samples, packet.payload, payload_size);

        size_t compressed_len = compress_telemetry(samples, sample_count, compressed_buf, MAX_PAYLOAD_SIZE);

        if (compressed_len > 0 && compressed_len < payload_size) {
            payload_src = compressed_buf;
            payload_size = compressed_len;
            hdr.is_compressed = 1;
            hdr.payload_len = static_cast<uint8_t>(compressed_len);

            print_compression_benchmark(packet.header.payload_len, compressed_len);
        } else {
            hdr.is_compressed = 0;
        }
    }

    size_t header_size = sizeof(PacketHeader);

    std::memcpy(buffer, &hdr, header_size);
    if (payload_size > 0 && payload_src) {
        std::memcpy(buffer + header_size, payload_src, payload_size);
    }

    uint16_t crc = calculate_crc16(buffer, header_size + payload_size);
    std::memcpy(buffer + header_size + payload_size, &crc, sizeof(uint16_t));

    out_len = header_size + payload_size + sizeof(uint16_t);
    return true;
}

bool deserialize_packet(const uint8_t* buffer, size_t length, MeshPacket& out_packet) {
    size_t min_size = sizeof(PacketHeader) + sizeof(uint16_t);
    if (!buffer || length < min_size) {
        return false;
    }

    const PacketHeader* hdr = reinterpret_cast<const PacketHeader*>(buffer);
    if (hdr->magic != PROTOCOL_MAGIC_BYTE) {
        return false;
    }

    size_t expected_len = sizeof(PacketHeader) + hdr->payload_len + sizeof(uint16_t);
    if (length < expected_len || hdr->payload_len > MAX_PAYLOAD_SIZE) {
        return false;
    }

    uint16_t received_crc = 0;
    std::memcpy(&received_crc, buffer + sizeof(PacketHeader) + hdr->payload_len, sizeof(uint16_t));

    uint16_t computed_crc = calculate_crc16(buffer, sizeof(PacketHeader) + hdr->payload_len);
    if (received_crc != computed_crc) {
        return false;
    }

    std::memcpy(&out_packet.header, buffer, sizeof(PacketHeader));
    out_packet.crc16 = received_crc;

    const uint8_t* wire_payload = buffer + sizeof(PacketHeader);

    // Decompress wire payload if header compression flag is present
    if (out_packet.header.is_compressed) {
        if (out_packet.header.type == static_cast<uint8_t>(PacketType::TELEMETRY_SWARM)) {
            int16_t samples[MAX_PAYLOAD_SIZE / sizeof(int16_t)] = {0};
            size_t decoded_samples = decompress_telemetry(wire_payload, out_packet.header.payload_len,
                                                          samples, MAX_PAYLOAD_SIZE / sizeof(int16_t));
            if (decoded_samples == 0) {
                return false;
            }

            size_t decompressed_size = decoded_samples * sizeof(int16_t);
            if (decompressed_size > MAX_PAYLOAD_SIZE) {
                return false;
            }

            std::memcpy(out_packet.payload, samples, decompressed_size);
            out_packet.header.payload_len = static_cast<uint8_t>(decompressed_size);
            out_packet.header.is_compressed = 0; // Payload now in uncompressed representation
            return true;
        }
        return false;
    }

    if (hdr->payload_len > 0) {
        std::memcpy(out_packet.payload, wire_payload, hdr->payload_len);
    }

    return true;
}

