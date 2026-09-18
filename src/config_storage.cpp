#include "config_storage.h"
#include <iostream>
#include <cstring>
#include <fstream>

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
#include <Preferences.h>
static Preferences prefs;
#endif

// In-memory cache to prevent redundant flash wear
static NodeConfig s_cached_config = {0, 0, 0, 0};
static bool s_has_cached = false;

// Path to native host simulation NVS storage file
static const char* NVS_SIM_FILE = "mesh_nvs_sim.bin";

void init_default_config(NodeConfig& config) {
    config.magic = CONFIG_MAGIC_NUMBER;
    config.node_id = DEFAULT_NODE_ID;
    config.tx_power = DEFAULT_TX_POWER;
    config.default_channel = DEFAULT_CHANNEL;
}

void save_node_config(const NodeConfig& config) {
    // Flash wear-out prevention: Only write if configuration has changed
    if (s_has_cached && s_cached_config == config) {
        std::cout << "[NVS INFO] Configuration unchanged. Skipping flash write to prevent flash wear." << std::endl;
        return;
    }

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    prefs.begin(NVS_CONFIG_NAMESPACE, false);
    if (!s_has_cached || s_cached_config.magic != config.magic) {
        prefs.putUInt(NVS_KEY_MAGIC, config.magic);
    }
    if (!s_has_cached || s_cached_config.node_id != config.node_id) {
        prefs.putUShort(NVS_KEY_NODE_ID, config.node_id);
    }
    if (!s_has_cached || s_cached_config.tx_power != config.tx_power) {
        prefs.putChar(NVS_KEY_TX_POWER, config.tx_power);
    }
    if (!s_has_cached || s_cached_config.default_channel != config.default_channel) {
        prefs.putUChar(NVS_KEY_CHANNEL, config.default_channel);
    }
    prefs.end();
#else
    // Native PC host simulation using persistent binary storage
    std::ofstream ofs(NVS_SIM_FILE, std::ios::binary | std::ios::trunc);
    if (ofs.is_open()) {
        ofs.write(reinterpret_cast<const char*>(&config), sizeof(NodeConfig));
        ofs.close();
    }
#endif

    s_cached_config = config;
    s_has_cached = true;
    std::cout << "[NVS SUCCESS] Configuration saved to persistent storage [NodeID: 0x" 
              << std::hex << config.node_id << std::dec 
              << ", TxPower: " << static_cast<int>(config.tx_power) << " dBm"
              << ", Channel: " << static_cast<int>(config.default_channel) << "]." << std::endl;
}

bool load_node_config(NodeConfig& config) {
    bool valid = false;

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    prefs.begin(NVS_CONFIG_NAMESPACE, true);
    if (prefs.isKey(NVS_KEY_MAGIC)) {
        uint32_t magic = prefs.getUInt(NVS_KEY_MAGIC, 0);
        if (magic == CONFIG_MAGIC_NUMBER) {
            config.magic = magic;
            config.node_id = prefs.getUShort(NVS_KEY_NODE_ID, DEFAULT_NODE_ID);
            config.tx_power = prefs.getChar(NVS_KEY_TX_POWER, DEFAULT_TX_POWER);
            config.default_channel = prefs.getUChar(NVS_KEY_CHANNEL, DEFAULT_CHANNEL);
            valid = true;
        }
    }
    prefs.end();
#else
    std::ifstream ifs(NVS_SIM_FILE, std::ios::binary);
    if (ifs.is_open()) {
        NodeConfig temp;
        if (ifs.read(reinterpret_cast<char*>(&temp), sizeof(NodeConfig))) {
            if (temp.magic == CONFIG_MAGIC_NUMBER) {
                config = temp;
                valid = true;
            }
        }
        ifs.close();
    }
#endif

    if (valid) {
        s_cached_config = config;
        s_has_cached = true;
        std::cout << "[NVS SUCCESS] Loaded configuration from persistent storage [NodeID: 0x" 
                  << std::hex << config.node_id << std::dec 
                  << ", TxPower: " << static_cast<int>(config.tx_power) << " dBm"
                  << ", Channel: " << static_cast<int>(config.default_channel) << "]." << std::endl;
        return true;
    }

    // Fallback Logic: Unformatted or missing configuration -> write defaults and auto-save
    std::cout << "[NVS WARN] Flash unformatted or corrupt! Restoring and auto-saving default fallback configuration." << std::endl;
    init_default_config(config);
    save_node_config(config);
    return false;
}

void erase_node_config() {
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    prefs.begin(NVS_CONFIG_NAMESPACE, false);
    prefs.clear();
    prefs.end();
#else
    std::remove(NVS_SIM_FILE);
#endif
    s_has_cached = false;
    std::memset(&s_cached_config, 0, sizeof(NodeConfig));
    std::cout << "[NVS INFO] Persistent configuration erased." << std::endl;
}
