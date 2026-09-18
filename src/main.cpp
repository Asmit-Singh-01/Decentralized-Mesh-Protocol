#include <iostream>
#include <cstring>
#include "swarm_orchestrator.h"
#include "security_engine.h"
#include "battery_monitor.h"
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

    // 3. Issue #16: Battery Monitoring & Low-Battery Emergency Alert Verification
    std::cout << "\n[BATTERY] Initializing Battery Monitoring Task (Issue #16)..." << std::endl;
    MeshNode mesh_node(0x1001);
    mesh_node.init();
    BatteryMonitor batt_mon(DEFAULT_BATTERY_PIN, 5);

    // Test nominal voltage (4.1V) - should NOT trigger alert
    batt_mon.set_simulated_voltage(4.1f);
    for (int i = 0; i < 5; ++i) {
        batt_mon.sample_voltage();
    }
    std::cout << "[BATTERY] Nominal smoothed voltage: " << batt_mon.get_smoothed_voltage() << "V (Low power: " << (batt_mon.is_low_power() ? "YES" : "NO") << ")" << std::endl;
    if (batt_mon.is_low_power()) {
        std::cerr << "[BATTERY FAIL] Nominal voltage falsely triggered low-power mode!" << std::endl;
        return 1;
    }

    // Test low voltage drop (3.25V) - triggers smoothed alert
    std::cout << "[BATTERY] Simulating voltage drop to 3.25V..." << std::endl;
    batt_mon.set_simulated_voltage(3.25f);
    for (int i = 0; i < 5; ++i) {
        batt_mon.sample_voltage();
    }

    if (!batt_mon.is_low_power()) {
        std::cerr << "[BATTERY FAIL] Low battery was not detected!" << std::endl;
        return 1;
    }

    bool alerted = batt_mon.check_and_alert(mesh_node);
    if (!alerted) {
        std::cerr << "[BATTERY FAIL] Low battery warning packet was not broadcasted!" << std::endl;
        return 1;
    }
    std::cout << "[BATTERY SUCCESS] Low battery emergency broadcast successfully triggered at " << batt_mon.get_smoothed_voltage() << "V!" << std::endl;

    std::cout << "\n>>> SYSTEM CORE & BATTERY MONITOR FULLY OPERATIONAL <<<" << std::endl;
    return 0;
}

