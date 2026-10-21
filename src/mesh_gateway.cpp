#include "mesh_gateway.h"
#include <iostream>

MeshGateway::MeshGateway(uint16_t node_id) : gateway_node_id(node_id) {}

void MeshGateway::register_transceiver(std::shared_ptr<ITransceiver> transceiver) {
    if (transceiver) {
        transceivers.push_back(transceiver);
    }
}

bool MeshGateway::initialize_all() {
    bool success = true;
    for (auto& t : transceivers) {
        if (!t->initialize()) {
            success = false;
        }
    }
    std::cout << "[MeshGateway] Node 0x" << std::hex << gateway_node_id 
              << " initialized " << std::dec << transceivers.size() << " transceivers successfully." << std::endl;
    return success;
}

bool MeshGateway::bridge_packet(const uint8_t* raw_packet, size_t len, TransceiverType source_type, TransceiverType target_type, uint16_t target_node_id) {
    std::shared_ptr<ITransceiver> target_transceiver = nullptr;
    
    // Find the target medium transceiver (e.g., bridging RF -> Acoustic)
    for (auto& t : transceivers) {
        if (t->get_type() == target_type) {
            target_transceiver = t;
            break;
        }
    }

    if (!target_transceiver) {
        std::cerr << "[MeshGateway Error] Target transceiver medium not found on gateway node!" << std::endl;
        return false;
    }

    std::cout << "[MeshGateway] Bridging packet from Medium [" << static_cast<int>(source_type) 
              << "] to Medium [" << static_cast<int>(target_type) 
              << "] for target node 0x" << std::hex << target_node_id << std::endl;

    return target_transceiver->transmit_packet(raw_packet, len, target_node_id);
}

size_t MeshGateway::get_active_transceiver_count() const {
    return transceivers.size();
}
