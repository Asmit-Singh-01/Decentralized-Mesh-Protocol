#pragma once

#include "transceiver_interface.h"
#include "packet_format.h"
#include <vector>
#include <memory>

class MeshGateway {
private:
    uint16_t gateway_node_id;
    std::vector<std::shared_ptr<ITransceiver>> transceivers;

public:
    explicit MeshGateway(uint16_t node_id);

    void register_transceiver(std::shared_ptr<ITransceiver> transceiver);
    bool initialize_all();
    
    // Cross-media bridge routing: Route packet from one medium to another
    bool bridge_packet(const uint8_t* raw_packet, size_t len, TransceiverType source_type, TransceiverType target_type, uint16_t target_node_id);
    
    size_t get_active_transceiver_count() const;
};
