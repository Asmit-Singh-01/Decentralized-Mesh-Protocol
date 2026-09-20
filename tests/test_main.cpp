#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <cassert>

#include "packet_format.h"
#include "routing_engine.h"
#include "swarm_orchestrator.h"
#include "security_engine.h"
#include "mesh_node.h"

// Lightweight, zero-dependency C++17 unit test harness
static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_CASE(name) void name(); \
    static void run_##name() { \
        g_tests_run++; \
        std::cout << "[ RUN      ] " #name << std::endl; \
        try { \
            name(); \
            g_tests_passed++; \
            std::cout << "\033[32m[       OK ]\033[0m " #name << std::endl; \
        } catch (const std::exception& e) { \
            g_tests_failed++; \
            std::cerr << "\033[31m[  FAILED  ]\033[0m " #name << " -> Exception: " << e.what() << std::endl; \
        } catch (...) { \
            g_tests_failed++; \
            std::cerr << "\033[31m[  FAILED  ]\033[0m " #name << " -> Unknown exception" << std::endl; \
        } \
    } \
    void name()

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        throw std::runtime_error(std::string("Assertion failed: ") + #cond + " at line " + std::to_string(__LINE__)); \
    } \
} while (0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        throw std::runtime_error(std::string("Assertion failed: ") + #a + " == " + #b + " at line " + std::to_string(__LINE__)); \
    } \
} while (0)

#define ASSERT_NE(a, b) do { \
    if ((a) == (b)) { \
        throw std::runtime_error(std::string("Assertion failed: ") + #a + " != " + #b + " at line " + std::to_string(__LINE__)); \
    } \
} while (0)

// ============================================================================
// 1. Packet Format & Serialization Tests
// ============================================================================

TEST_CASE(test_packet_serialization_roundtrip_valid) {
    MeshPacket tx_pkt{};
    tx_pkt.header.magic = PROTOCOL_MAGIC_BYTE;
    tx_pkt.header.type = static_cast<uint8_t>(PacketType::BEACON);
    tx_pkt.header.sender_id = 0x1001;
    tx_pkt.header.receiver_id = 0x2002;
    tx_pkt.header.sequence_num = 42;
    tx_pkt.header.ttl = 4;

    const char* payload = "HELLO_SWARM";
    tx_pkt.header.payload_len = static_cast<uint8_t>(std::strlen(payload));
    std::memcpy(tx_pkt.payload, payload, tx_pkt.header.payload_len);

    uint8_t buffer[128] = {0};
    size_t out_len = 0;
    ASSERT_TRUE(serialize_packet(tx_pkt, buffer, out_len));
    ASSERT_EQ(out_len, sizeof(PacketHeader) + tx_pkt.header.payload_len + sizeof(uint16_t));

    MeshPacket rx_pkt{};
    ASSERT_TRUE(deserialize_packet(buffer, out_len, rx_pkt));
    ASSERT_EQ(rx_pkt.header.magic, PROTOCOL_MAGIC_BYTE);
    ASSERT_EQ(rx_pkt.header.type, static_cast<uint8_t>(PacketType::BEACON));
    ASSERT_EQ(rx_pkt.header.sender_id, 0x1001);
    ASSERT_EQ(rx_pkt.header.receiver_id, 0x2002);
    ASSERT_EQ(rx_pkt.header.sequence_num, 42);
    ASSERT_EQ(rx_pkt.header.ttl, 4);
    ASSERT_EQ(rx_pkt.header.payload_len, tx_pkt.header.payload_len);
    ASSERT_EQ(std::memcmp(rx_pkt.payload, payload, rx_pkt.header.payload_len), 0);
}

TEST_CASE(test_packet_serialization_all_packet_types) {
    PacketType types[] = {
        PacketType::BEACON,
        PacketType::HEARTBEAT,
        PacketType::ROUTING_TABLE,
        PacketType::TELEMETRY_SWARM,
        PacketType::TASK_ALLOCATION,
        PacketType::ACK
    };

    for (auto type : types) {
        MeshPacket pkt{};
        pkt.header.magic = PROTOCOL_MAGIC_BYTE;
        pkt.header.type = static_cast<uint8_t>(type);
        pkt.header.sender_id = 0x01;
        pkt.header.receiver_id = 0x02;
        pkt.header.sequence_num = 1;
        pkt.header.ttl = 3;
        pkt.header.payload_len = 0;

        uint8_t buf[128];
        size_t len = 0;
        ASSERT_TRUE(serialize_packet(pkt, buf, len));

        MeshPacket out_pkt{};
        ASSERT_TRUE(deserialize_packet(buf, len, out_pkt));
        ASSERT_EQ(out_pkt.header.type, static_cast<uint8_t>(type));
    }
}

TEST_CASE(test_packet_serialization_empty_payload) {
    MeshPacket pkt{};
    pkt.header.magic = PROTOCOL_MAGIC_BYTE;
    pkt.header.type = static_cast<uint8_t>(PacketType::HEARTBEAT);
    pkt.header.sender_id = 0x1111;
    pkt.header.receiver_id = 0xFFFF;
    pkt.header.sequence_num = 10;
    pkt.header.ttl = 1;
    pkt.header.payload_len = 0;

    uint8_t buf[64];
    size_t len = 0;
    ASSERT_TRUE(serialize_packet(pkt, buf, len));
    ASSERT_EQ(len, sizeof(PacketHeader) + sizeof(uint16_t));

    MeshPacket rx{};
    ASSERT_TRUE(deserialize_packet(buf, len, rx));
    ASSERT_EQ(rx.header.payload_len, 0);
}

TEST_CASE(test_packet_serialization_max_payload) {
    MeshPacket pkt{};
    pkt.header.magic = PROTOCOL_MAGIC_BYTE;
    pkt.header.type = static_cast<uint8_t>(PacketType::TELEMETRY_SWARM);
    pkt.header.sender_id = 0x10;
    pkt.header.receiver_id = 0x20;
    pkt.header.sequence_num = 5;
    pkt.header.ttl = 5;
    pkt.header.payload_len = MAX_PAYLOAD_SIZE;

    for (size_t i = 0; i < MAX_PAYLOAD_SIZE; ++i) {
        pkt.payload[i] = static_cast<uint8_t>(i & 0xFF);
    }

    uint8_t buf[256];
    size_t len = 0;
    ASSERT_TRUE(serialize_packet(pkt, buf, len));
    ASSERT_EQ(len, sizeof(PacketHeader) + MAX_PAYLOAD_SIZE + sizeof(uint16_t));

    MeshPacket rx{};
    ASSERT_TRUE(deserialize_packet(buf, len, rx));
    ASSERT_EQ(rx.header.payload_len, MAX_PAYLOAD_SIZE);
    ASSERT_EQ(std::memcmp(rx.payload, pkt.payload, MAX_PAYLOAD_SIZE), 0);
}

TEST_CASE(test_packet_serialization_oversized_payload_rejected) {
    MeshPacket pkt{};
    pkt.header.magic = PROTOCOL_MAGIC_BYTE;
    pkt.header.payload_len = MAX_PAYLOAD_SIZE + 1; // 65 bytes exceeds MAX_PAYLOAD_SIZE

    uint8_t buf[256];
    size_t len = 0;
    ASSERT_FALSE(serialize_packet(pkt, buf, len));
}

TEST_CASE(test_packet_deserialization_truncated_buffer) {
    uint8_t small_buf[5] = { PROTOCOL_MAGIC_BYTE, 0x01, 0x00, 0x01, 0x00 };
    MeshPacket rx{};
    ASSERT_FALSE(deserialize_packet(small_buf, sizeof(small_buf), rx));
    ASSERT_FALSE(deserialize_packet(nullptr, 100, rx));
}

TEST_CASE(test_packet_deserialization_invalid_magic_rejected) {
    MeshPacket pkt{};
    pkt.header.magic = 0xAA; // Invalid magic byte (must be 0xD7)
    pkt.header.payload_len = 0;

    uint8_t buf[64];
    size_t len = 0;
    // serialize directly manually to test deserialize check
    std::memcpy(buf, &pkt.header, sizeof(PacketHeader));
    uint16_t crc = calculate_crc16(buf, sizeof(PacketHeader));
    std::memcpy(buf + sizeof(PacketHeader), &crc, sizeof(uint16_t));
    len = sizeof(PacketHeader) + sizeof(uint16_t);

    MeshPacket rx{};
    ASSERT_FALSE(deserialize_packet(buf, len, rx));
}

TEST_CASE(test_packet_corruption_detected_by_crc) {
    MeshPacket pkt{};
    pkt.header.magic = PROTOCOL_MAGIC_BYTE;
    pkt.header.type = static_cast<uint8_t>(PacketType::BEACON);
    pkt.header.sender_id = 0x100;
    pkt.header.receiver_id = 0x200;
    pkt.header.sequence_num = 99;
    pkt.header.ttl = 3;
    pkt.header.payload_len = 8;
    std::memcpy(pkt.payload, "12345678", 8);

    uint8_t buf[64];
    size_t len = 0;
    ASSERT_TRUE(serialize_packet(pkt, buf, len));

    // Corrupt a byte in the payload
    buf[sizeof(PacketHeader) + 2] ^= 0xFF;

    MeshPacket rx{};
    ASSERT_FALSE(deserialize_packet(buf, len, rx));
}

// ============================================================================
// 2. CRC16 Integrity Tests
// ============================================================================

TEST_CASE(test_crc16_deterministic_and_boundary) {
    ASSERT_EQ(calculate_crc16(nullptr, 0), 0);
    ASSERT_EQ(calculate_crc16(nullptr, 10), 0);

    const uint8_t test_data[] = "123456789";
    uint16_t crc1 = calculate_crc16(test_data, 9);
    uint16_t crc2 = calculate_crc16(test_data, 9);
    ASSERT_NE(crc1, 0);
    ASSERT_EQ(crc1, crc2);

    const uint8_t modified_data[] = "123456780";
    uint16_t crc3 = calculate_crc16(modified_data, 9);
    ASSERT_NE(crc1, crc3);
}

// ============================================================================
// 3. Routing Engine & Metric Calculation Tests
// ============================================================================

TEST_CASE(test_routing_engine_ignore_self_beacon) {
    RoutingEngine router(0x1001);
    router.process_beacon(0x1001, 0xFFFF, 1, -50, 1000);
    ASSERT_EQ(router.get_table().size(), 0);
}

TEST_CASE(test_routing_engine_insert_and_lookup) {
    RoutingEngine router(0x1001);
    router.process_beacon(0x2002, 0xFFFF, 1, -65, 1000);

    ASSERT_EQ(router.get_table().size(), 1);
    uint16_t next_hop = 0;
    ASSERT_TRUE(router.get_next_hop(0x2002, next_hop));
    ASSERT_EQ(next_hop, 0x2002);

    uint16_t unknown_next = 0;
    ASSERT_FALSE(router.get_next_hop(0x9999, unknown_next));
}

TEST_CASE(test_routing_engine_metric_comparison) {
    // Composite link metric: rssi - (hops * 10). Higher is better.
    // The routing engine uses: update if new_metric >= existing OR same next_hop sender.
    // Since sender_id is used as both key and next_hop on first insert,
    // same-sender beacons always refresh the route entry. We verify metric
    // direction separately using two distinct sender IDs.

    RoutingEngine router(0x1001);

    // Insert two separate peers with different metrics
    // Peer A (0x3003): hops=2, rssi=-70 -> metric = -70 - 20 = -90
    router.process_beacon(0x3003, 0xFFFF, 2, -70, 1000);
    // Peer B (0x4004): hops=1, rssi=-50 -> metric = -50 - 10 = -60 (better than A)
    router.process_beacon(0x4004, 0xFFFF, 1, -50, 1000);

    const auto& table = router.get_table();
    ASSERT_EQ(table.size(), 2);

    // Peer A should have hop=2, rssi=-70
    ASSERT_EQ(table.at(0x3003).hop_count, 2);
    ASSERT_EQ(table.at(0x3003).link_quality_rssi, -70);

    // Peer B should have hop=1, rssi=-50
    ASSERT_EQ(table.at(0x4004).hop_count, 1);
    ASSERT_EQ(table.at(0x4004).link_quality_rssi, -50);

    // Update Peer A with a superior route (hops=1, rssi=-45 -> metric = -45 - 10 = -55)
    // new_metric(-55) >= existing(-90) -> update
    router.process_beacon(0x3003, 0xFFFF, 1, -45, 2000);
    ASSERT_EQ(table.at(0x3003).hop_count, 1);
    ASSERT_EQ(table.at(0x3003).link_quality_rssi, -45);
    ASSERT_EQ(table.at(0x3003).last_updated_ms, 2000);

    // CRC & hop lookup still works after route upgrade
    uint16_t next_hop = 0;
    ASSERT_TRUE(router.get_next_hop(0x3003, next_hop));
    ASSERT_EQ(next_hop, 0x3003);
}

TEST_CASE(test_routing_engine_stale_route_pruning) {
    RoutingEngine router(0x1001);
    router.process_beacon(0x2001, 0xFFFF, 1, -50, 1000);
    router.process_beacon(0x2002, 0xFFFF, 1, -50, 5000);

    ASSERT_EQ(router.get_table().size(), 2);

    // Prune at t=12000 with timeout=10000ms:
    // For 0x2001: elapsed = 12000 - 1000 = 11000ms (> 10000) -> pruned!
    // For 0x2002: elapsed = 12000 - 5000 = 7000ms (<= 10000) -> kept!
    router.prune_stale_routes(12000, 10000);

    ASSERT_EQ(router.get_table().size(), 1);
    uint16_t next_hop = 0;
    ASSERT_FALSE(router.get_next_hop(0x2001, next_hop));
    ASSERT_TRUE(router.get_next_hop(0x2002, next_hop));
}

TEST_CASE(test_routing_engine_timer_wraparound) {
    RoutingEngine router(0x1001);
    // Last updated just before 32-bit uint rollover
    uint32_t near_max = 0xFFFFFFF0; // 4294967280
    router.process_beacon(0x4001, 0xFFFF, 1, -50, near_max);

    // Current time rolled over past 0 to 20ms
    uint32_t post_rollover = 20;
    // Elapsed should be: (0xFFFFFFFF - 0xFFFFFFF0 + 20) = 15 + 20 = 35ms <= 10000
    router.prune_stale_routes(post_rollover, 10000);
    ASSERT_EQ(router.get_table().size(), 1);

    // Now advance to time after timeout
    router.prune_stale_routes(post_rollover + 15000, 10000);
    ASSERT_EQ(router.get_table().size(), 0);
}

TEST_CASE(test_routing_engine_clear_table) {
    RoutingEngine router(0x1001);
    router.process_beacon(0x2001, 0xFFFF, 1, -50, 1000);
    router.process_beacon(0x2002, 0xFFFF, 1, -60, 1000);
    ASSERT_EQ(router.get_table().size(), 2);

    router.clear_table();
    ASSERT_EQ(router.get_table().size(), 0);
}

// ============================================================================
// 4. Swarm Orchestrator Tests
// ============================================================================

TEST_CASE(test_swarm_task_priority_ordering) {
    SwarmOrchestrator orchestrator(0x1001);
    orchestrator.init();

    orchestrator.assign_task(101, 2); // Lower priority
    orchestrator.assign_task(102, 9); // Higher priority
    orchestrator.assign_task(103, 5); // Medium priority

    const auto& tasks = orchestrator.get_tasks();
    ASSERT_EQ(tasks.size(), 3);
    ASSERT_EQ(tasks[0].task_id, 102);
    ASSERT_EQ(tasks[0].priority, 9);
    ASSERT_EQ(tasks[1].task_id, 103);
    ASSERT_EQ(tasks[1].priority, 5);
    ASSERT_EQ(tasks[2].task_id, 101);
    ASSERT_EQ(tasks[2].priority, 2);
}

TEST_CASE(test_swarm_task_deduplication) {
    SwarmOrchestrator orchestrator(0x1001);
    orchestrator.init();

    orchestrator.assign_task(200, 5);
    orchestrator.assign_task(200, 8); // Duplicate ID should be ignored

    const auto& tasks = orchestrator.get_tasks();
    ASSERT_EQ(tasks.size(), 1);
    ASSERT_EQ(tasks[0].task_id, 200);
    ASSERT_EQ(tasks[0].priority, 5);
}

TEST_CASE(test_swarm_incoming_task_payload_processing) {
    SwarmOrchestrator orchestrator(0x1001);
    orchestrator.init();

    uint16_t task_id = 350;
    uint8_t priority = 7;
    uint8_t payload[3];
    std::memcpy(payload, &task_id, sizeof(uint16_t));
    std::memcpy(payload + sizeof(uint16_t), &priority, sizeof(uint8_t));

    orchestrator.process_incoming_task(payload, sizeof(payload));

    const auto& tasks = orchestrator.get_tasks();
    ASSERT_EQ(tasks.size(), 1);
    ASSERT_EQ(tasks[0].task_id, 350);
    ASSERT_EQ(tasks[0].priority, 7);

    // Invalid length rejected
    orchestrator.process_incoming_task(payload, 2);
    ASSERT_EQ(orchestrator.get_tasks().size(), 1);
}

TEST_CASE(test_swarm_orchestration_cycle_and_cleanup) {
    SwarmOrchestrator orchestrator(0x1001);
    orchestrator.init();

    orchestrator.assign_task(401, 3);
    orchestrator.assign_task(402, 6);
    ASSERT_EQ(orchestrator.get_tasks().size(), 2);

    orchestrator.execute_orchestration_cycle(1000);
    // Completed tasks are garbage collected
    ASSERT_EQ(orchestrator.get_tasks().size(), 0);
}

TEST_CASE(test_swarm_clear_tasks) {
    SwarmOrchestrator orchestrator(0x1001);
    orchestrator.init();
    orchestrator.assign_task(501, 1);
    orchestrator.assign_task(502, 2);
    ASSERT_EQ(orchestrator.get_tasks().size(), 2);

    orchestrator.clear_tasks();
    ASSERT_EQ(orchestrator.get_tasks().size(), 0);
}

// ============================================================================
// 5. Security Engine Tests
// ============================================================================

TEST_CASE(test_security_engine_key_lifecycle) {
    SecurityEngine sec;
    uint8_t valid_key[16] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
    };

    ASSERT_TRUE(sec.set_key(valid_key, 16));
    ASSERT_FALSE(sec.set_key(valid_key, 15)); // Invalid length
    ASSERT_FALSE(sec.set_key(nullptr, 16));

    sec.clear_key();
    // After clearing, encryption should fail
    uint8_t plain[4] = {'T', 'E', 'S', 'T'};
    uint8_t cipher[4] = {0};
    uint8_t nonce[16] = {0};
    ASSERT_FALSE(sec.encrypt(plain, 4, cipher, nonce));
}

TEST_CASE(test_security_engine_roundtrip_encryption) {
    SecurityEngine sec;
    uint8_t key[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    uint8_t nonce[16] = {0xAA, 0x55, 0xAA, 0x55, 0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45, 0x67};
    ASSERT_TRUE(sec.set_key(key, 16));

    const char* message = "CONFIDENTIAL_SWARM_COORDINATE_DATA";
    size_t len = std::strlen(message);

    uint8_t cipher[64] = {0};
    uint8_t decrypted[64] = {0};

    ASSERT_TRUE(sec.encrypt(reinterpret_cast<const uint8_t*>(message), len, cipher, nonce));
    ASSERT_NE(std::memcmp(message, cipher, len), 0);

    ASSERT_TRUE(sec.decrypt(cipher, len, decrypted, nonce));
    ASSERT_EQ(std::memcmp(message, decrypted, len), 0);
}

TEST_CASE(test_security_engine_nonce_variation) {
    SecurityEngine sec;
    uint8_t key[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    uint8_t nonce1[16] = {0};
    uint8_t nonce2[16] = {1};
    ASSERT_TRUE(sec.set_key(key, 16));

    const char* message = "SAME_PLAINTEXT_PAYLOAD";
    size_t len = std::strlen(message);

    uint8_t cipher1[64] = {0};
    uint8_t cipher2[64] = {0};

    ASSERT_TRUE(sec.encrypt(reinterpret_cast<const uint8_t*>(message), len, cipher1, nonce1));
    ASSERT_TRUE(sec.encrypt(reinterpret_cast<const uint8_t*>(message), len, cipher2, nonce2));

    ASSERT_NE(std::memcmp(cipher1, cipher2, len), 0);
}

// ============================================================================
// 6. Mesh Node Deduplication & State Tests
// ============================================================================

TEST_CASE(test_mesh_node_deduplication_and_peer_tracking) {
    MeshNode node(0x1001);
    node.init();

    MeshPacket pkt{};
    pkt.header.magic = PROTOCOL_MAGIC_BYTE;
    pkt.header.type = static_cast<uint8_t>(PacketType::BEACON);
    pkt.header.sender_id = 0x2002;
    pkt.header.receiver_id = 0x1001;
    pkt.header.sequence_num = 100;
    pkt.header.ttl = 4;
    pkt.header.payload_len = 0;

    uint8_t raw[64];
    size_t len = 0;
    ASSERT_TRUE(serialize_packet(pkt, raw, len));

    // First arrival
    node.handle_received_packet(raw, len, -55, 1000);
    const auto& table = node.get_routing_table();
    ASSERT_EQ(table.size(), 1);
    ASSERT_EQ(table.at(0x2002).rssi, -55);
    ASSERT_EQ(table.at(0x2002).last_seen_ms, 1000);

    // Duplicate sequence number arrival
    node.handle_received_packet(raw, len, -40, 2000);
    // RSSI and last_seen should not have been updated because it's a duplicate sequence
    ASSERT_EQ(table.at(0x2002).last_seen_ms, 1000);

    // Cleanup dead peers
    node.cleanup_dead_peers(5000, 7000); // 7000 - 1000 = 6000 > 5000
    ASSERT_EQ(node.get_routing_table().size(), 0);
}

TEST_CASE(test_mesh_node_broadcast_and_send) {
    MeshNode node(0x5001);
    node.init();

    const uint8_t data[] = "NODE_DATA";
    ASSERT_TRUE(node.broadcast_payload(PacketType::TELEMETRY_SWARM, data, sizeof(data)));
    ASSERT_TRUE(node.send_to_node(0x5002, PacketType::TASK_ALLOCATION, data, sizeof(data)));

    // Rejects payload > MAX_PAYLOAD_SIZE
    uint8_t oversized[MAX_PAYLOAD_SIZE + 5];
    ASSERT_FALSE(node.broadcast_payload(PacketType::BEACON, oversized, sizeof(oversized)));
    ASSERT_FALSE(node.send_to_node(0x5002, PacketType::BEACON, oversized, sizeof(oversized)));
}

// ============================================================================
// Test Runner Main
// ============================================================================

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "   RUNNING AUTOMATED MESH PROTOCOL TEST SUITE     " << std::endl;
    std::cout << "==================================================" << std::endl;

    // 1. Packet format & serialization
    run_test_packet_serialization_roundtrip_valid();
    run_test_packet_serialization_all_packet_types();
    run_test_packet_serialization_empty_payload();
    run_test_packet_serialization_max_payload();
    run_test_packet_serialization_oversized_payload_rejected();
    run_test_packet_deserialization_truncated_buffer();
    run_test_packet_deserialization_invalid_magic_rejected();
    run_test_packet_corruption_detected_by_crc();

    // 2. CRC16
    run_test_crc16_deterministic_and_boundary();

    // 3. Routing Engine
    run_test_routing_engine_ignore_self_beacon();
    run_test_routing_engine_insert_and_lookup();
    run_test_routing_engine_metric_comparison();
    run_test_routing_engine_stale_route_pruning();
    run_test_routing_engine_timer_wraparound();
    run_test_routing_engine_clear_table();

    // 4. Swarm Orchestrator
    run_test_swarm_task_priority_ordering();
    run_test_swarm_task_deduplication();
    run_test_swarm_incoming_task_payload_processing();
    run_test_swarm_orchestration_cycle_and_cleanup();
    run_test_swarm_clear_tasks();

    // 5. Security Engine
    run_test_security_engine_key_lifecycle();
    run_test_security_engine_roundtrip_encryption();
    run_test_security_engine_nonce_variation();

    // 6. Mesh Node
    run_test_mesh_node_deduplication_and_peer_tracking();
    run_test_mesh_node_broadcast_and_send();

    std::cout << "==================================================" << std::endl;
    std::cout << "Tests Run   : " << g_tests_run << std::endl;
    std::cout << "Tests Passed: \033[32m" << g_tests_passed << "\033[0m" << std::endl;
    std::cout << "Tests Failed: " << (g_tests_failed > 0 ? "\033[31m" : "\033[32m") << g_tests_failed << "\033[0m" << std::endl;
    std::cout << "==================================================" << std::endl;

    return (g_tests_failed == 0) ? 0 : 1;
}
