#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_set>

class NodeOnboarding {
private:
    uint16_t local_node_id;
    std::string network_psk; // Pre-Shared Key for mesh authentication
    std::unordered_set<uint16_t> authenticated_nodes;

public:
    NodeOnboarding(uint16_t id, const std::string& psk) 
        : local_node_id(id), network_psk(psk) {}

    // Request to join the network
    bool request_join(uint16_t target_gateway_id, const std::string& provided_psk) {
        if (provided_psk == network_psk) {
            std::cout << "[Onboarding] Node 0x" << std::hex << local_node_id 
                      << " successfully authenticated with Gateway 0x" << target_gateway_id << std::dec << std::endl;
            return true;
        }
        std::cerr << "[Onboarding Error] Authentication failed for Node 0x" << std::hex << local_node_id << std::dec << std::endl;
        return false;
    }

    // Gateway side verification of incoming node
    bool authenticate_incoming_node(uint16_t incoming_node_id, const std::string& provided_psk) {
        if (provided_psk == network_psk) {
            authenticated_nodes.insert(incoming_node_id);
            std::cout << "[Gateway Onboarding] Node 0x" << std::hex << incoming_node_id 
                      << " authorized and onboarded into mesh network." << std::dec << std::endl;
            return true;
        }
        std::cerr << "[Gateway Onboarding Security Alert] Unauthorized join attempt from Node 0x" 
                  << std::hex << incoming_node_id << std::dec << std::endl;
        return false;
    }

    bool is_node_authenticated(uint16_t node_id) const {
        return authenticated_nodes.find(node_id) != authenticated_nodes.end();
    }
};
