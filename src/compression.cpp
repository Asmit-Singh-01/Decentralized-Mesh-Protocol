#include "compression.h"
#include <cstdio>

size_t compress_telemetry(const int16_t* input, size_t count, uint8_t* output) {
    if (input == nullptr || output == nullptr || count == 0) {
        return 0;
    }

    size_t out_idx = 0;

    output[out_idx++] = static_cast<uint8_t>((count >> 8) & 0xFF);
    output[out_idx++] = static_cast<uint8_t>(count & 0xFF);

    int16_t prev = input[0];
    output[out_idx++] = static_cast<uint8_t>((prev >> 8) & 0xFF);
    output[out_idx++] = static_cast<uint8_t>(prev & 0xFF);

    size_t i = 1;
    while (i < count) {
        int16_t current = input[i];
        int32_t delta_32 = static_cast<int32_t>(current) - static_cast<int32_t>(prev);

        if (delta_32 >= -128 && delta_32 <= 127) {
            int8_t delta_8 = static_cast<int8_t>(delta_32);
            uint8_t run_length = 1;
            while (i + run_length < count && run_length < 255) {
                int16_t next_val = input[i + run_length];
                int16_t prev_val = input[i + run_length - 1];
                int32_t next_delta = static_cast<int32_t>(next_val) - static_cast<int32_t>(prev_val);
                if (next_delta != delta_32) break;
                run_length++;
            }

            if (run_length > 1) {
                output[out_idx++] = 0x80;
                output[out_idx++] = static_cast<uint8_t>(delta_8);
                output[out_idx++] = run_length;
                prev = input[i + run_length - 1];
                i += run_length;
            } else {
                output[out_idx++] = 0x00;
                output[out_idx++] = static_cast<uint8_t>(delta_8);
                prev = current;
                i++;
            }
        } else {
            output[out_idx++] = 0xC0;
            output[out_idx++] = static_cast<uint8_t>((current >> 8) & 0xFF);
            output[out_idx++] = static_cast<uint8_t>(current & 0xFF);
            prev = current;
            i++;
        }
    }

    return out_idx;
}

size_t decompress_telemetry(const uint8_t* input, size_t input_bytes, int16_t* output) {
    if (input == nullptr || output == nullptr || input_bytes < 4) {
        return 0;
    }

    size_t in_idx = 0;
    size_t count = (static_cast<size_t>(input[in_idx]) << 8) | input[in_idx + 1];
    in_idx += 2;

    int16_t prev = static_cast<int16_t>((static_cast<uint16_t>(input[in_idx]) << 8) | input[in_idx + 1]);
    in_idx += 2;

    output[0] = prev;
    size_t out_idx = 1;

    while (in_idx < input_bytes && out_idx < count) {
        uint8_t flag = input[in_idx++];
        if (flag == 0x00) {
            int8_t delta = static_cast<int8_t>(input[in_idx++]);
            prev = static_cast<int16_t>(prev + delta);
            output[out_idx++] = prev;
        } else if (flag == 0x80) {
            int8_t delta = static_cast<int8_t>(input[in_idx++]);
            uint8_t run_length = input[in_idx++];
            for (uint8_t r = 0; r < run_length; ++r) {
                prev = static_cast<int16_t>(prev + delta);
                output[out_idx++] = prev;
            }
        } else if (flag == 0xC0) {
            int16_t raw_val = static_cast<int16_t>((static_cast<uint16_t>(input[in_idx]) << 8) | input[in_idx + 1]);
            in_idx += 2;
            prev = raw_val;
            output[out_idx++] = prev;
        }
    }

    return out_idx;
}

void print_compression_benchmark(size_t original_bytes, size_t compressed_bytes) {
    if (original_bytes == 0) return;
    double saved = ((double)(original_bytes - compressed_bytes) / (double)original_bytes) * 100.0;
    if (saved < 0.0) saved = 0.0;
    std::printf("[MESH COMPRESS] Original: %zuB -> Compressed: %zuB (%.0f%% saved)\n",
                original_bytes, compressed_bytes, saved);
}
