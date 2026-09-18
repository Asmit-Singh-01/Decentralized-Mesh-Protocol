#pragma once
#include <stdint.h>
#include <stddef.h>

#define NVS_CONFIG_NAMESPACE "mesh_cfg"
#define NVS_KEY_MAGIC        "magic"
#define NVS_KEY_NODE_ID      "node_id"
#define NVS_KEY_TX_POWER     "tx_power"
#define NVS_KEY_CHANNEL      "channel"

#define DEFAULT_NODE_ID      0x1001
#define DEFAULT_TX_POWER     20
#define DEFAULT_CHANNEL      1
#define CONFIG_MAGIC_NUMBER  0x4D455348U // "MESH"

struct NodeConfig {
    uint32_t magic;
    uint16_t node_id;
    int8_t tx_power;
    uint8_t default_channel;

    bool operator==(const NodeConfig& other) const {
        return magic == other.magic &&
               node_id == other.node_id &&
               tx_power == other.tx_power &&
               default_channel == other.default_channel;
    }

    bool operator!=(const NodeConfig& other) const {
        return !(*this == other);
    }
};

// Initialize default fallback configuration
void init_default_config(NodeConfig& config);

// NVS Persistent Storage functions (Issue #15)
void save_node_config(const NodeConfig& config);
bool load_node_config(NodeConfig& config);

// Helper for resetting/erasing NVS configuration (for testing fallback)
void erase_node_config();
