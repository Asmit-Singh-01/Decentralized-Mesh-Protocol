#include "packet_format.h"
#include <cstring>

uint16_t calculate_crc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; ++j) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

bool serialize_packet(const MeshPacket& packet, uint8_t* buffer, size_t& buffer_len) {
    if (!buffer) return false;

    size_t total_size = sizeof(MeshHeader) + packet.header.payload_len;
    if (total_size > (sizeof(MeshHeader) + MAX_PAYLOAD_SIZE)) return false;

    std::memcpy(buffer, &packet.header, sizeof(MeshHeader));
    if (packet.header.payload_len > 0) {
        std::memcpy(buffer + sizeof(MeshHeader), packet.payload, packet.header.payload_len);
    }

    uint16_t crc = calculate_crc16(buffer, total_size);
    std::memcpy(buffer + total_size, &crc, sizeof(uint16_t));

    buffer_len = total_size + sizeof(uint16_t);
    return true;
}

bool deserialize_packet(const uint8_t* buffer, size_t buffer_len, MeshPacket& packet) {
    if (!buffer || buffer_len < (sizeof(MeshHeader) + sizeof(uint16_t))) {
        return false;
    }

    size_t payload_and_header_len = buffer_len - sizeof(uint16_t);
    uint16_t calculated_crc = calculate_crc16(buffer, payload_and_header_len);
    
    uint16_t received_crc = 0;
    std::memcpy(&received_crc, buffer + payload_and_header_len, sizeof(uint16_t));

    if (calculated_crc != received_crc) {
        return false;
    }

    std::memcpy(&packet.header, buffer, sizeof(MeshHeader));
    if (packet.header.payload_len > MAX_PAYLOAD_SIZE || 
        payload_and_header_len != (sizeof(MeshHeader) + packet.header.payload_len)) {
        return false;
    }

    if (packet.header.payload_len > 0) {
        std::memcpy(packet.payload, buffer + sizeof(MeshHeader), packet.header.payload_len);
    }

    packet.crc16 = received_crc;
    return true;
}
