#include <iostream>
#include <cstring>
#include "swarm_orchestrator.h"
#include "security_engine.h"
#include "mesh_node.h"
#include "cli.h"

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
    // 3. Interactive Serial CLI Verification (Issue #8)
    // =========================================================================
    std::cout << "\n[CLI] Initializing Serial CLI Command Interface..." << std::endl;
    MeshNode cli_node(0x1001);
    cli_node.init();

    // Populate routing table with simulated neighbor nodes
    MeshPacket pkt1;
    pkt1.header.magic = PROTOCOL_MAGIC_BYTE;
    pkt1.header.type = static_cast<uint8_t>(PacketType::BEACON);
    pkt1.header.sender_id = 0x2001;
    pkt1.header.receiver_id = 0x1001;
    pkt1.header.sequence_num = 1;
    pkt1.header.ttl = 10;
    pkt1.header.payload_len = 0;

    uint8_t buf[256];
    size_t raw_len = 0;
    serialize_packet(pkt1, buf, raw_len);
    cli_node.handle_received_packet(buf, raw_len, -85);

    MeshPacket pkt2;
    pkt2.header.magic = PROTOCOL_MAGIC_BYTE;
    pkt2.header.type = static_cast<uint8_t>(PacketType::BEACON);
    pkt2.header.sender_id = 0x2002;
    pkt2.header.receiver_id = 0x1001;
    pkt2.header.sequence_num = 2;
    pkt2.header.ttl = 10;
    pkt2.header.payload_len = 0;

    serialize_packet(pkt2, buf, raw_len);
    cli_node.handle_received_packet(buf, raw_len, -42);

    SerialCLI cli(cli_node, 0);

    // Test 3a: mesh status command
    std::cout << "\n--- Testing CLI: 'mesh status' ---" << std::endl;
    cli.process_command("mesh status", 12500);

    // Test 3b: mesh routes command
    std::cout << "\n--- Testing CLI: 'mesh routes' ---" << std::endl;
    cli.process_command("mesh routes");

    // Test 3c: mesh ping command
    std::cout << "\n--- Testing CLI: 'mesh ping 0x2002' ---" << std::endl;
    cli.process_command("mesh ping 0x2002");

    // Test 3d: unknown command handling
    std::cout << "\n--- Testing CLI: unknown command ---" << std::endl;
    cli.process_command("mesh unknown_cmd");

    std::cout << "\n>>> SYSTEM CORE & SECURITY LAYER FULLY OPERATIONAL <<<" << std::endl;
    return 0;
}

