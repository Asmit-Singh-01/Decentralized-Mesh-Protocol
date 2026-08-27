#pragma once
#include "packet_format.h"
#include "routing_engine.h"
#include <stdint.h>
#include <vector>

struct SwarmTask {
    uint16_t task_id;
    uint8_t priority;
    uint16_t assigned_node;
    bool completed;
};

class SwarmOrchestrator {
private:
    uint16_t local_node_id;
    RoutingEngine routing_engine;
    std::vector<SwarmTask> task_queue;

public:
    explicit SwarmOrchestrator(uint16_t node_id);

    void init();
    void assign_task(uint16_t task_id, uint8_t priority);
    void process_incoming_task(const uint8_t* payload, uint8_t len);
    void execute_orchestration_cycle(uint32_t current_time_ms);
    
    const std::vector<SwarmTask>& get_tasks() const;
};
