#include "routing_engine.h"
#include <cstdint>
#include <algorithm>

RoutingEngine::RoutingEngine(uint16_t node_id)
    : local_node_id(node_id) {}

void RoutingEngine::process_beacon(
    uint16_t sender_id,
    uint16_t dest_id,
    uint8_t hops,
    int8_t rssi,
    uint32_t current_time
) {
    // Ignore our own route and routes that point to ourselves.
    if (sender_id == local_node_id || dest_id == local_node_id) {
        return;
    }

    // A neighbor must not be selected as a route to itself through us.
    if (sender_id == dest_id) {
        return;
    }

    const int32_t new_metric =
        static_cast<int32_t>(rssi) -
        (static_cast<int32_t>(hops) * 10);

    auto it = routing_table.find(dest_id);

    // First route learned for this destination.
    if (it == routing_table.end()) {
        RouteEntry entry{};
        entry.destination_id = dest_id;
        entry.next_hop_id = sender_id;
        entry.hop_count = hops;
        entry.link_quality_rssi = rssi;
        entry.last_updated_ms = current_time;

        routing_table.emplace(dest_id, entry);
        return;
    }

    RouteEntry& route = it->second;

    const int32_t primary_metric =
        static_cast<int32_t>(route.link_quality_rssi) -
        (static_cast<int32_t>(route.hop_count) * 10);

    // Refresh the current primary route.
    if (sender_id == route.next_hop_id) {
        route.hop_count = hops;
        route.link_quality_rssi = rssi;
        route.last_updated_ms = current_time;
        return;
    }

    // Refresh the current backup route.
    if (route.has_backup_hop &&
        sender_id == route.backup_hop_id) {

        route.backup_hop_count = hops;
        route.backup_link_quality_rssi = rssi;
        route.backup_last_updated_ms = current_time;

        const int32_t backup_metric =
            static_cast<int32_t>(route.backup_link_quality_rssi) -
            (static_cast<int32_t>(route.backup_hop_count) * 10);

        // Promote the backup if it now has the better metric.
        if (backup_metric > primary_metric) {
            std::swap(route.next_hop_id, route.backup_hop_id);
            std::swap(route.hop_count, route.backup_hop_count);
            std::swap(route.link_quality_rssi,
                      route.backup_link_quality_rssi);
            std::swap(route.last_updated_ms,
                      route.backup_last_updated_ms);
        }
        return;
    }

    // A newly discovered route is better than the primary.
    if (new_metric > primary_metric) {
        route.backup_hop_id = route.next_hop_id;
        route.backup_hop_count = route.hop_count;
        route.backup_link_quality_rssi = route.link_quality_rssi;
        route.backup_last_updated_ms = route.last_updated_ms;
        route.has_backup_hop = true;

        route.next_hop_id = sender_id;
        route.hop_count = hops;
        route.link_quality_rssi = rssi;
        route.last_updated_ms = current_time;
        return;
    }

    // Otherwise, keep it as backup if it is better than the
    // currently stored backup route.
    if (!route.has_backup_hop) {
        route.backup_hop_id = sender_id;
        route.backup_hop_count = hops;
        route.backup_link_quality_rssi = rssi;
        route.backup_last_updated_ms = current_time;
        route.has_backup_hop = true;
        return;
    }

    const int32_t backup_metric =
        static_cast<int32_t>(route.backup_link_quality_rssi) -
        (static_cast<int32_t>(route.backup_hop_count) * 10);

    if (new_metric > backup_metric) {
        route.backup_hop_id = sender_id;
        route.backup_hop_count = hops;
        route.backup_link_quality_rssi = rssi;
        route.backup_last_updated_ms = current_time;
        route.has_backup_hop = true;
    }
}

bool RoutingEngine::get_next_hop(
    uint16_t destination_id,
    uint16_t& next_hop_out
) {
    auto it = routing_table.find(destination_id);

    if (it == routing_table.end()) {
        return false;
    }

    next_hop_out = it->second.next_hop_id;
    return true;
}

void RoutingEngine::prune_stale_routes(
    uint32_t current_time,
    uint32_t timeout_ms
) {
    for (auto it = routing_table.begin();
         it != routing_table.end();) {

        RouteEntry& route = it->second;

        const uint32_t primary_elapsed =
            current_time - route.last_updated_ms;

        if (route.has_backup_hop) {
            const uint32_t backup_elapsed =
                current_time - route.backup_last_updated_ms;

            if (backup_elapsed > timeout_ms) {
                route.has_backup_hop = false;
                route.backup_hop_id = 0;
            }
        }

        // If primary is stale but backup is still fresh,
        // promote the backup route.
        if (primary_elapsed > timeout_ms) {
            if (route.has_backup_hop) {
                route.next_hop_id = route.backup_hop_id;
                route.hop_count = route.backup_hop_count;
                route.link_quality_rssi =
                    route.backup_link_quality_rssi;
                route.last_updated_ms =
                    route.backup_last_updated_ms;

                route.has_backup_hop = false;
                route.backup_hop_id = 0;
                ++it;
            } else {
                it = routing_table.erase(it);
            }
        } else {
            ++it;
        }
    }
}

void RoutingEngine::clear_table() {
    routing_table.clear();
}

const std::unordered_map<uint16_t, RouteEntry>&
RoutingEngine::get_table() const {
    return routing_table;
}
bool RoutingEngine::switch_to_backup(
    uint16_t destination_id,
    uint16_t failed_hop_id
) {
    auto it = routing_table.find(destination_id);

    if (it == routing_table.end()) {
        return false;
    }

    RouteEntry& route = it->second;

    // Only switch if the failed hop is the current primary.
    if (route.next_hop_id != failed_hop_id) {
        return false;
    }

    // Backup must exist and must not point to this node.
    if (!route.has_backup_hop ||
        route.backup_hop_id == route.next_hop_id ||
        route.backup_hop_id == local_node_id) {
        return false;
    }

    route.next_hop_id = route.backup_hop_id;
    route.hop_count = route.backup_hop_count;
    route.link_quality_rssi = route.backup_link_quality_rssi;
    route.last_updated_ms = route.backup_last_updated_ms;

    route.has_backup_hop = false;
    route.backup_hop_id = 0;

    return true;
}
