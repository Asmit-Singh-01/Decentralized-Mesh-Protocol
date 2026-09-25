#pragma once

#include <cstdint>
#include <cstddef>

namespace NVSConfig
{
    bool begin();

    bool save_node_id(uint16_t node_id);
    bool load_node_id(uint16_t& node_id);

    bool save_node_name(const char* node_name);
    bool load_node_name(char* node_name, size_t max_length);

    void clear();
}
