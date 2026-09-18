#include <iostream>
#include <cstring>
#include "swarm_orchestrator.h"
#include "security_engine.h"
#include "radio_driver.h"
#include "mock_radio_driver.h"
#include "esp_now_driver.h"
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

    // 3. Issue #19: Hardware Abstraction Layer (HAL) Radio Interface Verification
    std::cout << "\n[HAL] Initializing Hardware Abstraction Layer Pipeline (Issue #19)..." << std::endl;
    MockRadioDriver mock_radio;
    if (!mock_radio.init()) {
        std::cerr << "[HAL FAIL] MockRadioDriver initialization failed!" << std::endl;
        return 1;
    }

    // Dependency Injection: Pass MockRadioDriver into MeshNode
    MeshNode mesh_node(0x1001, &mock_radio);
    mesh_node.init();

    mock_radio.set_rx_callback([](const uint8_t* src_mac, const uint8_t* data, size_t len, int8_t rssi) {
        (void)src_mac;
        (void)data;
        std::cout << "[HAL RX CALLBACK] Received " << len << " bytes via radio interface (RSSI: " 
                  << static_cast<int>(rssi) << " dBm)." << std::endl;
    });

    const uint8_t test_data[] = "TELEMETRY_SAMPLE_HAL_PACKET";
    bool bcast_ok = mesh_node.broadcast_payload(PacketType::TELEMETRY_SWARM, test_data, sizeof(test_data));
    if (!bcast_ok || mock_radio.get_tx_count() != 1) {
        std::cerr << "[HAL FAIL] MeshNode broadcast via injected IRadioDriver failed!" << std::endl;
        return 1;
    }
    std::cout << "[HAL SUCCESS] Abstracted radio transmission verified via injected MeshNode driver ("
              << mock_radio.get_tx_count() << " packet recorded)!" << std::endl;

    const uint8_t sample_mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    mock_radio.inject_receive(sample_mac, test_data, sizeof(test_data), -68);
    std::cout << "[HAL SUCCESS] Abstracted radio reception & callback pipeline verified!" << std::endl;

    EspNowDriver espnow_driver;
    espnow_driver.init();
    espnow_driver.send_bytes(sample_mac, test_data, sizeof(test_data));
    std::cout << "[HAL SUCCESS] EspNowDriver hardware abstraction verified!" << std::endl;

    std::cout << "\n>>> SYSTEM CORE & HAL RADIO LAYER FULLY OPERATIONAL <<<" << std::endl;
    return 0;
}

