#pragma once

#include "routing_engine.h"
#include "ring_buffer.h"
#include <cstdint>
#include <cstddef>

class MeshNode {
private:
    uint16_t node_id;
    RoutingEngine routing_engine;
    PacketRingBuffer<64> tx_ring_buffer;
    PacketRingBuffer<64> rx_ring_buffer;
    uint32_t current_seq;

public:
    explicit MeshNode(uint16_t id);

    bool init();
    bool send_data(uint16_t target_id, const uint8_t* data, size_t len);
    void process_incoming(const uint8_t* raw_buffer, size_t buffer_len, int8_t rssi);

    // Buffer management
    bool queue_tx_packet(const MeshPacket& packet);
    bool dequeue_tx_packet(MeshPacket& packet_out);

    uint16_t get_node_id() const { return node_id; }
};
