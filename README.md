# Decentralized Mesh Protocol (Open-Core Swarm OS)

A lightweight, high-performance, CRC16-validated C++17 mesh networking protocol built for autonomous robot swarms, drones, and edge IoT nodes without central router dependencies.

## Key Architecture
- **Layered Design**: Modular HAL supporting ESP-NOW (ESP32), LoRa, and Native Desktop.
- **Dynamic Routing**: RSSI-based link quality evaluation with automatic stale route pruning.
- **Zero Configuration**: Self-healing node discovery and task distribution.

## Quickstart

### Native Host Build (CMake)
```bash
mkdir build && cd build
cmake ..
make
./mesh_node_sim
