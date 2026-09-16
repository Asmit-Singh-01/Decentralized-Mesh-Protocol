#pragma once

#include <cstddef>
#include <cstdint>

/**
 * Compress an array of signed 16-bit telemetry values using
 * delta encoding followed by ZigZag + variable-length encoding.
 *
 * The encoded format is:
 *   [sample_count: 1 byte]
 *   [first value as ZigZag varint]
 *   [subsequent deltas as ZigZag varints]
 *
 * Returns the number of bytes written to output.
 * Returns 0 if the input/output pointers are invalid or
 * the output buffer cannot hold the compressed data.
 */
size_t compress_telemetry(
    const int16_t* input,
    size_t count,
    uint8_t* output
);

/**
 * Decompress telemetry produced by compress_telemetry().
 *
 * Returns the number of int16_t samples written to output.
 * Returns 0 if the encoded data is invalid or the output
 * buffer is too small.
 */
size_t decompress_telemetry(
    const uint8_t* input,
    size_t input_size,
    int16_t* output,
    size_t output_capacity
);
