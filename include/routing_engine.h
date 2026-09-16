#pragma once

#include <unordered_map>
#include <cstdint>
#include <cstddef>
#include <mutex>

struct RouteEntry {
    uint16_t destination_id;

    // Primary and secondary next-hop candidates
    uint16_t primary_hop;
    uint16_t backup_hop;

    // Primary route metrics
    uint8_t hop_count;
    int8_t link_quality_rssi;

    // Backup route metrics
    uint8_t backup_hop_count;
    int8_t backup_link_quality_rssi;

    uint32_t last_updated_ms;
    uint32_t backup_updated_ms;
};

class RoutingEngine {
private:
    uint16_t local_node_id;
    std::unordered_map<uint16_t, RouteEntry> routing_table;

    mutable std::mutex routing_mutex;

public:
    explicit RoutingEngine(uint16_t node_id);

    void process_beacon(
        uint16_t sender_id,
        uint16_t dest_id,
        uint8_t hops,
        int8_t rssi,
        uint32_t current_time
    );

    bool get_next_hop(
        uint16_t destination_id,
        uint16_t& next_hop_out
    );

    bool handle_route_failure(
        uint16_t destination_id,
        uint16_t failed_hop,
        uint16_t& fallback_hop_out
    );

    void prune_stale_routes(
        uint32_t current_time,
        uint32_t timeout_ms = 10000
    );

    void clear_table();

    const std::unordered_map<uint16_t, RouteEntry>& get_table() const;
};
