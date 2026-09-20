#include "swarm_orchestrator.h"
#include <iostream>
#include <cstring>
#include <algorithm>

SwarmOrchestrator::SwarmOrchestrator(uint16_t node_id) 
    : local_node_id(node_id), routing_engine(node_id) {}

void SwarmOrchestrator::init() {
    task_queue.clear();
}

void SwarmOrchestrator::assign_task(uint16_t task_id, uint8_t priority) {
    // Check for duplicate task before adding
    for (const auto& task : task_queue) {
        if (task.task_id == task_id) {
            return;
        }
    }
    
    SwarmTask task{task_id, priority, local_node_id, false};
    task_queue.push_back(task);
    
    // Sort tasks based on priority (Highest priority first)
    std::sort(task_queue.begin(), task_queue.end(), [](const SwarmTask& a, const SwarmTask& b) {
        return a.priority > b.priority;
    });
}

void SwarmOrchestrator::process_incoming_task(const uint8_t* payload, uint8_t len) {
    if (!payload || len < (sizeof(uint16_t) + sizeof(uint8_t))) return;

    uint16_t t_id;
    uint8_t prio;
    std::memcpy(&t_id, payload, sizeof(uint16_t));
    std::memcpy(&prio, payload + sizeof(uint16_t), sizeof(uint8_t));

    assign_task(t_id, prio);
}

void SwarmOrchestrator::execute_orchestration_cycle(uint32_t current_time_ms) {
    routing_engine.prune_stale_routes(current_time_ms);
    
    for (auto& task : task_queue) {
        if (!task.completed) {
            // Simulate execution logic
            task.completed = true;
        }
    }

    // Garbage collect completed tasks to optimize memory footprint
    task_queue.erase(
        std::remove_if(task_queue.begin(), task_queue.end(), [](const SwarmTask& t) {
            return t.completed;
        }),
        task_queue.end()
    );
}

void SwarmOrchestrator::clear_tasks() {
    task_queue.clear();
}

const std::vector<SwarmTask>& SwarmOrchestrator::get_tasks() const {
    return task_queue;
}

