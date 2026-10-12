#pragma once

#include "routing_engine.h"
#include <cstdint>
#include <cstddef>

class MeshNode {
private:
    uint16_t node_id;
    RoutingEngine routing_engine;
    uint32_t current_seq;

public:
    explicit MeshNode(uint16_t id);

    bool init();
    bool send_data(uint16_t target_id, const uint8_t* data, size_t len);
    void process_incoming(const uint8_t* raw_buffer, size_t buffer_len, int8_t rssi);

    uint16_t get_node_id() const { return node_id; }
};

