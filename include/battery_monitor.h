#pragma once

#include "mesh_node.h"

namespace BatteryMonitor
{
    float read_battery_voltage();

    bool check_battery(MeshNode& mesh_node);

    bool is_low_power();
}
