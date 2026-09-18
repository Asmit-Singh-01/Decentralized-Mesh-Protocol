#include <iostream>
#include <cstring>
#include "swarm_orchestrator.h"
#include "security_engine.h"
#include "power_manager.h"

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

    // 3. Issue #7: ESP32 Light-Sleep Power Management Duty-Cycle Verification
    std::cout << "\n[POWER] Initializing ESP32 Light-Sleep Power Manager Pipeline (Issue #7)..." << std::endl;
    MeshNode mesh_node(0x1001);
    mesh_node.init();
    PowerManager power_mgr(true, 500);

    uint32_t short_window = power_mgr.calculate_sleep_window(1000, 1300, 50);
    std::cout << "[POWER] Calculated short window: " << short_window << " ms (Threshold: 500 ms)" << std::endl;
    bool slept_short = power_mgr.enter_light_sleep(short_window, &mesh_node);
    if (slept_short) {
        std::cerr << "[POWER FAIL] Sleep should not occur when window is below 500ms threshold!" << std::endl;
        return 1;
    }

    uint32_t beacon_interval_window = power_mgr.calculate_sleep_window(2000, 3550, 50);
    std::cout << "[POWER] Calculated valid sleep window: " << beacon_interval_window << " ms" << std::endl;
    bool slept_valid = power_mgr.enter_light_sleep(beacon_interval_window, &mesh_node);
    if (!slept_valid) {
        std::cerr << "[POWER FAIL] Light sleep should execute during valid idle window!" << std::endl;
        return 1;
    }
    if (power_mgr.get_total_sleep_cycles() != 1 || power_mgr.get_total_sleep_time_ms() != beacon_interval_window) {
        std::cerr << "[POWER FAIL] Sleep telemetry metrics mismatch!" << std::endl;
        return 1;
    }
    std::cout << "[POWER SUCCESS] Light-sleep duty cycling verified with seamless wake recovery!" << std::endl;

    std::cout << "\n>>> SYSTEM CORE & POWER MANAGER FULLY OPERATIONAL <<<" << std::endl;
    return 0;
}

