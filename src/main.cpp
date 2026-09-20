#include <iostream>
#include <cstring>
#include <iomanip>

#include "swarm_orchestrator.h"
#include "security_engine.h"
#include "packet_format.h"
#include "mesh_node.h"

void print_hex(const char* label, const uint8_t* data, size_t len) {
    std::cout << label << ": ";
    for (size_t i = 0; i < len; ++i) {
        std::cout << std::hex << std::setw(2)
                  << std::setfill('0')
                  << static_cast<int>(data[i]) << " ";
    }
    std::cout << std::dec << std::endl;
}

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << ">>> DECENTRALIZED SWARM OS / MESH CORE INITIALIZED <<<"
              << std::endl;
    std::cout << "==================================================" << std::endl;

    MeshNode local_node(0x1001);
    local_node.init();

    std::cout << "[SYSTEM] Node 0x1001 initialized successfully."
              << std::endl;

    SwarmOrchestrator orchestrator(0x1001);
    orchestrator.init();

    std::cout << "[SYSTEM] Assigning autonomous swarm mission tasks..."
              << std::endl;

    orchestrator.assign_task(101, 1);
    orchestrator.assign_task(102, 5);

    std::cout << "[SYSTEM] Running initial orchestration cycle..."
              << std::endl;

    orchestrator.execute_orchestration_cycle(1000);

    std::cout << "\n[NETWORK] Testing Packet Serialization & CRC16..."
              << std::endl;

    MeshPacket tx_packet{};

    tx_packet.header.magic = PROTOCOL_MAGIC_BYTE;
    tx_packet.header.type = 0x01;
    tx_packet.header.sender_id = 0x1001;
    tx_packet.header.receiver_id = 0x1002;
    tx_packet.header.sequence_num = 1;
    tx_packet.header.ttl = 5;

    const char* dummy_data = "PING_PAYLOAD";
    tx_packet.header.payload_len =
        static_cast<uint8_t>(std::strlen(dummy_data));

    std::memcpy(
        tx_packet.payload,
        dummy_data,
        tx_packet.header.payload_len
    );

    uint8_t buffer[256] = {0};
    size_t serialized_len = 0;

    if (serialize_packet(tx_packet, buffer, serialized_len)) {
        std::cout << "[SUCCESS] Packet Serialized! Size: "
                  << serialized_len << " bytes" << std::endl;

        print_hex("Raw Packet Buffer", buffer, serialized_len);
    } else {
        std::cerr << "[ERROR] Packet Serialization Failed!"
                  << std::endl;
        return 1;
    }

    std::cout << "\n[SECURITY] Initializing AES-128 Encryption Engine..."
              << std::endl;

    SecurityEngine sec;

    uint8_t secret_key[16] = {
        0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C,
        0x0D, 0x0E, 0x0F, 0x10
    };

    uint8_t nonce[16] = {
        0xAA, 0xBB, 0xCC, 0xDD,
        0xEE, 0xFF, 0x00, 0x11,
        0x22, 0x33, 0x44, 0x55,
        0x66, 0x77, 0x88, 0x99
    };

    if (!sec.set_key(secret_key, 16)) {
        std::cerr << "[ERROR] Key setup failed!" << std::endl;
        return 1;
    }

    const char* raw_payload = "SWARM_COMMAND_EXECUTE_TAKEOFF";
    size_t len = std::strlen(raw_payload);

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

    if (std::memcmp(raw_payload, decrypted, len) == 0) {
        std::cout
            << "[SECURITY SUCCESS] Encrypted packet decrypted "
               "with 100% integrity!"
            << std::endl;
    } else {
        std::cerr
            << "[SECURITY FAIL] Decrypted payload mismatch!"
            << std::endl;
        return 1;
    }

    sec.clear_key();

    std::cout
        << "\n[HEARTBEAT] Starting self-healing peer monitoring simulation..."
        << std::endl;

    MeshNode heartbeat_node(0x1001);
    heartbeat_node.init();

    heartbeat_node.service(0);

    bool heartbeat_2000 = heartbeat_node.service(2000);
    bool heartbeat_4000 = heartbeat_node.service(4000);

    std::cout << "[HEARTBEAT] t=2000ms -> "
              << (heartbeat_2000 ? "HEARTBEAT EMITTED" : "NO HEARTBEAT")
              << std::endl;

    std::cout << "[HEARTBEAT] t=4000ms -> "
              << (heartbeat_4000 ? "HEARTBEAT EMITTED" : "NO HEARTBEAT")
              << std::endl;

    MeshPacket peer_packet{};

    peer_packet.header.magic = PROTOCOL_MAGIC_BYTE;
    peer_packet.header.type =
        static_cast<uint8_t>(PacketType::HEARTBEAT);
    peer_packet.header.sender_id = 0x1002;
    peer_packet.header.receiver_id = 0xFFFF;
    peer_packet.header.sequence_num = 1;
    peer_packet.header.ttl = 5;
    peer_packet.header.payload_len = 0;

    uint8_t peer_buffer[256] = {0};
    size_t peer_serialized_len = 0;

    if (!serialize_packet(
            peer_packet,
            peer_buffer,
            peer_serialized_len)) {
        std::cerr
            << "[HEARTBEAT FAIL] Could not serialize simulated peer heartbeat."
            << std::endl;
        return 1;
    }

    heartbeat_node.handle_received_packet(
        peer_buffer,
        peer_serialized_len,
        -45,
        4500
    );

    std::cout << "[HEARTBEAT] Peer 0x1002 detected at t=4500ms."
              << std::endl;

    heartbeat_node.service(9000);

    std::cout << "[HEARTBEAT] t=9000ms -> Active peers: "
              << heartbeat_node.get_routing_table().size()
              << std::endl;

    heartbeat_node.service(11001);

    std::cout << "[HEARTBEAT] t=11001ms -> Active peers: "
              << heartbeat_node.get_routing_table().size()
              << std::endl;

    if (heartbeat_node.get_routing_table().empty()) {
        std::cout
            << "[HEARTBEAT SUCCESS] Stale peer automatically removed."
            << std::endl;
    } else {
        std::cerr
            << "[HEARTBEAT FAIL] Stale peer was not removed!"
            << std::endl;
        return 1;
    }

    std::cout
        << "\n==================================================" << std::endl;
    std::cout
        << ">>> SYSTEM CORE, SECURITY & SELF-HEALING MESH OPERATIONAL <<<"
        << std::endl;
    std::cout
        << "==================================================" << std::endl;

    return 0;
}