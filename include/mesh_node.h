#pragma once
#include "packet_format.h"
#include <vector>
#include <unordered_map>
#include <stdint.h>

struct PeerInfo {
    uint16_t node_id;
    int8_t rssi;
    uint32_t last_seen_ms;
    uint8_t hop_count;
};

class MeshNode {
private:
    uint16_t node_id;
    uint16_t current_seq;
    uint32_t last_telemetry_ms;
    std::unordered_map<uint16_t, PeerInfo> routing_table;
    std::vector<uint16_t> seen_packets;

    bool is_duplicate(uint16_t seq);
    void update_peer(uint16_t sender_id, int8_t rssi, uint8_t hops);

public:
    MeshNode(uint16_t id);
    
    void init();
    void handle_received_packet(const uint8_t* raw_data, size_t len, int8_t rssi);
    void print_telemetry(uint32_t current_time_ms, uint32_t interval_ms);
    bool broadcast_payload(PacketType type, const uint8_t* data, uint8_t len);
    bool send_to_node(uint16_t target_id, PacketType type, const uint8_t* data, uint8_t len);
    
    void cleanup_dead_peers(uint32_t timeout_ms, uint32_t current_time_ms);
    const std::unordered_map<uint16_t, PeerInfo>& get_routing_table() const;
};
