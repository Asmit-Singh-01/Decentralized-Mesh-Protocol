#include "esp_now_driver.h"

#ifdef ARDUINO

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <cstring>

EspNowDriver::RxCallback EspNowDriver::app_rx_callback = nullptr;

bool EspNowDriver::init()
{
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_now_init() != ESP_OK)
    {
        return false;
    }

#if ESP_IDF_VERSION_MAJOR >= 5
    esp_now_register_recv_cb(
        [](const esp_now_recv_info_t* info,
           const uint8_t* data,
           int len)
        {
            int8_t rssi = 0;

            if (info != nullptr)
            {
                rssi = info->rx_ctrl->rssi;
                EspNowDriver::handle_raw_receive(
                    info->src_addr,
                    data,
                    len,
                    rssi
                );
            }
        }
    );
#else
    esp_now_register_recv_cb(
        [](const uint8_t* mac_addr,
           const uint8_t* data,
           int len)
        {
            EspNowDriver::handle_raw_receive(
                mac_addr,
                data,
                len,
                0
            );
        }
    );
#endif

    return true;
}

bool EspNowDriver::send_packet(
    const uint8_t* target_mac,
    const uint8_t* data,
    size_t len
)
{
    if (target_mac == nullptr || data == nullptr || len > 250)
    {
        return false;
    }

    esp_now_peer_info_t peer_info{};
    std::memcpy(peer_info.peer_addr, target_mac, 6);
    peer_info.channel = 0;
    peer_info.encrypt = false;

    if (!esp_now_is_peer_exist(target_mac))
    {
        if (esp_now_add_peer(&peer_info) != ESP_OK)
        {
            return false;
        }
    }

    return esp_now_send(
        target_mac,
        data,
        len
    ) == ESP_OK;
}

void EspNowDriver::on_receive(RxCallback callback)
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
    if (app_rx_callback != nullptr)
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

EspNowDriver::RxCallback EspNowDriver::app_rx_callback = nullptr;

bool EspNowDriver::init()
{
    return true;
}

bool EspNowDriver::send_packet(
    const uint8_t* target_mac,
    const uint8_t* data,
    size_t len
)
{
    if (app_rx_callback != nullptr)
    {
        app_rx_callback(
            target_mac,
            data,
            len,
            -45
        );
    }

    return true;
}

void EspNowDriver::on_receive(RxCallback callback)
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
    if (app_rx_callback != nullptr)
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
