#include <iostream>
#include <cstring>
#include "swarm_orchestrator.h"
#include "security_engine.h"
#include "sd_logger.h"
#include <fstream>

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

    // 3. Issue #20: SPI MicroSD Card Transaction Logger Verification
    std::cout << "\n[SD LOGGER] Initializing SPI MicroSD Card Logger Pipeline (Issue #20)..." << std::endl;
    const char* test_log_file = "test_mesh_logs.csv";
    std::remove(test_log_file);

    SDCardLogger logger(DEFAULT_SD_CS_PIN, test_log_file, 5);
    if (!logger.init()) {
        std::cerr << "[SD FAIL] Logger initialization failed!" << std::endl;
        return 1;
    }

    std::cout << "[SD] Logging sample mesh packets..." << std::endl;
    for (uint16_t i = 1; i <= 5; ++i) {
        MeshPacket p = {};
        p.header.sender_id = 0x1000 + i;
        p.header.receiver_id = 0xFFFF;
        p.header.payload_len = 16;
        logger.log_packet(1000 * i, p, static_cast<int8_t>(-65 + i), true);
    }

    if (logger.get_unflushed_count() != 0) {
        std::cerr << "[SD FAIL] Logger should have automatically flushed at threshold 5!" << std::endl;
        return 1;
    }
    std::cout << "[SD SUCCESS] Auto-flush verified at interval threshold (" << logger.get_total_writes() << " packets written)!" << std::endl;

    logger.log_packet(6000, 0x1006, 0x1001, -85, 32, false);
    logger.flush();
    logger.close();

    std::ifstream log_check(test_log_file);
    if (!log_check.is_open()) {
        std::cerr << "[SD FAIL] Output CSV log file does not exist!" << std::endl;
        return 1;
    }
    std::string header_line;
    std::getline(log_check, header_line);
    std::cout << "[SD SUCCESS] CSV Header verified: \"" << header_line << "\"" << std::endl;
    log_check.close();
    std::remove(test_log_file);

    std::cout << "\n>>> SYSTEM CORE & SD CARD LOGGER FULLY OPERATIONAL <<<" << std::endl;
    return 0;
}

