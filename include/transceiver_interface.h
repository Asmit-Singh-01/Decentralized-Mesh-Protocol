#pragma once

#include <cstdint>
#include <cstddef>

enum class TransceiverType {
    RF_WIFI_ESP_NOW = 0x01,
    LORA_LONG_RANGE = 0x02,
    ACOUSTIC_UNDERWATER = 0x03
};

class ITransceiver {
public:
    virtual ~ITransceiver() = default;
    
    virtual bool initialize() = 0;
    virtual bool transmit_packet(const uint8_t* data, size_t len, uint16_t destination_id) = 0;
    virtual bool receive_packet(uint8_t* buffer, size_t max_len, size_t& out_len, int8_t& out_rssi) = 0;
    
    virtual TransceiverType get_type() const = 0;
};
