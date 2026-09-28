#include "compression.h"

#include <cstddef>
#include <cstdint>
#include <limits>

// ZigZag encoding converts signed values to unsigned values
// so small negative and positive deltas both use fewer bytes.
static uint16_t zigzag_encode(int16_t value) {
    int32_t v = static_cast<int32_t>(value);
    return static_cast<uint16_t>((v << 1) ^ (v >> 15));
}

static int16_t zigzag_decode(uint16_t value) {
    int32_t decoded =
        static_cast<int32_t>(value >> 1) ^
        -static_cast<int32_t>(value & 1);

    return static_cast<int16_t>(decoded);
}

// Store an unsigned value using a simple variable-length format.
// Seven bits are stored per byte.
// The high bit indicates that another byte follows.
static size_t write_varuint(
    uint16_t value,
    uint8_t* output,
    size_t capacity
) {
    size_t index = 0;

    do {
        if (index >= capacity) {
            return 0;
        }

        uint8_t byte = static_cast<uint8_t>(value & 0x7F);
        value >>= 7;

        if (value != 0) {
            byte |= 0x80;
        }

        output[index++] = byte;

    } while (value != 0);

    return index;
}

static bool read_varuint(
    const uint8_t* input,
    size_t input_size,
    size_t& offset,
    uint16_t& value
) {
    value = 0;

    uint8_t shift = 0;

    while (offset < input_size && shift <= 14) {
        uint8_t byte = input[offset++];

        value |=
            static_cast<uint16_t>(byte & 0x7F) << shift;

        if ((byte & 0x80) == 0) {
            return true;
        }

        shift += 7;
    }

    return false;
}

size_t compress_telemetry(
    const int16_t* input,
    size_t count,
    uint8_t* output,
    size_t output_capacity
) {
    if (!input || !output || count == 0) {
        return 0;
    }

    size_t offset = 0;

    // Store the first value as two bytes.
    if (output_capacity < sizeof(int16_t)) {
        return 0;
    }

    int16_t first = input[0];

    output[offset++] =
        static_cast<uint8_t>(first & 0xFF);

    output[offset++] =
        static_cast<uint8_t>(
            (static_cast<uint16_t>(first) >> 8) & 0xFF
        );

    int16_t previous = first;

    for (size_t i = 1; i < count; ++i) {
        int32_t delta =
            static_cast<int32_t>(input[i]) -
            static_cast<int32_t>(previous);

        if (delta < std::numeric_limits<int16_t>::min() ||
            delta > std::numeric_limits<int16_t>::max()) {
            return 0;
        }

        uint16_t encoded =
            zigzag_encode(static_cast<int16_t>(delta));

        size_t written =
            write_varuint(
                encoded,
                output + offset,
                output_capacity - offset
            );

        if (written == 0) {
            return 0;
        }

        offset += written;
        previous = input[i];
    }

    return offset;
}

size_t decompress_telemetry(
    const uint8_t* input,
    size_t input_size,
    int16_t* output,
    size_t output_capacity
) {
    if (!input ||
        !output ||
        input_size < sizeof(int16_t) ||
        output_capacity == 0) {
        return 0;
    }

    size_t offset = 0;

    // Recover the first value.
    uint16_t first_raw =
        static_cast<uint16_t>(input[0]) |
        (static_cast<uint16_t>(input[1]) << 8);

    int16_t previous =
        static_cast<int16_t>(first_raw);

    output[0] = previous;

    offset = sizeof(int16_t);

    size_t sample_count = 1;

    while (offset < input_size) {
        if (sample_count >= output_capacity) {
            return 0;
        }

        uint16_t encoded = 0;

        if (!read_varuint(
                input,
                input_size,
                offset,
                encoded)) {
            return 0;
        }

        int16_t delta =
            zigzag_decode(encoded);

        int32_t reconstructed =
            static_cast<int32_t>(previous) +
            static_cast<int32_t>(delta);

        if (reconstructed <
                std::numeric_limits<int16_t>::min() ||
            reconstructed >
                std::numeric_limits<int16_t>::max()) {
            return 0;
        }

        previous =
            static_cast<int16_t>(reconstructed);

        output[sample_count++] = previous;
    }

    return sample_count;
}