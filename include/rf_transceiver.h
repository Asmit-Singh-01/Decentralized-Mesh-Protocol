#pragma once

#include "transceiver_interface.h"
#include <iostream>

class RFTransceiver : public ITransceiver {
private:
    uint16_t node_id;
    bool active;

public:
    explicit RFTransceiver(uint16_t id) : node_id(id), active(false) {}

    bool initialize() override {
        std::cout << "[RFTransceiver] Initializing 2.4GHz RF Module for Node: 0x" << std::hex << node_id << std::endl;
        active = true;
        return true;
    }

    bool transmit_packet(const uint8_t* data, size_t len, uint16_t destination_id) override {
        if (!active || !data) return false;
        std::cout << "[RFTransceiver] Node 0x" << std::hex << node_id 
                  << " broadcasting " << std::dec << len << " bytes via RF to Target 0x" << std::hex << destination_id << std::endl;
        return true;
    }

    bool receive_packet(uint8_t* buffer, size_t max_len, size_t& out_len, int8_t& out_rssi) override {
        (void)buffer;
        (void)max_len;
        (void)out_len;
        (void)out_rssi;
        // Simulated polling/reception hook
        return false;
    }

    TransceiverType get_type() const override {
        return TransceiverType::RF_WIFI_ESP_NOW;
    }
};
