#include "mesh_node.h"
#include "packet_format.h"
#include <iostream>
#include <cstring>

MeshNode::MeshNode(uint16_t id) : node_id(id), routing_engine(id), current_seq(0) {}

bool MeshNode::init() {
    std::cout << "Initializing MeshNode with ID: 0x" << std::hex << node_id << std::endl;
    routing_engine.clear_table();
    tx_ring_buffer.clear();
    rx_ring_buffer.clear();
    return true;
}

bool MeshNode::queue_tx_packet(const MeshPacket& packet) {
    return tx_ring_buffer.push(packet);
}

bool MeshNode::dequeue_tx_packet(MeshPacket& packet_out) {
    return tx_ring_buffer.pop(packet_out);
}

bool MeshNode::send_data(uint16_t target_id, const uint8_t* data, size_t len) {
    if (!data || len == 0 || len > MAX_PAYLOAD_SIZE) {
        return false;
    }

    MeshPacket packet;
    packet.header.version = 1;
    packet.header.type = PacketType::DATA;
    packet.header.src_id = node_id;
    packet.header.dest_id = target_id;
    
    uint16_t next_hop = 0;
    if (routing_engine.get_next_hop(target_id, next_hop)) {
        packet.header.next_hop_id = next_hop;
    } else {
        packet.header.next_hop_id = target_id;
    }

    packet.header.seq_num = ++current_seq;
    packet.header.ttl = 10;
    packet.header.payload_len = static_cast<uint8_t>(len);

    std::memcpy(packet.payload, data, len);

    // Push into zero-copy static TX ring buffer
    if (!queue_tx_packet(packet)) {
        std::cerr << "[Error] Node 0x" << std::hex << node_id << " TX Ring Buffer Full!" << std::endl;
        return false;
    }

    MeshPacket packet_to_send;
    if (dequeue_tx_packet(packet_to_send)) {
        uint8_t serialized_buffer[256];
        size_t serialized_len = 0;

        if (serialize_packet(packet_to_send, serialized_buffer, serialized_len)) {
            std::cout << "Node 0x" << std::hex << node_id 
                      << " transmitting buffered packet to target 0x" << target_id << std::endl;
            return true;
        }
    }

    return false;
}

void MeshNode::process_incoming(const uint8_t* raw_buffer, size_t buffer_len, int8_t rssi) {
    MeshPacket packet;
    if (!deserialize_packet(raw_buffer, buffer_len, packet)) {
        std::cerr << "Node 0x" << std::hex << node_id << " packet deserialization failed!" << std::endl;
        return;
    }

    rx_ring_buffer.push(packet);

    MeshPacket rx_packet;
    if (rx_ring_buffer.pop(rx_packet)) {
        routing_engine.process_beacon(rx_packet.header.src_id, rx_packet.header.dest_id, 1, rssi, 0);

        if (rx_packet.header.dest_id == node_id) {
            std::cout << "Node 0x" << std::hex << node_id 
                      << " successfully received packet intended for self!" << std::endl;
        }
    }
}
