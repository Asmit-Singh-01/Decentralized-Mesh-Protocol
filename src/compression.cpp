#include "compression.h"

#include <limits>

namespace {

constexpr size_t MAX_COMPRESSED_SIZE = 64;
constexpr size_t MAX_TELEMETRY_SAMPLES = 32;

uint32_t zigzag_encode(int32_t value)
{
    return (static_cast<uint32_t>(value) << 1) ^
           static_cast<uint32_t>(value >> 31);
}

int32_t zigzag_decode(uint32_t value)
{
    return static_cast<int32_t>((value >> 1) ^
                                static_cast<uint32_t>(
                                    -static_cast<int32_t>(value & 1)));
}

bool write_varint(uint32_t value, uint8_t* output, size_t& position)
{
    while (value >= 0x80U) {
        if (position >= MAX_COMPRESSED_SIZE) {
            return false;
        }

        output[position++] =
            static_cast<uint8_t>((value & 0x7FU) | 0x80U);

        value >>= 7;
    }

    if (position >= MAX_COMPRESSED_SIZE) {
        return false;
    }

    output[position++] = static_cast<uint8_t>(value);
    return true;
}

bool read_varint(
    const uint8_t* input,
    size_t input_size,
    size_t& position,
    uint32_t& value)
{
    value = 0;

    for (unsigned int shift = 0; shift <= 28; shift += 7) {
        if (position >= input_size) {
            return false;
        }

        uint8_t byte = input[position++];

        uint32_t part = static_cast<uint32_t>(byte & 0x7FU);

        if (shift == 28 && part > 0x0FU) {
            return false;
        }

        value |= part << shift;

        if ((byte & 0x80U) == 0) {
            return true;
        }
    }

    return false;
}

} // namespace

size_t compress_telemetry(
    const int16_t* input,
    size_t count,
    uint8_t* output)
{
    if (input == nullptr || output == nullptr) {
        return 0;
    }

    if (count == 0 || count > MAX_TELEMETRY_SAMPLES) {
        return 0;
    }

    size_t position = 0;

    // Store the number of samples so the receiver knows
    // how many values to reconstruct.
    output[position++] = static_cast<uint8_t>(count);

    int32_t previous = input[0];

    if (!write_varint(
            zigzag_encode(previous),
            output,
            position)) {
        return 0;
    }

    for (size_t i = 1; i < count; ++i) {
        int32_t current = input[i];
        int32_t delta = current - previous;

        if (!write_varint(
                zigzag_encode(delta),
                output,
                position)) {
            return 0;
        }

        previous = current;
    }

    return position;
}

size_t decompress_telemetry(
    const uint8_t* input,
    size_t input_size,
    int16_t* output,
    size_t output_capacity)
{
    if (input == nullptr || output == nullptr) {
        return 0;
    }

    if (input_size < 2) {
        return 0;
    }

    size_t position = 0;

    const size_t count = input[position++];

    if (count == 0 ||
        count > MAX_TELEMETRY_SAMPLES ||
        count > output_capacity) {
        return 0;
    }

    uint32_t encoded_value = 0;

    if (!read_varint(
            input,
            input_size,
            position,
            encoded_value)) {
        return 0;
    }

    int32_t current = zigzag_decode(encoded_value);

    if (current < std::numeric_limits<int16_t>::min() ||
        current > std::numeric_limits<int16_t>::max()) {
        return 0;
    }

    output[0] = static_cast<int16_t>(current);

    for (size_t i = 1; i < count; ++i) {
        if (!read_varint(
                input,
                input_size,
                position,
                encoded_value)) {
            return 0;
        }

        const int32_t delta = zigzag_decode(encoded_value);
        const int32_t next = current + delta;

        if (next < std::numeric_limits<int16_t>::min() ||
            next > std::numeric_limits<int16_t>::max()) {
            return 0;
        }

        output[i] = static_cast<int16_t>(next);
        current = next;
    }

    return count;
}
