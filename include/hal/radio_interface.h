#pragma once

#include <cstddef>
#include <cstdint>

class IRadioDriver {
public:
    using RxCallback = void (*)(
        const uint8_t* src_mac,
        const uint8_t* data,
        size_t len,
        int8_t rssi
    );

    virtual ~IRadioDriver() = default;

    virtual bool init() = 0;

    virtual bool send_packet(
        const uint8_t* target_mac,
        const uint8_t* data,
        size_t len
    ) = 0;

    virtual void on_receive(RxCallback callback) = 0;
};
