#pragma once

#include <cstdint>
#include <cstddef>
#include <unordered_map>
#include <vector>

struct SwarmTask {
    uint32_t task_id;
    uint8_t task_type; // e.g., 0x01 = Frontier Exploration, 0x02 = Map Merging
    uint16_t target_x;
    uint16_t target_y;
    bool assigned;
    uint16_t assigned_node_id;
};

class SwarmOrchestrator {
private:
    uint16_t local_node_id;
    std::unordered_map<uint32_t, SwarmTask> active_tasks;
    std::vector<uint32_t> local_task_queue;

public:
    explicit SwarmOrchestrator(uint16_t node_id);

    bool init();
    void create_task(uint32_t task_id, uint8_t type, uint16_t x, uint16_t y);
    
    // Market-based auction bidding calculation
    uint32_t calculate_bid_score(const SwarmTask& task, int8_t rssi, uint8_t hop_count) const;
    
    bool evaluate_and_bid(uint32_t task_id, int8_t rssi, uint8_t hop_count);
    void execute_orchestration_cycle(uint32_t current_time_ms);

    const std::unordered_map<uint32_t, SwarmTask>& get_active_tasks() const;
};
