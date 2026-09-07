#include "routing_engine.h"
#include <iostream>

RoutingEngine::RoutingEngine(uint16_t node_id) : local_node_id(node_id) {}

bool RoutingEngine::is_unsafe_hop(uint16_t candidate_hop_id, uint16_t destination_id) const {
    if (candidate_hop_id == local_node_id) {
        return true;
    }

    auto it = routing_table.find(destination_id);
    if (it != routing_table.end() && candidate_hop_id == it->second.next_hop_id) {
        return true;
    }

    return false;
}

void RoutingEngine::process_beacon(uint16_t sender_id, uint16_t dest_id, uint8_t hops, int8_t rssi, uint32_t current_time) {
    if (sender_id == local_node_id) return;

    auto it = routing_table.find(sender_id);
    if (it == routing_table.end()) {
        routing_table[sender_id] = {
            sender_id,
            sender_id,
            hops,
            rssi,
            current_time
        };
        return;
    }

    RouteEntry& entry = it->second;
    if (rssi > entry.link_quality_rssi || hops < entry.hop_count) {
        // Existing primary-promotion behavior, preserved exactly as before.
        // Backup fields are intentionally left untouched by this update.
        entry.next_hop_id = sender_id;
        entry.hop_count = hops;
        entry.link_quality_rssi = rssi;
        entry.last_updated_ms = current_time;
        return;
    }

    // Backup candidate discovery: this beacon didn't beat the primary, so see if
    // it's worth keeping as a fallback for this same key/entry.
    //
    // Pre-existing quirk (see PR description): process_beacon keys the table on
    // sender_id and dest_id is otherwise unused, so every entry here really models
    // a single direct neighbor rather than a route to a distant destination. Under
    // that per-key model, sender_id can never itself supply a *second* distinct hop
    // identity for the same key (it would always equal the existing primary hop and
    // get rejected below as a duplicate). We therefore repurpose dest_id - which
    // carries no meaning for route creation/keying today - as the proposed backup
    // hop identity for this entry. This does not change how the table is keyed or
    // how the primary hop is chosen; it only adds new backup-only bookkeeping on
    // top of the existing per-key semantics.
    uint16_t candidate_hop = dest_id;
    if (candidate_hop == NO_ROUTE) {
        return;
    }
    if (is_unsafe_hop(candidate_hop, sender_id)) {
        return;
    }

    bool backup_is_better = (entry.backup_hop_id == NO_ROUTE) ||
                             (rssi > entry.backup_link_quality_rssi) ||
                             (hops < entry.backup_hop_count);
    if (backup_is_better) {
        entry.backup_hop_id = candidate_hop;
        entry.backup_hop_count = hops;
        entry.backup_link_quality_rssi = rssi;
        entry.backup_last_updated_ms = current_time;
    }
}

bool RoutingEngine::get_next_hop(uint16_t destination_id, uint16_t& next_hop_out) {
    auto it = routing_table.find(destination_id);
    if (it != routing_table.end()) {
        next_hop_out = it->second.next_hop_id;
        return true;
    }
    return false;
}

bool RoutingEngine::get_backup_hop(uint16_t destination_id, uint16_t& backup_hop_out) const {
    auto it = routing_table.find(destination_id);
    if (it != routing_table.end() && it->second.backup_hop_id != NO_ROUTE) {
        backup_hop_out = it->second.backup_hop_id;
        return true;
    }
    return false;
}

bool RoutingEngine::report_route_failure(uint16_t destination_id, uint32_t current_time) {
    auto it = routing_table.find(destination_id);
    if (it == routing_table.end()) {
        return false;
    }

    RouteEntry& entry = it->second;
    if (entry.backup_hop_id == NO_ROUTE) {
        std::cout << "[MESH WARN] Primary route to Node " << destination_id
                  << " failed! No backup hop available." << std::endl;
        failover_stats.failed_with_no_backup++;
        return false;
    }

    uint16_t promoted_hop = entry.backup_hop_id;
    entry.next_hop_id = entry.backup_hop_id;
    entry.hop_count = entry.backup_hop_count;
    entry.link_quality_rssi = entry.backup_link_quality_rssi;
    entry.last_updated_ms = current_time;

    entry.backup_hop_id = NO_ROUTE;
    entry.backup_hop_count = 0;
    entry.backup_link_quality_rssi = INT8_MIN;
    entry.backup_last_updated_ms = 0;

    std::cout << "[MESH WARN] Primary route to Node " << destination_id
              << " failed! Switched to backup hop Node " << promoted_hop << "." << std::endl;
    failover_stats.total_failovers++;
    return true;
}

void RoutingEngine::prune_stale_routes(uint32_t current_time, uint32_t timeout_ms) {
    for (auto it = routing_table.begin(); it != routing_table.end();) {
        if (current_time - it->second.last_updated_ms > timeout_ms) {
            it = routing_table.erase(it);
            continue;
        }

        // Backup can go stale independently of the primary route.
        RouteEntry& entry = it->second;
        if (entry.backup_hop_id != NO_ROUTE &&
            current_time - entry.backup_last_updated_ms > timeout_ms) {
            entry.backup_hop_id = NO_ROUTE;
            entry.backup_hop_count = 0;
            entry.backup_link_quality_rssi = INT8_MIN;
            entry.backup_last_updated_ms = 0;
        }

        ++it;
    }
}

const std::unordered_map<uint16_t, RouteEntry>& RoutingEngine::get_table() const {
    return routing_table;
}

const FailoverStats& RoutingEngine::get_failover_stats() const {
    return failover_stats;
}
