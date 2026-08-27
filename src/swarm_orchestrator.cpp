#include "swarm_orchestrator.h"
#include <iostream>
#include <cstring>

SwarmOrchestrator::SwarmOrchestrator(uint16_t node_id) 
    : local_node_id(node_id), routing_engine(node_id) {}

void SwarmOrchestrator::init() {
    task_queue.clear();
}

void SwarmOrchestrator::assign_task(uint16_t task_id, uint8_t priority) {
    SwarmTask task{task_id, priority, local_node_id, false};
    task_queue.push_back(task);
}

void SwarmOrchestrator::process_incoming_task(const uint8_t* payload, uint8_t len) {
    if (len < sizeof(uint16_t) + sizeof(uint8_t)) return;

    uint16_t t_id;
    uint8_t prio;
    std::memcpy(&t_id, payload, sizeof(uint16_t));
    std::memcpy(&prio, payload + sizeof(uint16_t), sizeof(uint8_t));

    task_queue.push_back({t_id, prio, 0xFFFF, false});
}

void SwarmOrchestrator::execute_orchestration_cycle(uint32_t current_time_ms) {
    routing_engine.prune_stale_routes(current_time_ms);
    for (auto& task : task_queue) {
        if (!task.completed) {
            task.completed = true;
        }
    }
}

const std::vector<SwarmTask>& SwarmOrchestrator::get_tasks() const {
    return task_queue;
}
