#pragma once

#include "packet_format.h"
#include "hal/radio_interface.h"

#include <cstdint>
#include <cstddef>
#include <unordered_map>
#include <unordered_set>

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

    IRadioDriver* radio_driver;

    std::unordered_map<uint16_t, PeerInfo> routing_table;
    std::unordered_set<uint16_t> seen_packets;

    static MeshNode* active_node;

    bool is_duplicate(uint16_t seq);

    void update_peer(
        uint16_t sender_id,
        int8_t rssi,
        uint8_t hops,
        uint32_t current_time_ms
    );

    static void radio_receive_callback(
        const uint8_t* src_mac,
        const uint8_t* data,
        size_t len,
        int8_t rssi
    );

public:
    MeshNode(uint16_t id, IRadioDriver* driver = nullptr);

    void init();

    void handle_received_packet(
        const uint8_t* raw_data,
        size_t len,
        int8_t rssi,
        uint32_t current_time_ms = 0
    );

    bool broadcast_payload(
        PacketType type,
        const uint8_t* data,
        uint8_t len
    );

    bool send_to_node(
        uint16_t target_id,
        PacketType type,
        const uint8_t* data,
        uint8_t len
    );

    void cleanup_dead_peers(
        uint32_t timeout_ms,
        uint32_t current_time_ms
    );

    const std::unordered_map<uint16_t, PeerInfo>&
    get_routing_table() const;
};
