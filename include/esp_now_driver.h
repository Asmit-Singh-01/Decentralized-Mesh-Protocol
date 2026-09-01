#pragma once
#include "radio_driver.h"

class EspNowDriver : public IRadioDriver {
private:


public:

    static void (*app_rx_callback)(const uint8_t* src_mac, const uint8_t* data, size_t len, int8_t rssi);
    EspNowDriver() = default;
    ~EspNowDriver() override = default;
    

    bool init() override;
    bool send_bytes(const uint8_t* target_mac, const uint8_t* data, size_t len) override;
    void set_rx_callback(void (*callback)(const uint8_t* src_mac, const uint8_t* data, size_t len, int8_t rssi)) override;
    
    static void handle_raw_receive(const uint8_t* mac_addr, const uint8_t* incoming_data, int len, int8_t rssi);
};
