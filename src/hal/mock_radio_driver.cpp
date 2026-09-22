#include "mock_radio_driver.h"

bool MockRadioDriver::init()
{
    rx_callback = nullptr;
    return true;
}

bool MockRadioDriver::send_packet(
    const uint8_t* target_mac,
    const uint8_t* data,
    size_t len
)
{
    (void)target_mac;
    (void)data;
    (void)len;

    return true;
}

void MockRadioDriver::on_receive(RxCallback callback)
{
    rx_callback = callback;
}

void MockRadioDriver::inject_packet(
    const uint8_t* src_mac,
    const uint8_t* data,
    size_t len,
    int8_t rssi
)
{
    if (rx_callback != nullptr)
    {
        rx_callback(src_mac, data, len, rssi);
    }
}
