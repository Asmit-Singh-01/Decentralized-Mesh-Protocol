#if defined(__has_include) && __has_include(<unity.h>)
#include <unity.h>
#else
#include <iostream>
#include <cassert>
#define UNITY_BEGIN() 0
#define UNITY_END() 0
#define RUN_TEST(fn) do { std::cout << "Running " #fn "... "; fn(); std::cout << "PASS\n"; } while(0)
#define TEST_ASSERT_TRUE(cond) assert(cond)
#define TEST_ASSERT_FALSE(cond) assert(!(cond))
#define TEST_ASSERT_EQUAL_UINT8(expected, actual) assert((expected) == (actual))
#define TEST_ASSERT_EQUAL_UINT16(expected, actual) assert((expected) == (actual))
#define TEST_ASSERT_EQUAL_INT(expected, actual) assert((expected) == (actual))
#define TEST_ASSERT_NOT_EQUAL(val1, val2) assert((val1) != (val2))
#endif

#include <cstring>
#include "packet_format.h"

void setUp(void) {}
void tearDown(void) {}

void test_serial_deserialize(void) {
    MeshPacket original_packet = {};
    original_packet.header.magic = PROTOCOL_MAGIC_BYTE;
    original_packet.header.sender_id = 17;
    original_packet.header.receiver_id = 25;
    original_packet.header.type = static_cast<uint8_t>(PacketType::HEARTBEAT);
    original_packet.header.ttl = 15;
    original_packet.header.payload_len = 25;
    original_packet.header.sequence_num = 115;

    for (int i = 0; i < 25; i++) {
        original_packet.payload[i] = static_cast<uint8_t>(i * 2);
    }

    uint8_t buffer[256] = {0};
    size_t out_len = 0;

    bool success = serialize_packet(original_packet, buffer, out_len);
    TEST_ASSERT_TRUE(success);

    MeshPacket reconstructed_packet = {};
    bool deserialize_success = deserialize_packet(buffer, out_len, reconstructed_packet);
    TEST_ASSERT_TRUE(deserialize_success);

    // Verify all header fields
    TEST_ASSERT_EQUAL_UINT8(original_packet.header.magic, reconstructed_packet.header.magic);
    TEST_ASSERT_EQUAL_UINT8(original_packet.header.type, reconstructed_packet.header.type);
    TEST_ASSERT_EQUAL_UINT16(original_packet.header.sender_id, reconstructed_packet.header.sender_id);
    TEST_ASSERT_EQUAL_UINT16(original_packet.header.receiver_id, reconstructed_packet.header.receiver_id);
    TEST_ASSERT_EQUAL_UINT16(original_packet.header.sequence_num, reconstructed_packet.header.sequence_num);
    TEST_ASSERT_EQUAL_UINT8(original_packet.header.ttl, reconstructed_packet.header.ttl);
    TEST_ASSERT_EQUAL_UINT8(original_packet.header.payload_len, reconstructed_packet.header.payload_len);

    // Verify payload integrity
    TEST_ASSERT_EQUAL_INT(0, std::memcmp(reconstructed_packet.payload, original_packet.payload, 25));

    // Verify CRC16 is generated and matched
    TEST_ASSERT_NOT_EQUAL(0, reconstructed_packet.crc16);
}

void test_corrupted_crc(void) {
    MeshPacket packet = {};
    packet.header.magic = PROTOCOL_MAGIC_BYTE;
    packet.header.sender_id = 1;
    packet.header.receiver_id = 2;
    packet.header.type = static_cast<uint8_t>(PacketType::BEACON);
    packet.header.ttl = 5;
    packet.header.payload_len = 4;
    packet.header.sequence_num = 1;
    std::memcpy(packet.payload, "TEST", 4);

    uint8_t buffer[256] = {0};
    size_t out_len = 0;
    TEST_ASSERT_TRUE(serialize_packet(packet, buffer, out_len));

    // Corrupt one byte of payload
    buffer[sizeof(PacketHeader) + 1] ^= 0xFF;

    MeshPacket out_packet = {};
    bool result = deserialize_packet(buffer, out_len, out_packet);
    TEST_ASSERT_FALSE(result);
}

void test_invalid_magic_byte(void) {
    MeshPacket packet = {};
    packet.header.magic = PROTOCOL_MAGIC_BYTE;
    packet.header.sender_id = 1;
    packet.header.receiver_id = 2;
    packet.header.type = static_cast<uint8_t>(PacketType::ACK);
    packet.header.ttl = 5;
    packet.header.payload_len = 2;
    packet.header.sequence_num = 2;

    uint8_t buffer[256] = {0};
    size_t out_len = 0;
    TEST_ASSERT_TRUE(serialize_packet(packet, buffer, out_len));

    // Corrupt magic byte
    buffer[0] = 0x00;

    MeshPacket out_packet = {};
    bool result = deserialize_packet(buffer, out_len, out_packet);
    TEST_ASSERT_FALSE(result);
}

void test_oversized_payload(void) {
    MeshPacket packet = {};
    packet.header.magic = PROTOCOL_MAGIC_BYTE;
    packet.header.payload_len = MAX_PAYLOAD_SIZE + 1; // Exceeds limit

    uint8_t buffer[256] = {0};
    size_t out_len = 0;
    bool result = serialize_packet(packet, buffer, out_len);
    TEST_ASSERT_FALSE(result);
}

void test_truncated_buffer(void) {
    MeshPacket packet = {};
    packet.header.magic = PROTOCOL_MAGIC_BYTE;
    packet.header.payload_len = 10;

    uint8_t buffer[256] = {0};
    size_t out_len = 0;
    TEST_ASSERT_TRUE(serialize_packet(packet, buffer, out_len));

    MeshPacket out_packet = {};
    // Pass truncated length
    bool result = deserialize_packet(buffer, sizeof(PacketHeader) + 2, out_packet);
    TEST_ASSERT_FALSE(result);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_serial_deserialize);
    RUN_TEST(test_corrupted_crc);
    RUN_TEST(test_invalid_magic_byte);
    RUN_TEST(test_oversized_payload);
    RUN_TEST(test_truncated_buffer);
    return UNITY_END();
}