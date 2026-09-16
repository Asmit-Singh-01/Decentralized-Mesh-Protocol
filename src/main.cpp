#include <iostream>
#include <cstring>
#include <string>
#include "swarm_orchestrator.h"
#include "security_engine.h"
#include "packet_format.h"

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

    // 3. Issue #12: Standalone Payload Wrapper & Packet Header Encryption Flag Verification
    std::cout << "\n[SECURITY ISSUE #12] Testing encrypt_payload and decrypt_payload wrappers..." << std::endl;
    const uint8_t mesh_key[AES_KEY_SIZE] = DEFAULT_MESH_KEY;
    const char* sensitive_telemetry = "NODE_0x1001_GPS:37.7749,-122.4194_BATTERY:98%";
    size_t tel_len = std::strlen(sensitive_telemetry);

    MeshPacket packet = {};
    packet.header.magic = PROTOCOL_MAGIC_BYTE;
    packet.header.type = static_cast<uint8_t>(PacketType::TELEMETRY_SWARM);
    packet.header.sender_id = 0x1001;
    packet.header.receiver_id = 0x1002;
    packet.header.sequence_num = 42;
    packet.header.ttl = 5;
    packet.header.payload_len = static_cast<uint8_t>(tel_len);
    packet.header.is_encrypted = 1;
    std::memcpy(packet.payload, sensitive_telemetry, tel_len);

    std::cout << "[SECURITY] Plaintext payload: \"" << sensitive_telemetry << "\"" << std::endl;

    encrypt_payload(packet.payload, packet.header.payload_len, mesh_key);
    std::cout << "[SECURITY] Ciphertext encrypted in-place (verified != plaintext)" << std::endl;
    if (std::memcmp(packet.payload, sensitive_telemetry, tel_len) == 0) {
        std::cerr << "[SECURITY FAIL] Payload was not encrypted!" << std::endl;
        return 1;
    }

    uint8_t tx_buffer[256];
    size_t tx_len = 0;
    if (!serialize_packet(packet, tx_buffer, tx_len)) {
        std::cerr << "[SECURITY FAIL] Serialization failed!" << std::endl;
        return 1;
    }

    MeshPacket rx_packet = {};
    if (!deserialize_packet(tx_buffer, tx_len, rx_packet)) {
        std::cerr << "[SECURITY FAIL] Deserialization failed!" << std::endl;
        return 1;
    }

    if (rx_packet.header.is_encrypted) {
        std::cout << "[SECURITY] Receiver detected encrypted packet header flag (is_encrypted=1)." << std::endl;
        bool ok = decrypt_payload(rx_packet.payload, rx_packet.header.payload_len, mesh_key);
        if (!ok || std::memcmp(rx_packet.payload, sensitive_telemetry, tel_len) != 0) {
            std::cerr << "[SECURITY FAIL] Wrapper decryption failed or data mismatch!" << std::endl;
            return 1;
        }
        std::cout << "[SECURITY SUCCESS] Decrypted payload: \""
                  << std::string(reinterpret_cast<char*>(rx_packet.payload), rx_packet.header.payload_len)
                  << "\" (100% Match!)" << std::endl;
    } else {
        std::cerr << "[SECURITY FAIL] Header is_encrypted flag missing on receiver!" << std::endl;
        return 1;
    }

    std::cout << "\n>>> SYSTEM CORE & SECURITY LAYER FULLY OPERATIONAL <<<" << std::endl;
    return 0;
}

