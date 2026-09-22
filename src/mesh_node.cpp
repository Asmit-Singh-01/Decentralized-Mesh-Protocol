#include "mesh_node.h"
#include "sd_logger.h"
#include <cstring>
#include <algorithm>

MeshNode::MeshNode(uint16_t id) : node_id(id), current_seq(0) {}

void MeshNode::init() {

    routing_table.clear();

    seen_packets.clear();

#ifdef ARDUINO
    // Set the SD card CS pin according to the ESP32 hardware wiring.
    // The logger remains disabled if initialization fails.
    constexpr uint8_t SD_CS_PIN = 5;
    sd_logger_init(SD_CS_PIN);
#endif

}

bool MeshNode::is_duplicate(uint16_t seq) {
    if (seen_packets.find(seq) != seen_packets.end()) {
        return true;
    }
    
    // Maintain fixed window size for sequence history to prevent memory leaks
    if (seen_packets.size() >= 100) {
        seen_packets.erase(seen_packets.begin());
    }
    seen_packets.insert(seq);
    return false;
}

void MeshNode::update_peer(uint16_t sender_id, int8_t rssi, uint8_t hops, uint32_t current_time_ms) {
    PeerInfo& peer = routing_table[sender_id];
    peer.node_id = sender_id;
    peer.rssi = rssi;
    peer.hop_count = hops;
    peer.last_seen_ms = current_time_ms;
}

void MeshNode::handle_received_packet(const uint8_t* raw_data, size_t len, int8_t rssi, uint32_t current_time_ms) {
    MeshPacket packet;
    if (!deserialize_packet(raw_data, len, packet)) {
        return;
    }

    if (is_duplicate(packet.header.sequence_num)) {
        return;
    }
update_peer(packet.header.sender_id, rssi, packet.header.ttl, current_time_ms);

// Log every valid, non-duplicate packet received by the mesh node.
sd_logger_log_packet(
    current_time_ms,
    packet.header.sender_id,
    packet.header.receiver_id,
    rssi,
    packet.header.payload_len,
    true
);

if (packet.header.receiver_id == node_id || packet.header.receiver_id == 0xFFFF) {
        // Core payload processing hook for swarm intelligence
    } else if (packet.header.ttl > 1) {
        // Dynamic multi-hop mesh forwarding (decrement TTL and relay)
        packet.header.ttl--;
    }
}

bool MeshNode::broadcast_payload(PacketType type, const uint8_t* data, uint8_t len) {
    if (len > MAX_PAYLOAD_SIZE) return false;

    MeshPacket packet;
    packet.header.magic = PROTOCOL_MAGIC_BYTE;
    packet.header.type = static_cast<uint8_t>(type);
    packet.header.sender_id = node_id;
    packet.header.receiver_id = 0xFFFF;
    packet.header.sequence_num = ++current_seq;
    packet.header.ttl = 5;
    packet.header.payload_len = len;

    if (data && len > 0) {
        std::memcpy(packet.payload, data, len);
    }

    return true;
}

bool MeshNode::send_to_node(uint16_t target_id, PacketType type, const uint8_t* data, uint8_t len) {
    if (len > MAX_PAYLOAD_SIZE) return false;

    MeshPacket packet;
    packet.header.magic = PROTOCOL_MAGIC_BYTE;
    packet.header.type = static_cast<uint8_t>(type);
    packet.header.sender_id = node_id;
    packet.header.receiver_id = target_id;
    packet.header.sequence_num = ++current_seq;
    packet.header.ttl = 5;
    packet.header.payload_len = len;

    if (data && len > 0) {
        std::memcpy(packet.payload, data, len);
    }

    return true;
}

void MeshNode::cleanup_dead_peers(uint32_t timeout_ms, uint32_t current_time_ms) {
    for (auto it = routing_table.begin(); it != routing_table.end();) {
        if (current_time_ms - it->second.last_seen_ms > timeout_ms) {
            it = routing_table.erase(it);
        } else {
            ++it;
        }
    }
}

const std::unordered_map<uint16_t, PeerInfo>& MeshNode::get_routing_table() const {
    return routing_table;
}
