#pragma once

#include "radio_interface.h"

class EspNowDriver : public IRadioDriver {
public:
    EspNowDriver() = default;
    ~EspNowDriver() override = default;

    EspNowDriver(const EspNowDriver&) = delete;
    EspNowDriver& operator=(const EspNowDriver&) = delete;

    bool init() override;

    bool send_packet(
        const uint8_t* target_mac,
        const uint8_t* data,
        size_t len
    ) override;

    void on_receive(RxCallback callback) override;

private:
    static RxCallback app_rx_callback;

    static void handle_raw_receive(
        const uint8_t* mac_addr,
        const uint8_t* incoming_data,
        int len,
        int8_t rssi
    );
};
