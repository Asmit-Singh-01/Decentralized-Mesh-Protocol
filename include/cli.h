#pragma once

#include "mesh_node.h"
#include <string>

class SerialCLI {
private:
    MeshNode& node;
    uint32_t start_time_ms;

    void cmd_status(uint32_t current_time_ms);
    void cmd_routes();
    void cmd_ping(const std::string& arg);
    void cmd_help();

public:
    explicit SerialCLI(MeshNode& mesh_node, uint32_t start_ms = 0);

    // Parse and execute a CLI command string (e.g., mesh status, mesh routes, mesh ping [node_id])
    bool process_command(const std::string& input, uint32_t current_time_ms = 0);
};
