#pragma once
#include "radio_driver.h"
#include <vector>

class MockRadioDriver : public IRadioDriver {
private:
    bool initialized;
    void (*app_rx_callback)(const uint8_t* src_mac, const uint8_t* data, size_t len, int8_t rssi);
    std::vector<std::vector<uint8_t>> tx_history;

public:
    MockRadioDriver();
    ~MockRadioDriver() override = default;

    bool init() override;
    bool send_bytes(const uint8_t* target_mac, const uint8_t* data, size_t len) override;
    void set_rx_callback(void (*callback)(const uint8_t* src_mac, const uint8_t* data, size_t len, int8_t rssi)) override;

    // Simulation Test Helpers
    void inject_receive(const uint8_t* src_mac, const uint8_t* data, size_t len, int8_t rssi);
    const std::vector<std::vector<uint8_t>>& get_tx_history() const;
    size_t get_tx_count() const;
    void clear_history();
};
