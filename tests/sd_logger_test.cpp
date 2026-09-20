#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

#include "sd_logger.h"

namespace {

const char* TEST_LOG_FILE = "mesh_logs.csv";

void remove_test_log() {
    std::remove(TEST_LOG_FILE);
}

std::string read_log_file() {
    std::ifstream file(TEST_LOG_FILE);

    assert(file.good());

    std::string content(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    return content;
}

void test_initialization_and_header() {
    remove_test_log();

    SDLogger logger;

    assert(logger.init());
    assert(logger.is_ready());
    assert(logger.pending_count() == 0);

    const std::string content = read_log_file();

    assert(
        content.find(
            "timestamp,source_id,dest_id,rssi,payload_len,crc_status"
        ) != std::string::npos
    );
}

void test_buffering() {
    remove_test_log();

    SDLogger logger;

    assert(logger.init());

    for (uint32_t i = 0; i < 9; ++i) {
        assert(
            logger.log_packet(
                1000 + i,
                0x1001,
                0x1002,
                -45,
                12,
                true
            )
        );
    }

    assert(logger.pending_count() == 9);

    /*
     * Fewer than 10 records must not trigger automatic flush.
     */
    logger.service();

    assert(logger.pending_count() == 9);
}

void test_flush_at_ten_records() {
    remove_test_log();

    SDLogger logger;

    assert(logger.init());

    for (uint32_t i = 0; i < 10; ++i) {
        assert(
            logger.log_packet(
                2000 + i,
                0x1001,
                0x1002,
                -40,
                20,
                true
            )
        );
    }

    assert(logger.pending_count() == 10);

    /*
     * service() performs the deferred flush after 10 records.
     */
    logger.service();

    assert(logger.pending_count() == 0);

    const std::string content = read_log_file();

    assert(
        content.find("2000,4097,4098,-40,20,OK") !=
        std::string::npos
    );

    assert(
        content.find("2009,4097,4098,-40,20,OK") !=
        std::string::npos
    );
}

void test_crc_status_format() {
    remove_test_log();

    SDLogger logger;

    assert(logger.init());

    assert(
        logger.log_packet(
            5000,
            0x1234,
            0x5678,
            -55,
            32,
            true
        )
    );

    assert(logger.flush());

    const std::string content = read_log_file();

    assert(
        content.find(
            "5000,4660,22136,-55,32,OK"
        ) != std::string::npos
    );
}

void test_buffer_capacity_is_bounded() {
    remove_test_log();

    SDLogger logger;

    assert(logger.init());

    for (int i = 0; i < 10; ++i) {
        assert(
            logger.log_packet(
                static_cast<uint32_t>(6000 + i),
                1,
                2,
                -30,
                4,
                true
            )
        );
    }

    /*
     * The 11th record is rejected rather than performing
     * blocking storage I/O inside log_packet().
     */
    assert(
        !logger.log_packet(
            6010,
            1,
            2,
            -30,
            4,
            true
        )
    );

    assert(logger.pending_count() == 10);

    assert(logger.flush());

    assert(logger.pending_count() == 0);
}

} // namespace

int main() {
    std::cout
        << "[TEST] SD logger initialization and header..."
        << std::endl;

    test_initialization_and_header();

    std::cout
        << "[PASS] initialization and header"
        << std::endl;

    std::cout
        << "[TEST] SD logger buffering..."
        << std::endl;

    test_buffering();

    std::cout
        << "[PASS] buffering"
        << std::endl;

    std::cout
        << "[TEST] SD logger flush at ten records..."
        << std::endl;

    test_flush_at_ten_records();

    std::cout
        << "[PASS] flush at ten records"
        << std::endl;

    std::cout
        << "[TEST] CRC status formatting..."
        << std::endl;

    test_crc_status_format();

    std::cout
        << "[PASS] CRC status formatting"
        << std::endl;

    std::cout
        << "[TEST] bounded buffer..."
        << std::endl;

    test_buffer_capacity_is_bounded();

    std::cout
        << "[PASS] bounded buffer"
        << std::endl;

    remove_test_log();

    std::cout
        << "\n[ALL SD LOGGER TESTS PASSED]"
        << std::endl;

    return 0;
}