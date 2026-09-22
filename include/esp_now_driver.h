#pragma once

#include "radio_driver.h"
#include <cstdint>
#include <cstddef>

class EspNowDriver : public IRadioDriver {
private:
    static void (*app_rx_callback)(
        const uint8_t* src_mac,
        const uint8_t* data,
        size_t len,
        int8_t rssi
    );

public:
    EspNowDriver() = default;
    ~EspNowDriver() override = default;

    // Prevent copy construction and assignment to maintain driver instance integrity
    EspNowDriver(const EspNowDriver&) = delete;
    EspNowDriver& operator=(const EspNowDriver&) = delete;

    bool init() override;

    void suspend();
    void resume();

    bool send_bytes(
        const uint8_t* target_mac,
        const uint8_t* data,
        size_t len
    ) override;

    void set_rx_callback(
        void (*callback)(
            const uint8_t* src_mac,
            const uint8_t* data,
            size_t len,
            int8_t rssi
        )
    ) override;

    static void handle_raw_receive(
        const uint8_t* mac_addr,
        const uint8_t* incoming_data,
        int len,
        int8_t rssi
    );
};
