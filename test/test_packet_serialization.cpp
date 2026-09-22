#include <unity.h>
#include <cstring>

#include "packet_format.h"

void test_packet_serialization_and_deserialization()
{
    // Create the original packet
    MeshPacket original = {};

    original.header.magic = PROTOCOL_MAGIC_BYTE;
    original.header.type =
        static_cast<uint8_t>(PacketType::HEARTBEAT);
    original.header.sender_id = 0x1001;
    original.header.receiver_id = 0x1002;
    original.header.sequence_num = 42;
    original.header.ttl = 5;

    const char* message = "HELLO_MESH";

    original.header.payload_len =
        static_cast<uint8_t>(std::strlen(message));

    std::memcpy(
        original.payload,
        message,
        original.header.payload_len
    );

    // Serialize the packet
    uint8_t buffer[128] = {0};
    size_t serialized_length = 0;

    bool serialize_result =
        serialize_packet(
            original,
            buffer,
            serialized_length
        );

    TEST_ASSERT_TRUE(serialize_result);
    TEST_ASSERT_GREATER_THAN(0, serialized_length);

    // Deserialize into a new packet
    MeshPacket restored = {};

    bool deserialize_result =
        deserialize_packet(
            buffer,
            serialized_length,
            restored
        );

    TEST_ASSERT_TRUE(deserialize_result);

    // Verify all header fields
    TEST_ASSERT_EQUAL(
        original.header.magic,
        restored.header.magic
    );

    TEST_ASSERT_EQUAL(
        original.header.type,
        restored.header.type
    );

    TEST_ASSERT_EQUAL(
        original.header.sender_id,
        restored.header.sender_id
    );

    TEST_ASSERT_EQUAL(
        original.header.receiver_id,
        restored.header.receiver_id
    );

    TEST_ASSERT_EQUAL(
        original.header.sequence_num,
        restored.header.sequence_num
    );

    TEST_ASSERT_EQUAL(
        original.header.ttl,
        restored.header.ttl
    );

    TEST_ASSERT_EQUAL(
        original.header.payload_len,
        restored.header.payload_len
    );

    // Verify payload
    TEST_ASSERT_EQUAL_UINT8_ARRAY(
        original.payload,
        restored.payload,
        original.header.payload_len
    );

 // Verify CRC
uint16_t expected_crc =
    calculate_crc16(
        buffer,
        serialized_length - sizeof(uint16_t)
    );

TEST_ASSERT_EQUAL(
    expected_crc,
    restored.crc16
);
}

void setUp()
{
}

void tearDown()
{
}

int main()
{
    UNITY_BEGIN();

    RUN_TEST(test_packet_serialization_and_deserialization);

    return UNITY_END();
}
