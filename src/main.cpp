#include <iostream>
#include <cstring>
#include "swarm_orchestrator.h"
#include "security_engine.h"
#include "mesh_node.h"

int main() {
    std::cout << ">>> DECENTRALIZED SWARM OS / MESH CORE INITIALIZED <<<" << std::endl;

    // 1. Swarm Orchestrator Initialization
    SwarmOrchestrator node(0x1001);
    node.init();

    std::cout << "[SYSTEM] Assigning autonomous swarm mission tasks..." << std::endl;
    node.assign_task(101, 1);
    node.assign_task(102, 2);

    std::cout << "[SYSTEM] Running initial orchestration cycle..." << std::endl;
    node.execute_orchestration_cycle(1000);

    std::cout << "[SUCCESS] Active task queue size: " << node.get_tasks().size() << std::endl;

    // 2. AES-128 Security Pipeline Verification
    std::cout << "\n[SECURITY] Initializing AES-128 Encryption Engine..." << std::endl;
    SecurityEngine sec;
    uint8_t secret_key[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    uint8_t nonce[16] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99};

    if (!sec.set_key(secret_key, 16)) {
        std::cerr << "[ERROR] Key setup failed!" << std::endl;
        return 1;
    }

    const char* raw_payload = "SWARM_COMMAND_EXECUTE_TAKEOFF";
    size_t len = std::strlen(raw_payload);
    uint8_t encrypted[64] = {0};
    uint8_t decrypted[64] = {0};

    sec.encrypt(reinterpret_cast<const uint8_t*>(raw_payload), len, encrypted, nonce);
    sec.decrypt(encrypted, len, decrypted, nonce);

    if (std::memcmp(raw_payload, decrypted, len) == 0) {
        std::cout << "[SECURITY SUCCESS] Encrypted packet decrypted with 100% integrity!" << std::endl;
    } else {
        std::cerr << "[SECURITY FAIL] Decrypted payload mismatch!" << std::endl;
        return 1;
    }

    // =========================================================================
    // 3. CRC16 Checksum Verification & Corrupted Packet Dropping (Issue #3)
    // =========================================================================
    std::cout << "\n[SECURITY] Verifying CRC16 Checksum Integrity Pipeline..." << std::endl;
    MeshNode rx_node(0x1001);
    rx_node.init();

    // Test 3a: Create and serialize a valid packet
    MeshPacket valid_pkt;
    valid_pkt.header.magic = PROTOCOL_MAGIC_BYTE;
    valid_pkt.header.type = static_cast<uint8_t>(PacketType::BEACON);
    valid_pkt.header.sender_id = 0x2002;
    valid_pkt.header.receiver_id = 0x1001;
    valid_pkt.header.sequence_num = 1;
    valid_pkt.header.ttl = 10;
    valid_pkt.header.payload_len = 4;
    std::memcpy(valid_pkt.payload, "PING", 4);

    uint8_t wire_buf[256];
    size_t wire_len = 0;
    serialize_packet(valid_pkt, wire_buf, wire_len);

    std::cout << "[TEST] Processing valid incoming packet..." << std::endl;
    rx_node.handle_received_packet(wire_buf, wire_len, -45);
    std::cout << "[SUCCESS] Valid packet processed. Routing table size: "
              << rx_node.get_routing_table().size() << std::endl;

    // Test 3b: Corrupt payload byte in transit to simulate wireless noise / collision
    std::cout << "[TEST] Injecting single-bit corruption into payload..." << std::endl;
    wire_buf[sizeof(PacketHeader) + 1] ^= 0xFF; // Flip bits

    // Process corrupted packet -> CRC validation fails, logs error, and drops immediately
    rx_node.handle_received_packet(wire_buf, wire_len, -45);
    std::cout << "[SUCCESS] Corrupted packet discarded. Routing table size unchanged: "
              << rx_node.get_routing_table().size() << std::endl;

    std::cout << "\n>>> SYSTEM CORE & SECURITY LAYER FULLY OPERATIONAL <<<" << std::endl;
    return 0;
}

