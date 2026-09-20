#include <cassert>
#include <cstdint>
#include <iostream>

#include "mesh_node.h"
#include "packet_format.h"

namespace {

void test_heartbeat_interval() {
    MeshNode node(0x1001);
    node.init();

    // First call initializes the scheduler.
    assert(!node.service(0));

    // Heartbeat must not be emitted before 2000 ms.
    assert(!node.service(1999));

    // Exactly at the interval, heartbeat must be emitted.
    assert(node.service(2000));

    // No immediate second heartbeat.
    assert(!node.service(2001));

    // Next heartbeat after another full interval.
    assert(node.service(4000));
}

void test_peer_timeout_boundary() {
    MeshNode node(0x1001);
    node.init();

    MeshPacket packet{};

    packet.header.magic = PROTOCOL_MAGIC_BYTE;
    packet.header.type =
        static_cast<uint8_t>(PacketType::HEARTBEAT);
    packet.header.sender_id = 0x1002;
    packet.header.receiver_id = 0xFFFF;
    packet.header.sequence_num = 1;
    packet.header.ttl = 5;
    packet.header.payload_len = 0;

    uint8_t buffer[256] = {};
    size_t serialized_len = 0;

    assert(serialize_packet(
        packet,
        buffer,
        serialized_len
    ));

    // Peer is first seen at 1000 ms.
    node.handle_received_packet(
        buffer,
        serialized_len,
        -45,
        1000
    );

    assert(node.get_routing_table().size() == 1);

    // Exactly at timeout: peer is still considered alive.
    node.service(7000);

    assert(node.get_routing_table().size() == 1);

    // One millisecond beyond timeout: peer must be removed.
    node.service(7001);

    assert(node.get_routing_table().empty());
}

void test_heartbeat_refreshes_peer_liveness() {
    MeshNode node(0x1001);
    node.init();

    MeshPacket first_heartbeat{};

    first_heartbeat.header.magic = PROTOCOL_MAGIC_BYTE;
    first_heartbeat.header.type =
        static_cast<uint8_t>(PacketType::HEARTBEAT);
    first_heartbeat.header.sender_id = 0x1002;
    first_heartbeat.header.receiver_id = 0xFFFF;
    first_heartbeat.header.sequence_num = 1;
    first_heartbeat.header.ttl = 5;
    first_heartbeat.header.payload_len = 0;

    uint8_t first_buffer[256] = {};
    size_t first_len = 0;

    assert(serialize_packet(
        first_heartbeat,
        first_buffer,
        first_len
    ));

    // First heartbeat received at 1000 ms.
    node.handle_received_packet(
        first_buffer,
        first_len,
        -40,
        1000
    );

    assert(node.get_routing_table().size() == 1);

    MeshPacket second_heartbeat = first_heartbeat;

    // Different sequence number = new heartbeat.
    second_heartbeat.header.sequence_num = 2;

    uint8_t second_buffer[256] = {};
    size_t second_len = 0;

    assert(serialize_packet(
        second_heartbeat,
        second_buffer,
        second_len
    ));

    // Refresh peer at 6000 ms.
    node.handle_received_packet(
        second_buffer,
        second_len,
        -42,
        6000
    );

    assert(node.get_routing_table().size() == 1);

    // 6000 ms after the refreshed timestamp is still valid.
    node.service(12000);

    assert(node.get_routing_table().size() == 1);

    // One millisecond beyond the refreshed timeout.
    node.service(12001);

    assert(node.get_routing_table().empty());
}

void test_timer_wraparound() {
    MeshNode node(0x1001);
    node.init();

    const uint32_t near_wrap =
        UINT32_MAX - 999;

    // Initialize scheduler near uint32_t overflow.
    assert(!node.service(near_wrap));

    // 1000 ms after the initial timestamp, across wrap-around.
    assert(!node.service(0));

    // Another 1000 ms reaches the 2000 ms heartbeat interval.
    assert(node.service(1000));
}

} // namespace

int main() {
    std::cout
        << "[TEST] heartbeat interval..."
        << std::endl;

    test_heartbeat_interval();

    std::cout
        << "[PASS] heartbeat interval"
        << std::endl;


    std::cout
        << "[TEST] peer timeout boundary..."
        << std::endl;

    test_peer_timeout_boundary();

    std::cout
        << "[PASS] peer timeout boundary"
        << std::endl;


    std::cout
        << "[TEST] heartbeat refreshes peer liveness..."
        << std::endl;

    test_heartbeat_refreshes_peer_liveness();

    std::cout
        << "[PASS] heartbeat refreshes peer liveness"
        << std::endl;


    std::cout
        << "[TEST] timer wrap-around..."
        << std::endl;

    test_timer_wraparound();

    std::cout
        << "[PASS] timer wrap-around"
        << std::endl;


    std::cout
        << "\n[ALL TESTS PASSED]"
        << std::endl;

    return 0;
}