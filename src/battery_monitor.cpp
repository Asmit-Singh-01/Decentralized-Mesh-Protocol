#include "battery_monitor.h"
#include <iostream>
#include <numeric>
#include <cstring>

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
#include <Arduino.h>
#endif

BatteryMonitor::BatteryMonitor(uint8_t pin, size_t smoothing_samples)
    : adc_pin(pin),
      current_voltage(4.2f),
      low_power_flag(false),
      alert_broadcasted(false),
      window_size(smoothing_samples),
      simulation_mode(false),
      simulated_voltage(4.2f) {
    sample_window.reserve(window_size);
}

uint16_t BatteryMonitor::read_raw_adc() {
    if (simulation_mode) {
        // Map simulated voltage back to raw ADC range (3.2V - 4.2V -> 0 - 4095)
        float clamped = simulated_voltage;
        if (clamped < BATTERY_VOLTAGE_MIN) clamped = BATTERY_VOLTAGE_MIN;
        if (clamped > BATTERY_VOLTAGE_MAX) clamped = BATTERY_VOLTAGE_MAX;
        float fraction = (clamped - BATTERY_VOLTAGE_MIN) / (BATTERY_VOLTAGE_MAX - BATTERY_VOLTAGE_MIN);
        return static_cast<uint16_t>(fraction * BATTERY_ADC_MAX_RAW);
    }

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    return static_cast<uint16_t>(analogRead(adc_pin));
#else
    return static_cast<uint16_t>(BATTERY_ADC_MAX_RAW);
#endif
}

float BatteryMonitor::raw_to_voltage(uint16_t raw_adc) const {
    float fraction = static_cast<float>(raw_adc) / BATTERY_ADC_MAX_RAW;
    return BATTERY_VOLTAGE_MIN + fraction * (BATTERY_VOLTAGE_MAX - BATTERY_VOLTAGE_MIN);
}

void BatteryMonitor::add_sample(float voltage) {
    if (sample_window.size() >= window_size) {
        sample_window.erase(sample_window.begin());
    }
    sample_window.push_back(voltage);

    float sum = std::accumulate(sample_window.begin(), sample_window.end(), 0.0f);
    current_voltage = sum / static_cast<float>(sample_window.size());

    if (current_voltage < BATTERY_LOW_THRESHOLD) {
        low_power_flag = true;
    } else {
        low_power_flag = false;
        alert_broadcasted = false;
    }
}

float BatteryMonitor::sample_voltage() {
    uint16_t raw = read_raw_adc();
    float v = raw_to_voltage(raw);
    add_sample(v);
    return current_voltage;
}

float BatteryMonitor::get_smoothed_voltage() const {
    return current_voltage;
}

bool BatteryMonitor::is_low_power() const {
    return low_power_flag;
}

bool BatteryMonitor::check_and_alert(MeshNode& node) {
    if (low_power_flag && !alert_broadcasted) {
        std::cout << "[POWER WARN] Low battery detected: " 
                  << current_voltage << "V! Sending warning packet." << std::endl;

        // Broadcast STATUS_BATTERY_LOW payload containing the 4-byte float voltage
        uint8_t payload[sizeof(float)];
        std::memcpy(payload, &current_voltage, sizeof(float));

        bool sent = node.broadcast_payload(PacketType::STATUS_BATTERY_LOW, payload, sizeof(payload));
        if (sent) {
            alert_broadcasted = true;
            return true;
        }
    }
    return false;
}

void BatteryMonitor::set_simulated_voltage(float voltage) {
    simulation_mode = true;
    simulated_voltage = voltage;
}

void BatteryMonitor::reset() {
    sample_window.clear();
    current_voltage = 4.2f;
    low_power_flag = false;
    alert_broadcasted = false;
    simulation_mode = false;
}
