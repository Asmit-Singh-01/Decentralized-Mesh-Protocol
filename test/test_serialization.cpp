#include<iostream>
#include<cstring>
#include<cassert>
#include "packet_format.h"

void test_serial_deserialize(){
    // Header initialization
    MeshPacket original_packet = {};
    original_packet.header.magic = PROTOCOL_MAGIC_BYTE;
    original_packet.header.sender_id = 17;
    original_packet.header.receiver_id = 25;
    original_packet.header.type = static_cast<uint8_t>(PacketType::HEARTBEAT);
    original_packet.header.ttl = 15;
    original_packet.header.payload_len = 25;
    original_packet.header.sequence_num = 115;


    // Payload Initialization
    for(int i = 0; i < 25; i++){
        original_packet.payload[i] = i*2;
    }

    // Raw data buffer
    uint8_t buffer[256];
    size_t out_len = 0;

    // Serialization
    bool success = serialize_packet(original_packet, buffer, out_len);

    // Test for Serialization
    assert(success == true);

    // Deserialize
    MeshPacket reconstructed_packet ={};
    bool deserialize_sucess = deserialize_packet(buffer, out_len, reconstructed_packet);

    // Test for Deserialization
    assert(deserialize_sucess == true);

    // Test for Header Data
    assert(reconstructed_packet.header.magic == original_packet.header.magic);
    assert(reconstructed_packet.header.sender_id == original_packet.header.sender_id);
    assert(reconstructed_packet.header.payload_len == original_packet.header.payload_len);
    assert(reconstructed_packet.header.receiver_id == original_packet.header.receiver_id);
    assert(reconstructed_packet.header.ttl == original_packet.header.ttl);
    assert(reconstructed_packet.header.type == original_packet.header.type);
    assert(reconstructed_packet.header.sequence_num == original_packet.header.sequence_num);
    
    // Test for Payload
    assert(memcmp(reconstructed_packet.payload, original_packet.payload, 25)==0);

}

int main(){
    test_serial_deserialize();
    std::cout << "All test passed successfully!" << std::endl;
    return 0;
}