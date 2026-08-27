#pragma once
#include <stdint.h>
#include <stddef.h>

class IRadioDriver {
public:
    virtual ~IRadioDriver() = default;
    
    virtual bool init() = 0;
    virtual bool send_bytes(const uint8_t* target_mac, const uint8_t* data, size_t len) = 0;
    virtual void set_rx_callback(void (*callback)(const uint8_t* src_mac, const uint8_t* data, size_t len, int8_t rssi)) = 0;
};
