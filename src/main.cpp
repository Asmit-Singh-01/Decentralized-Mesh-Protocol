#include <iostream>
#include "swarm_orchestrator.h"

int main() {
    std::cout << ">>> DECENTRALIZED SWARM OS / MESH CORE INITIALIZED <<<" << std::endl;

    SwarmOrchestrator node(0x1001);
    node.init();

    std::cout << "[SYSTEM] Assigning autonomous swarm mission tasks..." << std::endl;
    node.assign_task(101, 1);
    node.assign_task(102, 2);

    std::cout << "[SYSTEM] Running initial orchestration cycle..." << std::endl;
    node.execute_orchestration_cycle(1000);

    std::cout << "[SUCCESS] Active task queue size: " << node.get_tasks().size() << std::endl;
    std::cout << ">>> SYSTEM CORE FULLY OPERATIONAL <<<" << std::endl;

    return 0;
}

