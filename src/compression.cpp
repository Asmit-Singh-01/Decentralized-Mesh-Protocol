#include "compression.h"
#include <cstdio>
#include <cstring>

void print_compression_benchmark(size_t original_bytes, size_t compressed_bytes) {
    if (original_bytes == 0) return;
    size_t saved_percent = 0;
    if (original_bytes > compressed_bytes) {
        saved_percent = ((original_bytes - compressed_bytes) * 100) / original_bytes;
    }
    std::printf("[MESH COMPRESS] Original: %zuB -> Compressed: %zuB (%zu%% saved)\n",
                original_bytes, compressed_bytes, saved_percent);
}

size_t compress_telemetry(const int16_t* input, size_t count, uint8_t* output, size_t max_output) {
    if (!input || !output || count == 0 || count > 255) {
        return 0;
    }

    // Minimum needed for header: 1 byte count + 2 bytes initial value
    if (max_output < 3) {
        return 0;
    }

    size_t out_idx = 0;

    // Byte 0: Sample count
    output[out_idx++] = static_cast<uint8_t>(count);

    // Byte 1..2: First sample value (big-endian)
    int16_t prev = input[0];
    output[out_idx++] = static_cast<uint8_t>((prev >> 8) & 0xFF);
    output[out_idx++] = static_cast<uint8_t>(prev & 0xFF);

    size_t i = 1;
    while (i < count) {
        int16_t current = input[i];
        int32_t delta_32 = static_cast<int32_t>(current) - static_cast<int32_t>(prev);

        if (delta_32 >= -128 && delta_32 <= 127) {
            int8_t delta_8 = static_cast<int8_t>(delta_32);
            uint8_t run_len = 1;

            // Detect identical subsequent deltas (RLE run)
            while ((i + run_len < count) && (run_len < 255)) {
                int16_t next_val = input[i + run_len];
                int16_t prev_val = input[i + run_len - 1];
                int32_t next_delta = static_cast<int32_t>(next_val) - static_cast<int32_t>(prev_val);
                if (next_delta != delta_32) {
                    break;
                }
                run_len++;
            }

            if (run_len > 1) {
                // RLE encoding: [TAG 0x80] [RUN_LENGTH] [DELTA]
                if (out_idx + 3 > max_output) {
                    return 0; // Buffer overflow protection
                }
                output[out_idx++] = 0x80;
                output[out_idx++] = run_len;
                output[out_idx++] = static_cast<uint8_t>(delta_8);
                prev = input[i + run_len - 1];
                i += run_len;
            } else {
                // Single delta encoding: [TAG 0x00] [DELTA]
                if (out_idx + 2 > max_output) {
                    return 0; // Buffer overflow protection
                }
                output[out_idx++] = 0x00;
                output[out_idx++] = static_cast<uint8_t>(delta_8);
                prev = current;
                i++;
            }
        } else {
            // Absolute 16-bit value: [TAG 0xC0] [VAL_HI] [VAL_LO]
            if (out_idx + 3 > max_output) {
                return 0; // Buffer overflow protection
            }
            output[out_idx++] = 0xC0;
            output[out_idx++] = static_cast<uint8_t>((current >> 8) & 0xFF);
            output[out_idx++] = static_cast<uint8_t>(current & 0xFF);
            prev = current;
            i++;
        }
    }

    return out_idx;
}

size_t decompress_telemetry(const uint8_t* input, size_t input_len, int16_t* output, size_t max_output_count) {
    if (!input || !output || input_len < 3) {
        return 0;
    }

    size_t in_idx = 0;
    size_t count = input[in_idx++];
    if (count == 0 || count > max_output_count) {
        return 0;
    }

    int16_t prev = static_cast<int16_t>((static_cast<uint16_t>(input[in_idx]) << 8) | input[in_idx + 1]);
    in_idx += 2;

    output[0] = prev;
    size_t out_idx = 1;

    while (in_idx < input_len && out_idx < count) {
        uint8_t tag = input[in_idx++];

        if (tag == 0x00) {
            // Single delta
            if (in_idx + 1 > input_len) return 0;
            int8_t delta = static_cast<int8_t>(input[in_idx++]);
            prev = static_cast<int16_t>(prev + delta);
            output[out_idx++] = prev;
        } else if (tag == 0x80) {
            // RLE delta run
            if (in_idx + 2 > input_len) return 0;
            uint8_t run_len = input[in_idx++];
            int8_t delta = static_cast<int8_t>(input[in_idx++]);
            for (uint8_t r = 0; r < run_len; ++r) {
                if (out_idx >= count) break;
                prev = static_cast<int16_t>(prev + delta);
                output[out_idx++] = prev;
            }
        } else if (tag == 0xC0) {
            // Absolute 16-bit value
            if (in_idx + 2 > input_len) return 0;
            int16_t val = static_cast<int16_t>((static_cast<uint16_t>(input[in_idx]) << 8) | input[in_idx + 1]);
            in_idx += 2;
            prev = val;
            output[out_idx++] = prev;
        } else {
            // Invalid tag
            return 0;
        }
    }

    return out_idx;
}
