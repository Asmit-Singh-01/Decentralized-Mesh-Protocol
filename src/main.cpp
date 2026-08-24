#include <iostream>
#include <cstring>
#include "mesh_node.h"
#include "packet_format.h"

int main() {
    std::cout << "=== SWARM MESH PROTOCOL INITIALIZED ===" << std::endl;

    MeshNode node1(0x0001);
    node1.init();

    const char* telemetry = "TEMP:24C;POS:X10,Y20,Z0";
    uint8_t payload_len = static_cast<uint8_t>(strlen(telemetry));

    std::cout << "[SIM] Broadcasting telemetry payload..." << std::endl;
    bool broadcast_status = node1.broadcast_payload(
        PacketType::TELEMETRY_SWARM, 
        reinterpret_cast<const uint8_t*>(telemetry), 
        payload_len
    );

    if (broadcast_status) {
        std::cout << "[STATUS] Beacon queued for transmission." << std::endl;
    }

    // Simulate incoming packet from neighbor node 0x0002
    MeshPacket incoming_pkt;
    incoming_pkt.header.magic = PROTOCOL_MAGIC_BYTE;
    incoming_pkt.header.type = static_cast<uint8_t>(PacketType::BEACON);
    incoming_pkt.header.sender_id = 0x0002;
    incoming_pkt.header.receiver_id = 0xFFFF;
    incoming_pkt.header.sequence_num = 101;
    incoming_pkt.header.ttl = 4;
    incoming_pkt.header.payload_len = 0;

    uint8_t buffer[128];
    size_t serialized_len = 0;
    if (serialize_packet(incoming_pkt, buffer, serialized_len)) {
        std::cout << "[SIM] Ingesting packet from node 0x0002 (RSSI: -65 dBm)..." << std::endl;
        node1.handle_received_packet(buffer, serialized_len, -65);
    }

    std::cout << "[STATUS] Active Routing Table size: " << node1.get_routing_table().size() << std::endl;
    std::cout << "=== MESH TEST RUN COMPLETE ===" << std::endl;

    return 0;
}
