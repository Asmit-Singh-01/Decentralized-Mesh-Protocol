#include "mesh_node.h"

#include <cstdint>
#include <iostream>

#ifdef ARDUINO
#include <Arduino.h>
#endif

namespace TelemetryLogger
{
    void log_node_health(
        const MeshNode& mesh_node,
        uint32_t current_time_ms)
    {
        const auto& routing_table =
            mesh_node.get_routing_table();

#ifdef ARDUINO

        Serial.print("[TELEMETRY] time_ms=");
        Serial.print(current_time_ms);

        Serial.print(" active_peers=");
        Serial.println(routing_table.size());

#else

        std::cout
            << "[TELEMETRY] time_ms="
            << current_time_ms
            << " active_peers="
            << routing_table.size()
            << std::endl;

#endif
    }
}
