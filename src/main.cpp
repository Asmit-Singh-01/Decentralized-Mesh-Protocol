#include "packet_format.h"
#include "mesh_node.h"
#include "security_engine.h"
#include "swarm_orchestrator.h"
#include <iostream>
#include <vector>

int main() {
    std::cout << "Starting Swarm Mesh Node Simulation..." << std::endl;

    MeshNode node1(0x1001);
    MeshNode node2(0x1002);

    node1.init();
    node2.init();

    SecurityEngine sec_engine;
    uint8_t master_key[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                              0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    sec_engine.set_key(master_key, 16);

    MeshPacket tx_packet;
    tx_packet.header.version = 1;
    tx_packet.header.type = PacketType::BEACON;
    tx_packet.header.src_id = 0x1001;
    tx_packet.header.dest_id = 0x1002;
    tx_packet.header.next_hop_id = 0x1002;
    tx_packet.header.seq_num = 1;
    tx_packet.header.ttl = 5;
    tx_packet.header.payload_len = 5;

    const char* dummy_data = "HELLO";
    std::memcpy(tx_packet.payload, dummy_data, 5);

    uint8_t buffer[256];
    size_t out_len = 0;

    if (serialize_packet(tx_packet, buffer, out_len)) {
        std::cout << "Packet Serialized Successfully. Byte Count: " << out_len << std::endl;
    } else {
        std::cerr << "Packet Serialization Failed!" << std::endl;
        return 1;
    }

    MeshPacket rx_packet;
    if (deserialize_packet(buffer, out_len, rx_packet)) {
        std::cout << "Packet Deserialized Successfully. Sender ID: 0x" 
                  << std::hex << rx_packet.header.src_id << std::endl;
    } else {
        std::cerr << "Packet Deserialization Failed!" << std::endl;
        return 1;
    }

    SwarmOrchestrator orchestrator(0x1001);
    orchestrator.init();
    orchestrator.assign_task(101, 1);
    orchestrator.execute_orchestration_cycle(1000);

    sec_engine.clear_key();
    std::cout << "Simulation completed successfully!" << std::endl;
    return 0;
}
