#pragma once
#include <stdint.h>
#include <stddef.h>
#include <vector>
#include "mesh_node.h"

#define DEFAULT_BATTERY_PIN 34
#define BATTERY_VOLTAGE_MIN 3.2f
#define BATTERY_VOLTAGE_MAX 4.2f
#define BATTERY_LOW_THRESHOLD 3.3f
#define BATTERY_ADC_MAX_RAW 4095.0f

class BatteryMonitor {
private:
    uint8_t adc_pin;
    float current_voltage;
    bool low_power_flag;
    bool alert_broadcasted;
    std::vector<float> sample_window;
    size_t window_size;
    bool simulation_mode;
    float simulated_voltage;

public:
    explicit BatteryMonitor(uint8_t pin = DEFAULT_BATTERY_PIN, size_t smoothing_samples = 5);

    // ADC Measurement & Mapping
    uint16_t read_raw_adc();
    float raw_to_voltage(uint16_t raw_adc) const;
    float sample_voltage();

    // Moving Average Smoothing
    void add_sample(float voltage);
    float get_smoothed_voltage() const;

    // Status & Threshold Evaluation
    bool is_low_power() const;
    bool check_and_alert(MeshNode& node);

    // Simulation / Host Environment Hooks
    void set_simulated_voltage(float voltage);
    void reset();
};
