#include "sd_logger.h"

#ifdef ARDUINO

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

namespace {

constexpr char LOG_FILE[] = "/mesh_logs.csv";
constexpr uint8_t FLUSH_INTERVAL = 10;

bool sd_ready = false;
uint8_t write_count = 0;

void create_log_file_if_needed() {
    if (!SD.exists(LOG_FILE)) {
        File file = SD.open(LOG_FILE, FILE_WRITE);

        if (!file) {
            return;
        }

        file.println(
            "timestamp,source_id,dest_id,rssi,payload_len,crc_status"
        );

        file.close();
        return;
    }

    File file = SD.open(LOG_FILE, FILE_APPEND);

    if (!file) {
        return;
    }

    if (file.size() == 0) {
        file.println(
            "timestamp,source_id,dest_id,rssi,payload_len,crc_status"
        );
    }

    file.close();
}

} // namespace


bool sd_logger_init(uint8_t cs_pin) {

    sd_ready = false;
    write_count = 0;

    if (!SD.begin(cs_pin)) {
        Serial.println("SD card initialization failed.");
        return false;
    }

    if (!SD.cardType()) {
        Serial.println("No SD card detected.");
        return false;
    }

    create_log_file_if_needed();

    sd_ready = true;

    Serial.println("SD card logger initialized.");

    return true;
}


void sd_logger_log_packet(
    uint32_t timestamp,
    uint32_t source_id,
    uint32_t dest_id,
    int8_t rssi,
    size_t payload_len,
    bool crc_status
) {

    if (!sd_ready) {
        return;
    }

    File file = SD.open(LOG_FILE, FILE_APPEND);

    if (!file) {
        Serial.println("Unable to open mesh log file.");
        sd_ready = false;
        return;
    }

    file.print(timestamp);
    file.print(",");

    file.print(source_id);
    file.print(",");

    file.print(dest_id);
    file.print(",");

    file.print(rssi);
    file.print(",");

    file.print(payload_len);
    file.print(",");

    file.println(crc_status ? "OK" : "FAIL");

    file.close();

    write_count++;

    if (write_count >= FLUSH_INTERVAL) {
        sd_logger_flush();
    }
}


void sd_logger_flush() {

    if (!sd_ready) {
        return;
    }

    File file = SD.open(LOG_FILE, FILE_APPEND);

    if (file) {
        file.flush();
        file.close();
    }

    write_count = 0;
}

#else

// Host/simulator stubs.
// The actual SD-card implementation is compiled for Arduino/ESP32.

bool sd_logger_init(uint8_t) {
    return false;
}


void sd_logger_log_packet(
    uint32_t,
    uint32_t,
    uint32_t,
    int8_t,
    size_t,
    bool
) {
}


void sd_logger_flush() {
}

#endif
