#pragma once
#include <unordered_map>
#include <stdint.h>

// Sentinel value meaning "no route / no hop assigned".
static constexpr uint16_t NO_ROUTE = 0xFFFF;

struct RouteEntry {
    uint16_t destination_id;
    uint16_t next_hop_id;
    uint8_t hop_count;
    int8_t link_quality_rssi;
    uint32_t last_updated_ms;

    // Secondary/failover candidate for this destination. Populated opportunistically
    // by process_beacon() and promoted into the primary slot by report_route_failure().
    uint16_t backup_hop_id = NO_ROUTE;
    uint8_t backup_hop_count = 0;
    int8_t backup_link_quality_rssi = INT8_MIN;
    uint32_t backup_last_updated_ms = 0;
};

// Running counters for the fallback/failover mechanism (see report_route_failure()).
struct FailoverStats {
    uint32_t total_failovers = 0;
    uint32_t failed_with_no_backup = 0;
};

class RoutingEngine {
private:
    uint16_t local_node_id;
    std::unordered_map<uint16_t, RouteEntry> routing_table;
    FailoverStats failover_stats;

    // Loop-avoidance guard used before accepting a hop as primary or backup.
    // At minimum rejects routing back through ourselves, and rejects a backup
    // candidate that duplicates the entry's current primary hop. This does NOT
    // provide full loop-freedom (that would need sequence numbers / split-horizon,
    // which is out of scope here) - it only catches the locally-detectable case.
    bool is_unsafe_hop(uint16_t candidate_hop_id, uint16_t destination_id) const;

public:
    explicit RoutingEngine(uint16_t node_id);

    // NOTE: this class does no internal locking/synchronization. It is used
    // synchronously today; callers must serialize access if that ever changes.
    void process_beacon(uint16_t sender_id, uint16_t dest_id, uint8_t hops, int8_t rssi, uint32_t current_time);
    bool get_next_hop(uint16_t destination_id, uint16_t& next_hop_out);
    bool get_backup_hop(uint16_t destination_id, uint16_t& backup_hop_out) const;

    // Call this when a send to `destination_id` via its current primary hop has
    // failed (e.g. an ACK timeout in a future send path). If a backup hop is on
    // file it is promoted to primary and the backup slot is cleared (it will be
    // repopulated from future beacons). Returns false if there is no entry, or
    // no backup was available to fail over to.
    bool report_route_failure(uint16_t destination_id, uint32_t current_time);

    void prune_stale_routes(uint32_t current_time, uint32_t timeout_ms = 10000);
    const std::unordered_map<uint16_t, RouteEntry>& get_table() const;
    const FailoverStats& get_failover_stats() const;
};
