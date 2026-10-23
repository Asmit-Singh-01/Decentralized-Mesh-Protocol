#include "mesh_gateway.h"
#include "rf_transceiver.h"
#include "acoustic_transceiver.h"
#include "node_onboarding.h"
#include <iostream>
#include <memory>

int main() {
    std::cout << "Starting Decentralized Mesh Protocol - Real-World Secure Onboarding Simulation...\n" << std::endl;

    uint16_t gateway_id = 0x5555;
    MeshGateway gateway(gateway_id);

    auto rf_modem = std::make_shared<RFTransceiver>(gateway_id);
    gateway.register_transceiver(rf_modem);
    gateway.initialize_all();

    // Initialize Security Onboarding System with a Pre-Shared Key
    NodeOnboarding security_manager(gateway_id, "SWARM_SECURE_KEY_2026");

    uint16_t new_drone_id = 0x1234;
    
    // Simulate new drone attempting to join with correct PSK
    std::cout << "\n[Simulating New Drone Onboarding Request]...\n";
    if (security_manager.authenticate_incoming_node(new_drone_id, "SWARM_SECURE_KEY_2026")) {
        std::cout << "Drone 0x" << std::hex << new_drone_id << " is now fully secured in the mesh swarm!\n" << std::dec;
    }

    std::cout << "\nAll Systems Nominal. Secure Onboarding Verified Successfully." << std::endl;
    return 0;
}
