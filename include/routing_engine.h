#pragma once
#include <unordered_map>
#include <stdint.h>

struct RouteEntry {
    uint16_t destination_id;
    uint16_t next_hop_id;
    uint8_t hop_count;
    int8_t link_quality_rssi;
    uint32_t last_updated_ms;
};

class RoutingEngine {
private:
    uint16_t local_node_id;
    std::unordered_map<uint16_t, RouteEntry> routing_table;

public:
    explicit RoutingEngine(uint16_t node_id);

    void process_beacon(uint16_t sender_id, uint16_t dest_id, uint8_t hops, int8_t rssi, uint32_t current_time);
    bool get_next_hop(uint16_t destination_id, uint16_t& next_hop_out);
    void prune_stale_routes(uint32_t current_time, uint32_t timeout_ms = 10000);
    const std::unordered_map<uint16_t, RouteEntry>& get_table() const;
};
