#include "packet_format.h"
#include <cstring>

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

    size_t header_size = sizeof(PacketHeader);
    size_t payload_size = packet.header.payload_len;

    std::memcpy(buffer, &packet.header, header_size);
    std::memcpy(buffer + header_size, packet.payload, payload_size);

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
    std::memcpy(out_packet.payload, buffer + sizeof(PacketHeader), hdr->payload_len);
    out_packet.crc16 = received_crc;

    return true;
}

