#include "routing_engine.h"
#include <cstdio>

RoutingEngine::RoutingEngine(uint16_t node_id)
    : local_node_id(node_id) {}

void RoutingEngine::process_beacon(
    uint16_t sender_id,
    uint16_t dest_id,
    uint8_t hops,
    int8_t rssi,
    uint32_t current_time) {

    // Ignore beacons originating from this node.
    if (sender_id == local_node_id) {
        return;
    }

    std::lock_guard<std::mutex> lock(routing_mutex);

    auto it = routing_table.find(dest_id);

    // Higher metric is better.
    int32_t new_metric =
        static_cast<int32_t>(rssi) -
        (static_cast<int32_t>(hops) * 10);

    // Create the first route for this destination.
    if (it == routing_table.end()) {
        RouteEntry entry{
            dest_id,
            sender_id,
            0,
            hops,
            rssi,
            0,
            0,
            current_time,
            0
        };

        routing_table[dest_id] = entry;
        return;
    }

    RouteEntry& route = it->second;

    int32_t primary_metric =
        static_cast<int32_t>(route.link_quality_rssi) -
        (static_cast<int32_t>(route.hop_count) * 10);

    int32_t backup_metric =
        static_cast<int32_t>(route.backup_link_quality_rssi) -
        (static_cast<int32_t>(route.backup_hop_count) * 10);

    // Refresh the existing primary route.
    if (sender_id == route.primary_hop) {
        route.hop_count = hops;
        route.link_quality_rssi = rssi;
        route.last_updated_ms = current_time;
        return;
    }

    // Refresh the existing backup route.
    if (sender_id == route.backup_hop) {
        route.backup_hop_count = hops;
        route.backup_link_quality_rssi = rssi;
        route.backup_updated_ms = current_time;
        return;
    }

    // A better candidate becomes the primary route.
    if (new_metric > primary_metric) {

        // Move the old primary route into the backup position.
        if (route.primary_hop != 0 &&
            route.primary_hop != local_node_id) {

            route.backup_hop = route.primary_hop;
            route.backup_hop_count = route.hop_count;
            route.backup_link_quality_rssi =
                route.link_quality_rssi;
            route.backup_updated_ms =
                route.last_updated_ms;
        }

        route.primary_hop = sender_id;
        route.hop_count = hops;
        route.link_quality_rssi = rssi;
        route.last_updated_ms = current_time;

        return;
    }

    // Otherwise, store this candidate as the backup route
    // if there is no backup yet or this candidate is better.
    if (sender_id != local_node_id &&
        (route.backup_hop == 0 || new_metric > backup_metric)) {

        route.backup_hop = sender_id;
        route.backup_hop_count = hops;
        route.backup_link_quality_rssi = rssi;
        route.backup_updated_ms = current_time;
    }
}

bool RoutingEngine::get_next_hop(
    uint16_t destination_id,
    uint16_t& next_hop_out) {

    std::lock_guard<std::mutex> lock(routing_mutex);

    auto it = routing_table.find(destination_id);

    if (it != routing_table.end() &&
        it->second.primary_hop != 0) {

        next_hop_out = it->second.primary_hop;
        return true;
    }

    return false;
}

bool RoutingEngine::handle_route_failure(
    uint16_t destination_id,
    uint16_t failed_hop,
    uint16_t& fallback_hop_out) {

    std::lock_guard<std::mutex> lock(routing_mutex);

    auto it = routing_table.find(destination_id);

    if (it == routing_table.end()) {
        return false;
    }

    RouteEntry& route = it->second;

    // Only switch routes when the failed hop
    // is the currently active primary route.
    if (route.primary_hop != failed_hop) {
        return false;
    }

    // No valid backup route is available.
    if (route.backup_hop == 0 ||
        route.backup_hop == local_node_id ||
        route.backup_hop == destination_id) {

        return false;
    }

    // Promote the backup route to primary.
    route.primary_hop = route.backup_hop;
    route.hop_count = route.backup_hop_count;
    route.link_quality_rssi =
        route.backup_link_quality_rssi;
    route.last_updated_ms =
        route.backup_updated_ms;

    // Clear the failed route from the backup position.
    route.backup_hop = 0;
    route.backup_hop_count = 0;
    route.backup_link_quality_rssi = 0;
    route.backup_updated_ms = 0;

    fallback_hop_out = route.primary_hop;

    // Required failover warning/metric.
    std::printf(
        "[MESH WARN] Primary route to Node %u failed! "
        "Switched to backup hop Node %u.\n",
        destination_id,
        fallback_hop_out
    );

    return true;
}

void RoutingEngine::prune_stale_routes(
    uint32_t current_time,
    uint32_t timeout_ms) {

    std::lock_guard<std::mutex> lock(routing_mutex);

    for (auto it = routing_table.begin();
         it != routing_table.end();) {

        uint32_t primary_elapsed =
            (current_time >= it->second.last_updated_ms)
                ? (current_time - it->second.last_updated_ms)
                : (0xFFFFFFFF -
                   it->second.last_updated_ms +
                   current_time);

        uint32_t backup_elapsed =
            (current_time >= it->second.backup_updated_ms)
                ? (current_time - it->second.backup_updated_ms)
                : (0xFFFFFFFF -
                   it->second.backup_updated_ms +
                   current_time);

        bool primary_stale =
            primary_elapsed > timeout_ms;

        bool backup_stale =
            it->second.backup_hop != 0 &&
            backup_elapsed > timeout_ms;

        // If the primary route is stale but the backup
        // route is still valid, promote the backup route.
        if (primary_stale) {

            if (!backup_stale &&
                it->second.backup_hop != 0) {

                it->second.primary_hop =
                    it->second.backup_hop;

                it->second.hop_count =
                    it->second.backup_hop_count;

                it->second.link_quality_rssi =
                    it->second.backup_link_quality_rssi;

                it->second.last_updated_ms =
                    it->second.backup_updated_ms;

                // Clear the backup after promotion.
                it->second.backup_hop = 0;
                it->second.backup_hop_count = 0;
                it->second.backup_link_quality_rssi = 0;
                it->second.backup_updated_ms = 0;

                ++it;

            } else {
                // Neither route is usable.
                it = routing_table.erase(it);
            }

        } else {

            // Remove only a stale backup route.
            if (backup_stale) {
                it->second.backup_hop = 0;
                it->second.backup_hop_count = 0;
                it->second.backup_link_quality_rssi = 0;
                it->second.backup_updated_ms = 0;
            }

            ++it;
        }
    }
}

void RoutingEngine::clear_table() {
    std::lock_guard<std::mutex> lock(routing_mutex);
    routing_table.clear();
}

const std::unordered_map<uint16_t, RouteEntry>&
RoutingEngine::get_table() const {
    return routing_table;
}
