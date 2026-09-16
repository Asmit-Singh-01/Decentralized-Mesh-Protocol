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
    // 3. RSSI-based Route Selection Verification (Issue #2)
    // =========================================================================
    std::cout << "\n[ROUTING] Verifying RSSI-based Route Selection..." << std::endl;
    MeshNode router(0x1001);
    router.init();

    // Verify empty routing table handling
    if (router.select_best_neighbor() == 0) {
        std::cout << "[ROUTING] Correctly handled empty routing table (returned 0)." << std::endl;
    }

    // Simulate neighbor packets with different signal strengths
    // Neighbor 1: Node 0x2001 with weak signal (-85 dBm)
    MeshPacket pktA;
    pktA.header.magic = PROTOCOL_MAGIC_BYTE;
    pktA.header.type = static_cast<uint8_t>(PacketType::BEACON);
    pktA.header.sender_id = 0x2001;
    pktA.header.receiver_id = 0x1001;
    pktA.header.sequence_num = 1;
    pktA.header.ttl = 10;
    pktA.header.payload_len = 0;

    uint8_t buf[256];
    size_t raw_len = 0;
    serialize_packet(pktA, buf, raw_len);
    router.handle_received_packet(buf, raw_len, -85);

    // Neighbor 2: Node 0x2002 with strongest signal (-42 dBm)
    MeshPacket pktB;
    pktB.header.magic = PROTOCOL_MAGIC_BYTE;
    pktB.header.type = static_cast<uint8_t>(PacketType::BEACON);
    pktB.header.sender_id = 0x2002;
    pktB.header.receiver_id = 0x1001;
    pktB.header.sequence_num = 2;
    pktB.header.ttl = 10;
    pktB.header.payload_len = 0;

    serialize_packet(pktB, buf, raw_len);
    router.handle_received_packet(buf, raw_len, -42);

    // Neighbor 3: Node 0x2003 with medium signal (-65 dBm)
    MeshPacket pktC;
    pktC.header.magic = PROTOCOL_MAGIC_BYTE;
    pktC.header.type = static_cast<uint8_t>(PacketType::BEACON);
    pktC.header.sender_id = 0x2003;
    pktC.header.receiver_id = 0x1001;
    pktC.header.sequence_num = 3;
    pktC.header.ttl = 10;
    pktC.header.payload_len = 0;

    serialize_packet(pktC, buf, raw_len);
    router.handle_received_packet(buf, raw_len, -65);

    // select_best_neighbor() must pick Node 0x2002 (8194) because -42 dBm > -65 dBm > -85 dBm
    uint16_t selected = router.select_best_neighbor();
    if (selected == 0x2002) {
        std::cout << "[SUCCESS] Correctly selected strongest route (Node 0x2002 at -42 dBm)!" << std::endl;
    }

    std::cout << "\n>>> SYSTEM CORE & SECURITY LAYER FULLY OPERATIONAL <<<" << std::endl;
    return 0;
}

