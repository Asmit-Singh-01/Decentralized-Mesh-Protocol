#include "routing_engine.h"
#include <cmath>
#include <algorithm>

RoutingEngine::RoutingEngine(uint16_t node_id) : local_node_id(node_id) {}

uint32_t RoutingEngine::compute_metric(uint8_t hops, int8_t rssi) const {
    // Metric Formula: Lower score represents better link path quality
    int8_t abs_rssi = std::abs(rssi);
    return static_cast<uint32_t>(hops * 100 + abs_rssi);
}

void RoutingEngine::process_beacon(uint16_t sender_id, uint16_t dest_id, uint8_t hops, int8_t rssi, uint32_t current_time) {
    (void)dest_id; // Unused parameter handled safely
    update_route(sender_id, sender_id, hops + 1, rssi, 0, current_time);
}

bool RoutingEngine::update_route(uint16_t dest_id, uint16_t next_hop, uint8_t hops, int8_t rssi, uint32_t seq_num, uint32_t current_time) {
    if (dest_id == local_node_id) {
        return false;
    }

    uint32_t new_metric = compute_metric(hops, rssi);
    auto it = routing_table.find(dest_id);

    if (it == routing_table.end()) {
        // New Route Entry
        RouteEntry new_entry{dest_id, next_hop, hops, rssi, new_metric, current_time, seq_num};
        routing_table[dest_id] = new_entry;
        return true;
    }

    // AODV Route Selection Rule: Sequence Number > Metric Cost
    if (seq_num > it->second.seq_num || 
       (seq_num == it->second.seq_num && new_metric < it->second.metric_cost)) {
        it->second.next_hop_id = next_hop;
        it->second.hop_count = hops;
        it->second.link_quality_rssi = rssi;
        it->second.metric_cost = new_metric;
        it->second.last_updated_ms = current_time;
        it->second.seq_num = seq_num;
        return true;
    }

    return false;
}

bool RoutingEngine::get_next_hop(uint16_t destination_id, uint16_t& next_hop_out) {
    auto it = routing_table.find(destination_id);
    if (it != routing_table.end()) {
        next_hop_out = it->second.next_hop_id;
        return true;
    }
    return false;
}

void RoutingEngine::invalidate_route(uint16_t destination_id) {
    routing_table.erase(destination_id);
}

void RoutingEngine::prune_stale_routes(uint32_t current_time, uint32_t timeout_ms) {
    for (auto it = routing_table.begin(); it != routing_table.end(); ) {
        if ((current_time - it->second.last_updated_ms) > timeout_ms) {
            it = routing_table.erase(it);
        } else {
            ++it;
        }
    }
}

void RoutingEngine::clear_table() {
    routing_table.clear();
}

const std::unordered_map<uint16_t, RouteEntry>& RoutingEngine::get_table() const {
    return routing_table;
}

