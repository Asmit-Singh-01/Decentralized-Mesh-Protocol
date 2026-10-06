#pragma once

#include <cstdint>
#include <cstddef>

constexpr size_t MAX_PAYLOAD_SIZE = 200;

enum class PacketType : uint8_t {
    BEACON      = 0x01,
    DATA        = 0x02,
    TASK        = 0x03,
    ACK         = 0x04,
    ROUTE_REQ   = 0x05, // AODV Route Discovery Request
    ROUTE_REP   = 0x06, // AODV Route Discovery Reply
    ROUTE_ERR   = 0x07  // Link Failure Notification
};

struct MeshHeader {
    uint8_t  version;
    PacketType type;
    uint16_t src_id;
    uint16_t dest_id;
    uint16_t next_hop_id;
    uint16_t seq_num;
    uint8_t  ttl;
    uint8_t  payload_len;
} __attribute__((packed));

struct RouteDiscoveryPayload {
    uint16_t target_dest_id;
    uint32_t dest_seq_num;
    uint16_t metric_cost;
} __attribute__((packed));

struct MeshPacket {
    MeshHeader header;
    uint8_t payload[MAX_PAYLOAD_SIZE];
    uint16_t crc16;
};

bool serialize_packet(const MeshPacket& packet, uint8_t* buffer, size_t& buffer_len);
bool deserialize_packet(const uint8_t* buffer, size_t buffer_len, MeshPacket& packet);
uint16_t calculate_crc16(const uint8_t* data, size_t len);
