#pragma once

#include "mesh_node.h"
#include <cstdint>

namespace TelemetryLogger
{
    void log_node_health(
        const MeshNode& mesh_node,
        uint32_t current_time_ms
    );
}
