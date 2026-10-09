#pragma once

#include "packet_format.h"
#include <unordered_map>
#include <cstdint>
#include <cstddef>

struct RouteEntry {
    uint16_t destination_id;
    uint16_t next_hop_id;
    uint8_t hop_count;
    int8_t link_quality_rssi;
    uint32_t metric_cost;
    uint32_t last_updated_ms;
    uint32_t seq_num;
};

class RoutingEngine {
private:
    uint16_t local_node_id;
    std::unordered_map<uint16_t, RouteEntry> routing_table;

    uint32_t compute_metric(uint8_t hops, int8_t rssi) const;

public:
    explicit RoutingEngine(uint16_t node_id);

    void process_beacon(uint16_t sender_id, uint16_t dest_id, uint8_t hops, int8_t rssi, uint32_t current_time);
    
    // AODV Route Update Logic
    bool update_route(uint16_t dest_id, uint16_t next_hop, uint8_t hops, int8_t rssi, uint32_t seq_num, uint32_t current_time);
    
    bool get_next_hop(uint16_t destination_id, uint16_t& next_hop_out);
    void invalidate_route(uint16_t destination_id);
    void prune_stale_routes(uint32_t current_time, uint32_t timeout_ms = 10000);
    void clear_table();

    const std::unordered_map<uint16_t, RouteEntry>& get_table() const;
};
