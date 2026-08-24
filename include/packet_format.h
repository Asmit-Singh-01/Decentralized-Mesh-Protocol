#pragma once
#include <stdint.h>
#include <stddef.h>

#define MAX_PAYLOAD_SIZE 64
#define PROTOCOL_MAGIC_BYTE 0xD7

enum class PacketType : uint8_t {
    BEACON = 0x01,
    HEARTBEAT = 0x02,
    ROUTING_TABLE = 0x03,
    TELEMETRY_SWARM = 0x04,
    TASK_ALLOCATION = 0x05,
    ACK = 0x06
};

#pragma pack(push, 1)
struct PacketHeader {
    uint8_t magic;
    uint8_t type;
    uint16_t sender_id;
    uint16_t receiver_id;
    uint16_t sequence_num;
    uint8_t ttl;
    uint8_t payload_len;
};

struct MeshPacket {
    PacketHeader header;
    uint8_t payload[MAX_PAYLOAD_SIZE];
    uint16_t crc16;
};
#pragma pack(pop)

uint16_t calculate_crc16(const uint8_t* data, size_t length);
bool serialize_packet(const MeshPacket& packet, uint8_t* buffer, size_t& out_len);
bool deserialize_packet(const uint8_t* buffer, size_t length, MeshPacket& out_packet);

