#include "swarm_orchestrator.h"
#include <iostream>
#include <algorithm>
#include <cmath>

SwarmOrchestrator::SwarmOrchestrator(uint16_t node_id) : local_node_id(node_id) {}

bool SwarmOrchestrator::init() {
    std::cout << "Initializing SwarmOrchestrator for Node: 0x" << std::hex << local_node_id << std::endl;
    active_tasks.clear();
    local_task_queue.clear();
    return true;
}

void SwarmOrchestrator::create_task(uint32_t task_id, uint8_t type, uint16_t x, uint16_t y) {
    SwarmTask task{task_id, type, x, y, false, 0};
    active_tasks[task_id] = task;
    std::cout << "New Swarm Mission Task Created -> ID: " << task_id 
              << " [Type: " << static_cast<int>(type) << " at X:" << x << ", Y:" << y << "]" << std::endl;
}

uint32_t SwarmOrchestrator::calculate_bid_score(const SwarmTask& task, int8_t rssi, uint8_t hop_count) const {
    (void)task;
    // Lower score wins the auction (Cost = Hops * 50 + Signal Loss Factor)
    int attr_cost = hop_count * 50 + std::abs(rssi);
    return static_cast<uint32_t>(attr_cost);
}

bool SwarmOrchestrator::evaluate_and_bid(uint32_t task_id, int8_t rssi, uint8_t hop_count) {
    auto it = active_tasks.find(task_id);
    if (it == active_tasks.end() || it->second.assigned) {
        return false; // Task does not exist or already claimed
    }

    uint32_t my_bid = calculate_bid_score(it->second, rssi, hop_count);
    
    // Assign task locally if bid is evaluated successfully
    it->second.assigned = true;
    it->second.assigned_node_id = local_node_id;
    local_task_queue.push_back(task_id);

    std::cout << "Node 0x" << std::hex << local_node_id 
              << " won auction for Task ID: " << std::dec << task_id 
              << " with Bid Cost: " << my_bid << std::endl;
    return true;
}

void SwarmOrchestrator::execute_orchestration_cycle(uint32_t current_time_ms) {
    (void)current_time_ms;
    // Periodic check for stale unassigned tasks or telemetry heartbeats
}

const std::unordered_map<uint32_t, SwarmTask>& SwarmOrchestrator::get_active_tasks() const {
    return active_tasks;
}
