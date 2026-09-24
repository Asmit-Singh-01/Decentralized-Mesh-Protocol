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

const std::unordered_map<uint16_t, RouteEntry>& RoutingEngine::get_table() const {
    return routing_table;
}
#include <iostream>
#include <vector>
#include "../include/routing_engine.h"

std::vector<NeighborNode> active_neighbors;

// 1. Send periodic heartbeat broadcast
void check_and_send_heartbeat(uint32_t current_millis) {
    static uint32_t last_heartbeat_ms = 0;
    if (current_millis - last_heartbeat_ms >= HEARTBEAT_INTERVAL_MS) {
        last_heartbeat_ms = current_millis;
        // Logic to transmit lightweight HEARTBEAT packet
    }
}

// 2. Update node timestamp on packet receipt
void update_neighbor_timestamp(uint8_t node_id, uint32_t current_millis) {
    for (auto& neighbor : active_neighbors) {
        if (neighbor.node_id == node_id) {
            neighbor.last_seen_ms = current_millis;
            neighbor.is_active = true;
            return;
        }
    }
    active_neighbors.push_back({node_id, current_millis, true});
}

// 3. Background cleanup function to purge inactive nodes
void purge_stale_nodes(uint32_t current_millis) {
    auto it = active_neighbors.begin();
    while (it != active_neighbors.end()) {
        if (current_millis - it->last_seen_ms > TIMEOUT_THRESHOLD_MS) {
            std::cout << "[MESH SWARM] Node ID " << static_cast<int>(it->node_id) 
                      << " timed out and purged. Routing table updated." << std::endl;
            it = active_neighbors.erase(it);
        } else {
            ++it;
        }
    }
}
