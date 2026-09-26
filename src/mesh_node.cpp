#include "mesh_node.h"

#include <algorithm>
#include <cstring>

MeshNode::MeshNode(uint16_t id, IRadioDriver& driver)
    : node_id(id),
      current_seq(0),
      radio_driver(driver),
      routing_engine(id) {}

void MeshNode::init() {
    routing_table.clear();
    seen_packets.clear();
}

bool MeshNode::is_duplicate(uint16_t seq) {
    if (seen_packets.find(seq) != seen_packets.end()) {
        return true;
    }

    if (seen_packets.size() >= 100) {
        seen_packets.erase(seen_packets.begin());
    }

    seen_packets.insert(seq);
    return false;
}

void MeshNode::update_peer(
    uint16_t sender_id,
    const uint8_t* mac_address,
    int8_t rssi,
    uint8_t hops,
    uint32_t current_time_ms
) {
    if (sender_id == node_id || mac_address == nullptr) {
        return;
    }

    PeerInfo& peer = routing_table[sender_id];

    peer.node_id = sender_id;
    std::copy(mac_address, mac_address + 6, peer.mac_address.begin());
    peer.rssi = rssi;
    peer.hop_count = hops;
    peer.last_seen_ms = current_time_ms;
}

void MeshNode::handle_received_packet(
    const uint8_t* mac_address,
    const uint8_t* raw_data,
    size_t len,
    int8_t rssi,
    uint32_t current_time_ms
) {
    if (raw_data == nullptr || mac_address == nullptr) {
        return;
    }

    MeshPacket packet{};

    if (!deserialize_packet(raw_data, len, packet)) {
        return;
    }

    if (packet.header.sender_id == node_id) {
        return;
    }

    if (is_duplicate(packet.header.sequence_num)) {
        return;
    }

    update_peer(
        packet.header.sender_id,
        mac_address,
        rssi,
        packet.header.ttl,
        current_time_ms
    );

    if (packet.header.receiver_id == node_id ||
        packet.header.receiver_id == 0xFFFF) {
        // Packet is for this node or broadcast.
        // Application payload handling can be added here.
        return;
    }

    // Multi-hop forwarding is not implemented in this step.
}

bool MeshNode::broadcast_payload(
    PacketType type,
    const uint8_t* data,
    uint8_t len
) {
    if (len > MAX_PAYLOAD_SIZE || (len > 0 && data == nullptr)) {
        return false;
    }

    MeshPacket packet{};

    packet.header.magic = PROTOCOL_MAGIC_BYTE;
    packet.header.type = static_cast<uint8_t>(type);
    packet.header.sender_id = node_id;
    packet.header.receiver_id = 0xFFFF;
    packet.header.sequence_num = ++current_seq;
    packet.header.ttl = 5;
    packet.header.payload_len = len;

    if (len > 0) {
        std::memcpy(packet.payload, data, len);
    }

    uint8_t buffer[256]{};
    size_t serialized_len = 0;

    if (!serialize_packet(packet, buffer, serialized_len)) {
        return false;
    }

    // nullptr means broadcast destination for the driver.
    return radio_driver.send_bytes(nullptr, buffer, serialized_len);
}

bool MeshNode::send_to_node(
    uint16_t target_id,
    PacketType type,
    const uint8_t* data,
    uint8_t len
) {
    if (len > MAX_PAYLOAD_SIZE || (len > 0 && data == nullptr)) {
        return false;
    }

    auto peer_it = routing_table.find(target_id);
    if (peer_it == routing_table.end()) {
        return false;
    }

    MeshPacket packet{};

    packet.header.magic = PROTOCOL_MAGIC_BYTE;
    packet.header.type = static_cast<uint8_t>(type);
    packet.header.sender_id = node_id;
    packet.header.receiver_id = target_id;
    packet.header.sequence_num = ++current_seq;
    packet.header.ttl = 5;
    packet.header.payload_len = len;

    if (len > 0) {
        std::memcpy(packet.payload, data, len);
    }

    uint8_t buffer[256]{};
    size_t serialized_len = 0;

    if (!serialize_packet(packet, buffer, serialized_len)) {
        return false;
    }

    return radio_driver.send_bytes(
        peer_it->second.mac_address.data(),
        buffer,
        serialized_len
    );
}

void MeshNode::cleanup_dead_peers(
    uint32_t timeout_ms,
    uint32_t current_time_ms
) {
    for (auto it = routing_table.begin(); it != routing_table.end();) {
        if (current_time_ms - it->second.last_seen_ms > timeout_ms) {
            it = routing_table.erase(it);
        } else {
            ++it;
        }
    }
}

const std::unordered_map<uint16_t, PeerInfo>&
MeshNode::get_routing_table() const {
    return routing_table;
}
