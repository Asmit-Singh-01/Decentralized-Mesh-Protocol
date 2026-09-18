#include "sd_logger.h"
#include <iostream>
#include <fstream>
#include <sstream>

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
#include <SPI.h>
#include <SD.h>
static File s_log_file;
#else
static std::ofstream s_sim_file;
#endif

static const char* CSV_HEADER = "timestamp,source_id,dest_id,rssi,payload_len,crc_status";

SDCardLogger::SDCardLogger(uint8_t pin, const char* filename, size_t flush_thresh)
    : cs_pin(pin),
      log_filename(filename ? filename : DEFAULT_LOG_FILENAME),
      card_available(false),
      unflushed_writes(0),
      total_writes(0),
      flush_interval(flush_thresh > 0 ? flush_thresh : DEFAULT_FLUSH_INTERVAL) {}

SDCardLogger::~SDCardLogger() {
    close();
}

bool SDCardLogger::init() {
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    if (!SD.begin(cs_pin)) {
        std::cout << "[SD WARN] MicroSD card not detected on CS pin " 
                  << static_cast<int>(cs_pin) << "! Logging disabled gracefully." << std::endl;
        card_available = false;
        return false;
    }

    card_available = true;
    bool needs_header = !SD.exists(log_filename.c_str());
    s_log_file = SD.open(log_filename.c_str(), FILE_APPEND);
    if (s_log_file && needs_header) {
        s_log_file.println(CSV_HEADER);
        s_log_file.flush();
    }
#else
    // Native PC simulation: check if file exists, write header if missing
    std::ifstream check_file(log_filename);
    bool needs_header = !check_file.good();
    check_file.close();

    s_sim_file.open(log_filename, std::ios::out | std::ios::app);
    if (!s_sim_file.is_open()) {
        std::cout << "[SD WARN] Failed to open log file " << log_filename << "! Logging disabled gracefully." << std::endl;
        card_available = false;
        return false;
    }

    card_available = true;
    if (needs_header) {
        s_sim_file << CSV_HEADER << "\n";
        s_sim_file.flush();
    }
#endif

    std::cout << "[SD SUCCESS] MicroSD card logger initialized successfully. Log path: " 
              << log_filename << std::endl;
    return true;
}

bool SDCardLogger::is_card_available() const {
    return card_available;
}

bool SDCardLogger::log_packet(uint32_t timestamp_ms, uint16_t source_id, uint16_t dest_id,
                             int8_t rssi, uint8_t payload_len, bool crc_valid) {
    if (!card_available) {
        return false;
    }

    std::stringstream ss;
    ss << timestamp_ms << ","
       << "0x" << std::hex << source_id << std::dec << ","
       << "0x" << std::hex << dest_id << std::dec << ","
       << static_cast<int>(rssi) << ","
       << static_cast<int>(payload_len) << ","
       << (crc_valid ? "VALID" : "CORRUPT");

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    if (!s_log_file) {
        s_log_file = SD.open(log_filename.c_str(), FILE_APPEND);
    }
    if (s_log_file) {
        s_log_file.println(ss.str().c_str());
    }
#else
    if (s_sim_file.is_open()) {
        s_sim_file << ss.str() << "\n";
    }
#endif

    ++unflushed_writes;
    ++total_writes;

    // Periodic flush to prevent SD card wear and protect data against unexpected power failure
    if (unflushed_writes >= flush_interval) {
        flush();
    }

    return true;
}

bool SDCardLogger::log_packet(uint32_t timestamp_ms, const MeshPacket& packet, int8_t rssi, bool crc_valid) {
    return log_packet(timestamp_ms, 
                      packet.header.sender_id, 
                      packet.header.receiver_id, 
                      rssi, 
                      packet.header.payload_len, 
                      crc_valid);
}

void SDCardLogger::flush() {
    if (!card_available || unflushed_writes == 0) return;

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    if (s_log_file) {
        s_log_file.flush();
    }
#else
    if (s_sim_file.is_open()) {
        s_sim_file.flush();
    }
#endif

    std::cout << "[SD INFO] Buffer flushed to storage (" << unflushed_writes 
              << " writes synced). Total logs: " << total_writes << std::endl;
    unflushed_writes = 0;
}

void SDCardLogger::close() {
    flush();
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    if (s_log_file) {
        s_log_file.close();
    }
#else
    if (s_sim_file.is_open()) {
        s_sim_file.close();
    }
#endif
    card_available = false;
}

size_t SDCardLogger::get_unflushed_count() const {
    return unflushed_writes;
}

size_t SDCardLogger::get_total_writes() const {
    return total_writes;
}

const std::string& SDCardLogger::get_filename() const {
    return log_filename;
}
