#include "battery_monitor.h"

#include <cstdint>

#ifdef ARDUINO

#include <Arduino.h>

namespace BatteryMonitor
{
    constexpr uint8_t BATTERY_ADC_PIN = 34;

    constexpr float MIN_BATTERY_VOLTAGE = 3.2f;
    constexpr float MAX_BATTERY_VOLTAGE = 4.2f;
    constexpr float LOW_BATTERY_THRESHOLD = 3.3f;

    constexpr uint8_t ADC_SAMPLE_COUNT = 8;
    constexpr uint16_t ADC_MAX_VALUE = 4095;

  

    static bool low_power_flag = false;
    static bool warning_sent = false;

    float read_battery_voltage()
    {
        uint32_t total = 0;

        for (uint8_t i = 0; i < ADC_SAMPLE_COUNT; ++i)
        {
            total += analogRead(BATTERY_ADC_PIN);
        }

        const float average_raw =
            static_cast<float>(total) /
            static_cast<float>(ADC_SAMPLE_COUNT);

        const float voltage =
            MIN_BATTERY_VOLTAGE +
            (average_raw / ADC_MAX_VALUE) *
            (MAX_BATTERY_VOLTAGE - MIN_BATTERY_VOLTAGE);

        return voltage;
    }

    bool check_battery(MeshNode& mesh_node)
    {
        const float voltage = read_battery_voltage();

        if (voltage < LOW_BATTERY_THRESHOLD)
        {
            low_power_flag = true;

            const char warning_message[] =
                "STATUS_BATTERY_LOW";

            mesh_node.broadcast_payload(
                PacketType::STATUS_BATTERY_LOW,
                reinterpret_cast<const uint8_t*>(
                    warning_message
                ),
                sizeof(warning_message) - 1
            );

            if (!warning_sent)
            {
                Serial.print(
                    "[POWER WARN] Low battery detected: "
                );
                Serial.print(voltage, 2);
                Serial.println(
                    "V! Sending warning packet."
                );

                warning_sent = true;
            }

            return true;
        }

        low_power_flag = false;
        warning_sent = false;

        return false;
    }

    bool is_low_power()
    {
        return low_power_flag;
    }
}

#else

namespace BatteryMonitor
{
    float read_battery_voltage()
    {
        return 4.2f;
    }

    bool check_battery(MeshNode&)
    {
        return false;
    }

    bool is_low_power()
    {
        return false;
    }
}

#endif
