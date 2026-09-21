#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <cstdint>
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

size_t compress_telemetry(const int16_t* input, size_t count, uint8_t* output);
size_t decompress_telemetry(const uint8_t* input, size_t input_bytes, int16_t* output);
void print_compression_benchmark(size_t original_bytes, size_t compressed_bytes);

#ifdef __cplusplus
}
#endif

#endif // COMPRESSION_H
