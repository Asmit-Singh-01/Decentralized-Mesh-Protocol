#pragma once

#include <cstddef>
#include <cstdint>

#define MAX_TELEMETRY_SAMPLES 32

/**
 * @brief Compress an array of 16-bit signed telemetry samples using
 *        Delta Encoding combined with Run-Length Encoding (RLE).
 *
 * @param input Pointer to the source int16_t sample array.
 * @param count Number of int16_t samples to compress.
 * @param output Destination buffer for compressed telemetry stream.
 * @param max_output Maximum capacity of the output buffer in bytes (default: 64).
 * @return Number of compressed bytes written, or 0 on failure/overflow.
 */
size_t compress_telemetry(
    const int16_t* input,
    size_t count,
    uint8_t* output,
    size_t max_output = 64
);

/**
 * @brief Decompress telemetry stream back into original 16-bit samples.
 *
 * @param input Pointer to the compressed byte stream.
 * @param input_len Length of the compressed byte stream in bytes.
 * @param output Destination buffer for decompressed int16_t samples.
 * @param max_output_count Maximum capacity of output array in samples (default: 32).
 * @return Number of decompressed samples recovered, or 0 on error.
 */
size_t decompress_telemetry(
    const uint8_t* input,
    size_t input_len,
    int16_t* output,
    size_t max_output_count = MAX_TELEMETRY_SAMPLES
);

/**
 * @brief Benchmark helper to log telemetry compression ratio.
 *
 * Output format: [MESH COMPRESS] Original: 128B -> Compressed: 42B (67% saved)
 */
void print_compression_benchmark(size_t original_bytes, size_t compressed_bytes);
