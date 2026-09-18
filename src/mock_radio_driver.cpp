#include "mock_radio_driver.h"
#include <iostream>
#include <cstring>

MockRadioDriver::MockRadioDriver() : initialized(false), app_rx_callback(nullptr) {}

bool MockRadioDriver::init() {
    initialized = true;
    tx_history.clear();
    std::cout << "[HAL MOCK] MockRadioDriver initialized for native host simulation." << std::endl;
    return true;
}

bool MockRadioDriver::send_bytes(const uint8_t* target_mac, const uint8_t* data, size_t len) {
    (void)target_mac;
    if (!initialized || !data || len == 0) return false;

    std::vector<uint8_t> buffer(data, data + len);
    tx_history.push_back(buffer);
    std::cout << "[HAL MOCK] Transmitted " << len << " bytes via simulated radio interface." << std::endl;
    return true;
}

void MockRadioDriver::set_rx_callback(void (*callback)(const uint8_t* src_mac, const uint8_t* data, size_t len, int8_t rssi)) {
    app_rx_callback = callback;
}

void MockRadioDriver::inject_receive(const uint8_t* src_mac, const uint8_t* data, size_t len, int8_t rssi) {
    if (!initialized || !data || len == 0) return;

    std::cout << "[HAL MOCK] Injected " << len << " incoming bytes (RSSI: " << static_cast<int>(rssi) << " dBm)." << std::endl;
    if (app_rx_callback) {
        app_rx_callback(src_mac, data, len, rssi);
    }
}

const std::vector<std::vector<uint8_t>>& MockRadioDriver::get_tx_history() const {
    return tx_history;
}

size_t MockRadioDriver::get_tx_count() const {
    return tx_history.size();
}

void MockRadioDriver::clear_history() {
    tx_history.clear();
}
