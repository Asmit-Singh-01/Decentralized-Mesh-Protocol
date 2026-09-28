#pragma once

#include <cstddef>
#include <cstdint>

// Compress telemetry using delta encoding,
// ZigZag encoding and variable-length integers.
size_t compress_telemetry(
    const int16_t* input,
    size_t count,
    uint8_t* output,
    size_t output_capacity
);

// Decompress telemetry.
size_t decompress_telemetry(
    const uint8_t* input,
    size_t input_size,
    int16_t* output,
    size_t output_capacity
);