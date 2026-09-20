#include "routing_engine.h"
#include <algorithm>

RoutingEngine::RoutingEngine(uint16_t node_id) : local_node_id(node_id) {}

void RoutingEngine::process_beacon(uint16_t sender_id, uint16_t dest_id, uint8_t hops, int8_t rssi, uint32_t current_time) {
    if (sender_id == local_node_id) return;

    auto it = routing_table.find(sender_id);
    
    // Composite link metric (higher is better): dynamic weight based on RSSI and hop count
    int32_t new_metric = static_cast<int32_t>(rssi) - (static_cast<int32_t>(hops) * 10);
    
    if (it == routing_table.end()) {
        routing_table[sender_id] = {
            sender_id,
            sender_id,
            hops,
            rssi,
            current_time
        };
    } else {
        int32_t existing_metric = static_cast<int32_t>(it->second.link_quality_rssi) - (static_cast<int32_t>(it->second.hop_count) * 10);
        
        // Update route if metric is superior or if route is refreshed from same path
        if (new_metric >= existing_metric || it->second.next_hop_id == sender_id) {
            it->second.next_hop_id = sender_id;
            it->second.hop_count = hops;
            it->second.link_quality_rssi = rssi;
            it->second.last_updated_ms = current_time;
        }
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
        uint32_t elapsed = (current_time >= it->second.last_updated_ms) ? 
                           (current_time - it->second.last_updated_ms) : 
                           (0xFFFFFFFF - it->second.last_updated_ms + current_time);
                           
        if (elapsed > timeout_ms) {
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

