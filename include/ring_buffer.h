#pragma once

#include "packet_format.h"
#include <cstddef>
#include <cstdint>
#include <array>

template <size_t CAPACITY = 32>
class PacketRingBuffer {
private:
    std::array<MeshPacket, CAPACITY> buffer;
    size_t head;
    size_t tail;
    size_t count;

public:
    PacketRingBuffer() : head(0), tail(0), count(0) {}

    bool push(const MeshPacket& packet) {
        if (count >= CAPACITY) {
            return false; // Buffer overflow safety
        }
        buffer[tail] = packet;
        tail = (tail + 1) % CAPACITY;
        count++;
        return true;
    }

    bool pop(MeshPacket& packet_out) {
        if (count == 0) {
            return false; // Buffer underflow safety
        }
        packet_out = buffer[head];
        head = (head + 1) % CAPACITY;
        count--;
        return true;
    }

    bool is_empty() const { return count == 0; }
    bool is_full() const { return count >= CAPACITY; }
    size_t size() const { return count; }
    void clear() { head = 0; tail = 0; count = 0; }
};
