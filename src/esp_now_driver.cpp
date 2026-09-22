#include "esp_now_driver.h"

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <cstring>

void (*EspNowDriver::app_rx_callback)(
    const uint8_t* src_mac,
    const uint8_t* data,
    size_t len,
    int8_t rssi
) = nullptr;

// Compatibility wrapper for ESP-IDF v4 vs v5 callback signatures
#if defined(ESP_IDF_VERSION) && ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)

static void esp_now_on_data_recv(
    const esp_now_recv_info_t* recv_info,
    const uint8_t* incomingData,
    int len
)
{
    int8_t rssi = recv_info->rx_ctrl
        ? recv_info->rx_ctrl->rssi
        : 0;

    EspNowDriver::handle_raw_receive(
        recv_info->src_addr,
        incomingData,
        len,
        rssi
    );
}

#else

static void esp_now_on_data_recv(
    const uint8_t* mac_addr,
    const uint8_t* incomingData,
    int len
)
{
    EspNowDriver::handle_raw_receive(
        mac_addr,
        incomingData,
        len,
        0
    );
}

#endif

bool EspNowDriver::init()
{
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_now_init() != ESP_OK)
    {
        return false;
    }

    esp_now_register_recv_cb(esp_now_on_data_recv);

    return true;
}

void EspNowDriver::suspend()
{
    esp_now_deinit();
}

void EspNowDriver::resume()
{
    init();
}

bool EspNowDriver::send_bytes(
    const uint8_t* target_mac,
    const uint8_t* data,
    size_t len
)
{
    if (!data || len == 0 || len > 250)
    {
        return false;
    }

    esp_now_peer_info_t peer_info = {};

    if (target_mac)
    {
        std::memcpy(
            peer_info.peer_addr,
            target_mac,
            6
        );
    }
    else
    {
        std::memset(
            peer_info.peer_addr,
            0xFF,
            6
        );
    }

    peer_info.channel = 0;
    peer_info.encrypt = false;
    peer_info.ifidx = WIFI_IF_STA;

    if (!esp_now_is_peer_exist(peer_info.peer_addr))
    {
        if (esp_now_add_peer(&peer_info) != ESP_OK)
        {
            return false;
        }
    }

    esp_err_t result = esp_now_send(
        peer_info.peer_addr,
        data,
        len
    );

    return (result == ESP_OK);
}

void EspNowDriver::set_rx_callback(
    void (*callback)(
        const uint8_t* src_mac,
        const uint8_t* data,
        size_t len,
        int8_t rssi
    )
)
{
    app_rx_callback = callback;
}

void EspNowDriver::handle_raw_receive(
    const uint8_t* mac_addr,
    const uint8_t* incoming_data,
    int len,
    int8_t rssi
)
{
    if (
        app_rx_callback &&
        mac_addr &&
        incoming_data &&
        len > 0
    )
    {
        app_rx_callback(
            mac_addr,
            incoming_data,
            static_cast<size_t>(len),
            rssi
        );
    }
}

#else

// Desktop host mock implementation with direct loopback execution

void (*EspNowDriver::app_rx_callback)(
    const uint8_t* src_mac,
    const uint8_t* data,
    size_t len,
    int8_t rssi
) = nullptr;

bool EspNowDriver::init()
{
    return true;
}

void EspNowDriver::suspend()
{
    // No radio hardware to suspend on desktop builds.
}

void EspNowDriver::resume()
{
    // No radio hardware to resume on desktop builds.
}

bool EspNowDriver::send_bytes(
    const uint8_t* target_mac,
    const uint8_t* data,
    size_t len
)
{
    if (!data || len == 0)
    {
        return false;
    }

    uint8_t dummy_mac[6] = {
        0xAA,
        0xBB,
        0xCC,
        0xDD,
        0xEE,
        0xFF
    };

    const uint8_t* src =
        target_mac ? target_mac : dummy_mac;

    handle_raw_receive(
        src,
        data,
        static_cast<int>(len),
        -45
    );

    return true;
}

void EspNowDriver::set_rx_callback(
    void (*callback)(
        const uint8_t*,
        const uint8_t*,
        size_t,
        int8_t
    )
)
{
    app_rx_callback = callback;
}

void EspNowDriver::handle_raw_receive(
    const uint8_t* mac_addr,
    const uint8_t* incoming_data,
    int len,
    int8_t rssi
)
{
    if (
        app_rx_callback &&
        mac_addr &&
        incoming_data &&
        len > 0
    )
    {
        app_rx_callback(
            mac_addr,
            incoming_data,
            static_cast<size_t>(len),
            rssi
        );
    }
}

#endif
