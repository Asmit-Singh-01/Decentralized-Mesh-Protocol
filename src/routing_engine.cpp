#include "routing_engine.h"

RoutingEngine::RoutingEngine(uint16_t node_id) : local_node_id(node_id) {}

void RoutingEngine::process_beacon(uint16_t sender_id, uint16_t dest_id, uint8_t hops, int8_t rssi, uint32_t current_time) {
    if (sender_id == local_node_id) return;

    auto it = routing_table.find(sender_id);
    if (it == routing_table.end() || rssi > it->second.link_quality_rssi || hops < it->second.hop_count) {
        routing_table[sender_id] = {
            sender_id,
            sender_id,
            hops,
            rssi,
            current_time
        };
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

void RoutingEngine::prune_stale_routes(uint32_t current_time, uint32_t timeout_ms) {
    for (auto it = routing_table.begin(); it != routing_table.end();) {
        if (current_time - it->second.last_updated_ms > timeout_ms) {
            it = routing_table.erase(it);
        } else {
            ++it;
        }
    }
}

const std::unordered_map<uint16_t, RouteEntry>& RoutingEngine::get_table() const {
    return routing_table;
}
