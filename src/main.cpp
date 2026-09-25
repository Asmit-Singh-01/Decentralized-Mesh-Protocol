#include <iostream>
#include <cstring>
#include <iomanip>

#include "swarm_orchestrator.h"
#include "security_engine.h"
#include "packet_format.h"
#include "mesh_node.h"
#include "battery_monitor.h"
#include "nvs_config.h"

void print_hex(const char* label, const uint8_t* data, size_t len)
{
    std::cout << label << ": ";

    for (size_t i = 0; i < len; ++i)
    {
        std::cout
            << std::hex
            << std::setw(2)
            << std::setfill('0')
            << static_cast<int>(data[i])
            << " ";
    }

    std::cout << std::dec << std::endl;
}

int main()
{
    std::cout
        << "=================================================="
        << std::endl;

    std::cout
        << ">>> DECENTRALIZED SWARM OS / MESH CORE INITIALIZED <<<"
        << std::endl;

    std::cout
        << "=================================================="
        << std::endl;

    // 1. NVS Configuration Initialization
    NVSConfig::begin();

    uint16_t stored_node_id = 0x1001;

    if (NVSConfig::load_node_id(stored_node_id))
    {
        std::cout
            << "[NVS] Loaded node ID from flash: 0x"
            << std::hex
            << stored_node_id
            << std::dec
            << std::endl;
    }
    else
    {
        NVSConfig::save_node_id(stored_node_id);

        std::cout
            << "[NVS] Saved default node ID to flash."
            << std::endl;
    }

    char node_name[32] = "MESH_NODE_1001";

    if (!NVSConfig::load_node_name(
            node_name,
            sizeof(node_name)))
    {
        NVSConfig::save_node_name(node_name);

        std::cout
            << "[NVS] Saved default node name: "
            << node_name
            << std::endl;
    }
    else
    {
        std::cout
            << "[NVS] Loaded node name: "
            << node_name
            << std::endl;
    }

    // 2. Mesh Node Initialization
    MeshNode local_node(stored_node_id);
    local_node.init();

    BatteryMonitor::check_battery(local_node);

    std::cout
        << "[SYSTEM] Node 0x"
        << std::hex
        << stored_node_id
        << std::dec
        << " initialized successfully."
        << std::endl;

    // 3. Swarm Orchestrator Initialization
    SwarmOrchestrator orchestrator(stored_node_id);
    orchestrator.init();

    std::cout
        << "[SYSTEM] Assigning autonomous swarm mission tasks..."
        << std::endl;

    orchestrator.assign_task(101, 1);
    orchestrator.assign_task(102, 5);

    std::cout
        << "[SYSTEM] Running initial orchestration cycle..."
        << std::endl;

    orchestrator.execute_orchestration_cycle(1000);

    // 4. Packet Serialization and CRC Check
    std::cout
        << "\n[NETWORK] Testing Packet Serialization & CRC16..."
        << std::endl;

    MeshPacket tx_packet = {};

    tx_packet.header.magic = PROTOCOL_MAGIC_BYTE;
    tx_packet.header.type = 0x01;
    tx_packet.header.sender_id = stored_node_id;
    tx_packet.header.receiver_id = 0x1002;
    tx_packet.header.sequence_num = 1;
    tx_packet.header.ttl = 5;

    const char* dummy_data = "PING_PAYLOAD";

    tx_packet.header.payload_len =
        std::strlen(dummy_data);

    std::memcpy(
        tx_packet.payload,
        dummy_data,
        tx_packet.header.payload_len
    );

    uint8_t buffer[256] = {0};
    size_t serialized_len = 0;

    if (serialize_packet(
            tx_packet,
            buffer,
            serialized_len))
    {
        std::cout
            << "[SUCCESS] Packet Serialized! Size: "
            << serialized_len
            << " bytes"
            << std::endl;

        print_hex(
            "Raw Packet Buffer",
            buffer,
            serialized_len
        );
    }
    else
    {
        std::cerr
            << "[ERROR] Packet Serialization Failed!"
            << std::endl;

        return 1;
    }

    // 5. AES-128 Security Pipeline Verification
    std::cout
        << "\n[SECURITY] Initializing AES-128 Encryption Engine..."
        << std::endl;

    SecurityEngine sec;

    uint8_t secret_key[16] =
    {
        0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C,
        0x0D, 0x0E, 0x0F, 0x10
    };

    uint8_t nonce[16] =
    {
        0xAA, 0xBB, 0xCC, 0xDD,
        0xEE, 0xFF, 0x00, 0x11,
        0x22, 0x33, 0x44, 0x55,
        0x66, 0x77, 0x88, 0x99
    };

    if (!sec.set_key(secret_key, 16))
    {
        std::cerr
            << "[ERROR] Key setup failed!"
            << std::endl;

        return 1;
    }

    const char* raw_payload =
        "SWARM_COMMAND_EXECUTE_TAKEOFF";

    size_t len =
        std::strlen(raw_payload);

    uint8_t encrypted[64] = {0};
    uint8_t decrypted[64] = {0};

    sec.encrypt(
        reinterpret_cast<const uint8_t*>(raw_payload),
        len,
        encrypted,
        nonce
    );

    sec.decrypt(
        encrypted,
        len,
        decrypted,
        nonce
    );

    return 0;
}
