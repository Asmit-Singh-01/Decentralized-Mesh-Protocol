#include "sd_logger.h"

#include <cstdio>
#include <cstring>

#ifdef ARDUINO

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

#else

#include <fstream>

#endif

SDLogger::SDLogger(uint8_t cs_pin_value)
    : cs_pin(cs_pin_value),
      initialized(false),
      pending_records(0),
      record_buffer{} {}

bool SDLogger::init() {
    pending_records = 0;
    initialized = false;

#ifdef ARDUINO

    /*
     * Use the default ESP32 SPI bus.
     * The CS pin is configurable through the constructor.
     */
    SPI.begin();

    if (!SD.begin(cs_pin)) {
        return false;
    }

    if (!ensure_log_file()) {
        return false;
    }

    initialized = true;
    return true;

#else

    /*
     * Host simulation uses a local CSV file so the logger
     * can be tested without physical SD hardware.
     */
    if (!ensure_log_file()) {
        return false;
    }

    initialized = true;
    return true;

#endif
}

bool SDLogger::ensure_log_file() {

#ifdef ARDUINO

    if (!SD.exists(LOG_PATH)) {
        File file = SD.open(LOG_PATH, FILE_WRITE);

        if (!file) {
            return false;
        }

        file.println(
            "timestamp,source_id,dest_id,rssi,payload_len,crc_status"
        );

        file.close();
        return true;
    }

    File file = SD.open(LOG_PATH, FILE_READ);

    if (!file) {
        return false;
    }

    bool empty = file.size() == 0;
    file.close();

    if (empty) {
        file = SD.open(LOG_PATH, FILE_APPEND);

        if (!file) {
            return false;
        }

        file.println(
            "timestamp,source_id,dest_id,rssi,payload_len,crc_status"
        );

        file.close();
    }

    return true;

#else

    std::ifstream input("mesh_logs.csv", std::ios::binary);

    if (!input.good()) {
        std::ofstream output("mesh_logs.csv", std::ios::out);

        if (!output) {
            return false;
        }

        output
            << "timestamp,source_id,dest_id,rssi,payload_len,crc_status\n";

        return true;
    }

    input.seekg(0, std::ios::end);

    const std::streamoff file_size = input.tellg();

    input.close();

    if (file_size == 0) {
        std::ofstream output("mesh_logs.csv", std::ios::out);

        if (!output) {
            return false;
        }

        output
            << "timestamp,source_id,dest_id,rssi,payload_len,crc_status\n";
    }

    return true;

#endif
}

bool SDLogger::log_packet(
    uint32_t timestamp,
    uint16_t source_id,
    uint16_t dest_id,
    int8_t rssi,
    uint8_t payload_len,
    bool crc_valid
) {
    if (!initialized) {
        return false;
    }

    /*
     * Packet processing must never perform SD/file I/O.
     * Only format the record into the RAM buffer here.
     */
    if (pending_records >= MAX_RECORDS) {
        return false;
    }

    const int written = std::snprintf(
        record_buffer[pending_records],
        MAX_RECORD_SIZE,
        "%lu,%u,%u,%d,%u,%s\n",
        static_cast<unsigned long>(timestamp),
        static_cast<unsigned int>(source_id),
        static_cast<unsigned int>(dest_id),
        static_cast<int>(rssi),
        static_cast<unsigned int>(payload_len),
        crc_valid ? "OK" : "INVALID"
    );

    if (written <= 0 ||
        static_cast<size_t>(written) >= MAX_RECORD_SIZE) {
        return false;
    }

    ++pending_records;

    return true;
}

void SDLogger::service() {
    if (!initialized || pending_records == 0) {
        return;
    }

    if (pending_records >= FLUSH_THRESHOLD) {
        flush();
    }
}

bool SDLogger::flush() {
    if (!initialized || pending_records == 0) {
        return true;
    }

    if (!append_buffer_to_storage()) {
        return false;
    }

    pending_records = 0;

    return true;
}

bool SDLogger::append_buffer_to_storage() {

#ifdef ARDUINO

    File file = SD.open(LOG_PATH, FILE_APPEND);

    if (!file) {
        return false;
    }

    for (uint8_t i = 0; i < pending_records; ++i) {
        const size_t length =
            std::strlen(record_buffer[i]);

        if (file.write(
                reinterpret_cast<const uint8_t*>(record_buffer[i]),
                length
            ) != length) {

            file.close();
            return false;
        }
    }

    file.flush();
    file.close();

    return true;

#else

    std::ofstream output(
        "mesh_logs.csv",
        std::ios::out | std::ios::app
    );

    if (!output) {
        return false;
    }

    for (uint8_t i = 0; i < pending_records; ++i) {
        output << record_buffer[i];
    }

    output.flush();

    return output.good();

#endif
}

bool SDLogger::is_ready() const {
    return initialized;
}

uint8_t SDLogger::pending_count() const {
    return pending_records;
}