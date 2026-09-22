#pragma once

#include "radio_interface.h"

class MockRadioDriver : public IRadioDriver {
public:
    MockRadioDriver() = default;
    ~MockRadioDriver() override = default;

    bool init() override;

    bool send_packet(
        const uint8_t* target_mac,
        const uint8_t* data,
        size_t len
    ) override;

    void on_receive(RxCallback callback) override;

    // Used by desktop tests to simulate an incoming packet.
    void inject_packet(
        const uint8_t* src_mac,
        const uint8_t* data,
        size_t len,
        int8_t rssi = -45
    );

private:
    RxCallback rx_callback = nullptr;
};
