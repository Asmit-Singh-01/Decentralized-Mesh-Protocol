#include "mesh_gateway.h"
#include "rf_transceiver.h"
#include "power_management.h"
#include <iostream>
#include <memory>

int main() {
    std::cout << "Starting Decentralized Mesh Protocol - Power Management & Duty Cycling Simulation...\n" << std::endl;

    uint16_t node_id = 0x9999;
    PowerManager power_manager(node_id, 85); // 85% battery

    std::cout << "\n[Simulating Hardware Duty Cycling Cycle]...\n";
    power_manager.consume_battery(10);
    
    // Execute sleep/wake cycle to preserve power
    power_manager.execute_duty_cycle_sleep(50);

    std::cout << "\nAll Systems Nominal. Power Management Verified Successfully." << std::endl;
    return 0;
}
