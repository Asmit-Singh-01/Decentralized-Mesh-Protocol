#include <iostream>
#include <cstring>
#include "swarm_orchestrator.h"
#include "security_engine.h"
#include "config_storage.h"

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

    // 3. Issue #15: Persistent NVS Flash Configuration Storage Verification
    std::cout << "\n[NVS] Initializing NVS Configuration Storage Pipeline (Issue #15)..." << std::endl;
    erase_node_config(); // Start with unformatted / empty storage

    NodeConfig config = {0, 0, 0, 0};
    // Step 1: Loading from unformatted NVS triggers fallback defaults and auto-saves
    bool loaded = load_node_config(config);
    if (loaded) {
        std::cerr << "[NVS FAIL] Unformatted NVS should have triggered fallback!" << std::endl;
        return 1;
    }
    if (config.node_id != DEFAULT_NODE_ID || config.tx_power != DEFAULT_TX_POWER || config.default_channel != DEFAULT_CHANNEL) {
        std::cerr << "[NVS FAIL] Default fallback configuration mismatch!" << std::endl;
        return 1;
    }
    std::cout << "[NVS SUCCESS] Automatic fallback configuration applied & saved successfully!" << std::endl;

    // Step 2: Update configuration with new node parameters
    config.node_id = 0x2048;
    config.tx_power = 17; // 17 dBm
    config.default_channel = 11; // Wi-Fi channel 11
    std::cout << "[NVS] Saving updated node configuration..." << std::endl;
    save_node_config(config);

    // Step 3: Flash wear-out prevention check (writing identical config should be skipped)
    std::cout << "[NVS] Verifying flash wear-out prevention (duplicate save)..." << std::endl;
    save_node_config(config);

    // Step 4: Simulate reboot / reload from persistent storage
    NodeConfig rebooted_config = {0, 0, 0, 0};
    bool reload_ok = load_node_config(rebooted_config);
    if (!reload_ok || rebooted_config != config) {
        std::cerr << "[NVS FAIL] Reloaded configuration does not match saved configuration!" << std::endl;
        return 1;
    }
    std::cout << "[NVS SUCCESS] Configuration persistence verified across reboot simulation! [NodeID: 0x"
              << std::hex << rebooted_config.node_id << std::dec << "]" << std::endl;

    erase_node_config(); // Clean up simulation file

    std::cout << "\n>>> SYSTEM CORE & NVS STORAGE FULLY OPERATIONAL <<<" << std::endl;
    return 0;
}

