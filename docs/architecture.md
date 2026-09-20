# Architecture Overview

This project is built around a small layered mesh architecture that separates hardware I/O, packet handling, routing, swarm orchestration, and security.

## Layering

### 1. Hardware abstraction layer

The HAL is defined by `IRadioDriver` in `include/radio_driver.h` and implemented by `EspNowDriver` in `include/esp_now_driver.h` and `src/esp_now_driver.cpp`.

The abstraction exposes:

- driver initialization
- byte transmission to a target peer
- a receive callback for incoming radio frames

When the build is not targeting an ESP32, the desktop implementation is used so the core protocol can be built and tested without embedded hardware.

### 2. Packet format and integrity

`MeshPacket` and `PacketHeader` are defined in `include/packet_format.h`. Packet serialization and CRC handling are implemented in `src/packet_format.cpp`.

Key checks include:

- magic byte validation
- payload length validation
- CRC16-CCITT verification
- deterministic serialization for desktop tests

### 3. Mesh node and peer tracking

`MeshNode` in `include/mesh_node.h` tracks local packet sequence numbers, deduplicates incoming packets, and maintains peer metadata such as RSSI and recent activity time. This is the state base for message reception and forwarding without a larger runtime.

### 4. Routing engine

`RoutingEngine` in `include/routing_engine.h` stores route entries keyed by destination and updates them using beacon information. The route policy currently uses a lightweight metric built from RSSI and hop count, and stale entries are removed with `prune_stale_routes()`.

### 5. Security engine

`SecurityEngine` in `include/security_engine.h` provides a small symmetric encryption and decryption path using a 16-byte key and nonce-based XOR logic. The implementation is intentionally minimal and device-friendly.

### 6. Swarm orchestration

`SwarmOrchestrator` in `include/swarm_orchestrator.h` keeps a priority-sorted queue of tasks and handles incoming task payloads. The runtime logic is small and deterministic so it can be tested directly on desktop builds.

## Build and execution flow

The desktop build target is configured in `CMakeLists.txt` and produces `mesh_node_sim` from `src/main.cpp` plus the core implementation files.

The PlatformIO configuration in `platformio.ini` targets an `esp32dev` board and includes the same `include` directory for embedded builds.

## Simulation and testing

The project uses a desktop simulation entry point in `src/main.cpp` and a lightweight CMake test target when the testing branch is enabled. This makes it possible to validate protocol logic without requiring a physical radio or device.
