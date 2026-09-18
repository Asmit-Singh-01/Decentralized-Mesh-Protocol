#include "mesh_node.h"
#include "radio_driver.h"
#include <cstring>
#include <algorithm>

MeshNode::MeshNode(uint16_t id, IRadioDriver* driver) 
    : node_id(id), current_seq(0), radio_driver(driver) {}

void MeshNode::set_radio_driver(IRadioDriver* driver) {
    radio_driver = driver;
}

IRadioDriver* MeshNode::get_radio_driver() const {
    return radio_driver;
}

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

    if (radio_driver) {
        uint8_t buffer[sizeof(PacketHeader) + MAX_PAYLOAD_SIZE + sizeof(uint16_t)];
        size_t out_len = 0;
        if (serialize_packet(packet, buffer, out_len)) {
            radio_driver->send_bytes(nullptr, buffer, out_len);
        }
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

    if (radio_driver) {
        uint8_t buffer[sizeof(PacketHeader) + MAX_PAYLOAD_SIZE + sizeof(uint16_t)];
        size_t out_len = 0;
        if (serialize_packet(packet, buffer, out_len)) {
            uint8_t target_mac[6] = {0x00, 0x00, static_cast<uint8_t>(target_id >> 8), static_cast<uint8_t>(target_id & 0xFF), 0x00, 0x00};
            radio_driver->send_bytes(target_mac, buffer, out_len);
        }
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
