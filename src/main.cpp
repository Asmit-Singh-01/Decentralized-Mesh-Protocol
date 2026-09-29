#include <iostream>
#include <cstring>
#include "swarm_orchestrator.h"
#include "security_engine.h"
#include "packet_format.h"
#include "ota_updater.h"

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

    // 3. Issue #17: ESP-NOW / LoRa OTA Chunked Firmware Update Verification
    std::cout << "\n[OTA TEST #17] Initializing Over-The-Air Firmware Update Test..." << std::endl;
    OtaUpdater ota;

    // Simulate 150-byte compiled firmware binary payload
    uint8_t test_firmware[150];
    for (size_t i = 0; i < sizeof(test_firmware); ++i) {
        test_firmware[i] = static_cast<uint8_t>((i * 7 + 0x33) & 0xFF);
    }

    uint32_t expected_crc = calculate_crc32(test_firmware, sizeof(test_firmware)) ^ 0xFFFFFFFF;
    const uint16_t chunk_count = 3;
    const size_t chunk_size = 50;

    // Step A: Send OTA_START packet
    OtaStartPacket start_pkt{};
    start_pkt.cmd = OTA_CMD_START;
    start_pkt.total_chunks = chunk_count;
    start_pkt.chunk_size = static_cast<uint16_t>(chunk_size);
    start_pkt.firmware_size = static_cast<uint32_t>(sizeof(test_firmware));
    start_pkt.expected_crc32 = expected_crc;

    if (!ota.handle_packet(reinterpret_cast<const uint8_t*>(&start_pkt), sizeof(start_pkt))) {
        std::cerr << "[OTA FAIL] Failed to initiate OTA session!" << std::endl;
        return 1;
    }

    // Step B: Send Chunks out-of-order to test missing-chunk tracking (Send Chunk 0 and Chunk 2, skipping Chunk 1)
    std::cout << "[OTA TEST] Simulating out-of-order arrival and packet loss..." << std::endl;

    // Chunk 0
    uint8_t chunk_buf[64] = {0};
    chunk_buf[0] = OTA_CMD_CHUNK;
    uint16_t c0 = 0;
    std::memcpy(chunk_buf + 1, &c0, sizeof(uint16_t));
    chunk_buf[3] = static_cast<uint8_t>(chunk_size);
    std::memcpy(chunk_buf + 4, test_firmware + 0, chunk_size);
    ota.handle_packet(chunk_buf, 4 + chunk_size);

    // Chunk 2 (skipping Chunk 1)
    uint16_t c2 = 2;
    std::memcpy(chunk_buf + 1, &c2, sizeof(uint16_t));
    chunk_buf[3] = static_cast<uint8_t>(chunk_size);
    std::memcpy(chunk_buf + 4, test_firmware + 100, chunk_size);
    ota.handle_packet(chunk_buf, 4 + chunk_size);

    // Verify missing-chunk tracking detected Chunk 1 is missing
    if (ota.get_missing_chunks_count() != 1) {
        std::cerr << "[OTA FAIL] Missing-chunk tracking failed to detect lost chunk!" << std::endl;
        return 1;
    }
    std::cout << "[OTA TRACKING] Successfully identified 1 missing chunk before completion." << std::endl;

    // Attempting completion while chunk is missing must fail
    if (ota.complete()) {
        std::cerr << "[OTA FAIL] OTA completion should have failed with missing chunks!" << std::endl;
        return 1;
    }

    // Reset session and transmit all chunks sequentially
    ota.reset();
    ota.begin(static_cast<uint32_t>(sizeof(test_firmware)), chunk_count, expected_crc, static_cast<uint16_t>(chunk_size));

    for (uint16_t c = 0; c < chunk_count; ++c) {
        chunk_buf[0] = OTA_CMD_CHUNK;
        std::memcpy(chunk_buf + 1, &c, sizeof(uint16_t));
        chunk_buf[3] = static_cast<uint8_t>(chunk_size);
        std::memcpy(chunk_buf + 4, test_firmware + (c * chunk_size), chunk_size);
        ota.handle_packet(chunk_buf, 4 + chunk_size);
    }

    // Step C: Send OTA_END packet
    OtaEndPacket end_pkt{};
    end_pkt.cmd = OTA_CMD_END;
    end_pkt.expected_crc32 = expected_crc;

    if (!ota.handle_packet(reinterpret_cast<const uint8_t*>(&end_pkt), sizeof(end_pkt))) {
        std::cerr << "[OTA FAIL] Final OTA verification failed!" << std::endl;
        return 1;
    }

    if (ota.get_state() == OtaState::COMPLETED &&
        std::memcmp(ota.get_mock_flash(), test_firmware, sizeof(test_firmware)) == 0) {
        std::cout << "[OTA SUCCESS] All " << chunk_count << " chunks verified with 100% binary flash integrity!" << std::endl;
    } else {
        std::cerr << "[OTA FAIL] Flash binary data mismatch!" << std::endl;
        return 1;
    }

    std::cout << "\n>>> SYSTEM CORE & SECURITY LAYER FULLY OPERATIONAL <<<" << std::endl;
    return 0;
}

