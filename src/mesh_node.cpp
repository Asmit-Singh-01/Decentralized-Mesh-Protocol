#include "mesh_node.h"
#include <cstring>
#include <algorithm>
#include <iostream>

MeshNode::MeshNode(uint16_t id) : node_id(id), current_seq(0) {}

void MeshNode::init() {
    routing_table.clear();
    seen_packets.clear();
}

bool MeshNode::is_duplicate(uint16_t seq) {
    if (std::find(seen_packets.begin(), seen_packets.end(), seq) != seen_packets.end()) {
        return true;
    }
    if (seen_packets.size() > 50) {
        seen_packets.erase(seen_packets.begin());
    }
    seen_packets.push_back(seq);
    return false;
}

void MeshNode::update_peer(uint16_t sender_id, int8_t rssi, uint8_t hops) {
    PeerInfo& peer = routing_table[sender_id];
    peer.node_id = sender_id;
    peer.rssi = rssi;
    peer.hop_count = hops;
}

void MeshNode::handle_received_packet(const uint8_t* raw_data, size_t len, int8_t rssi) {
    MeshPacket packet;
    if (!deserialize_packet(raw_data, len, packet)) {
        return;
    }

    if (is_duplicate(packet.header.sequence_num)) {
        return;
    }

    update_peer(packet.header.sender_id, rssi, packet.header.ttl);

    if (packet.header.receiver_id == node_id || packet.header.receiver_id == 0xFFFF) {
        // Core payload processing hook for swarm intelligence
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
uint16_t MeshNode::select_best_neighbor() const {
    int8_t best_rssi = -128;
    uint16_t best_id = 0;
    if(routing_table.empty()){
        return best_id;
    }
    for (const auto& pair : routing_table) {
        const PeerInfo& peer = pair.second; 
        if (peer.rssi != 0 && peer.rssi > best_rssi) {
            best_rssi = peer.rssi;
            best_id = peer.node_id;
        }
    }
    if (best_id != 0) {
        std::cout << "[MESH] Selected route via Node ID: " << best_id
                  << " (RSSI: " << static_cast<int>(best_rssi) << " dBm)" << std::endl;
    }
    return best_id;
}