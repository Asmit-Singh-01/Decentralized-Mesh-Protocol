# ⚡ Decentralized Mesh Protocol (Open-Core Swarm OS)

[![Build Status](https://github.com/Asmit-Singh-01/Decentralized-Mesh-Protocol/actions/workflows/build.yml/badge.svg)](https://github.com/Asmit-Singh-01/Decentralized-Mesh-Protocol/actions/workflows/build.yml)
![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue?style=for-the-badge&logo=cplusplus)
![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32-orange?style=for-the-badge&logo=platformio)
![License](https://img.shields.io/badge/License-MIT-green.svg?style=for-the-badge)
![Architecture](https://img.shields.io/badge/Architecture-P2P%20Mesh-purple?style=for-the-badge)

> **The Open-Standard Decentralized Backbone for Autonomous Swarm Robotics, Drone Constellations, and Distributed Edge Computing.**

---

## 📌 Executive Overview

The **Decentralized Mesh Protocol** is an open-core, lightweight, deterministic C++17 communication and orchestration layer designed to transform independent hardware nodes (ESP32, Raspberry Pi, STM32, and custom robotics platforms) into a unified, self-healing autonomous swarm.

Unlike traditional robotic architectures (such as centralized ROS 1 masters) that rely heavily on single-point-of-failure Wi-Fi access points, routers, or internet infrastructure, this protocol operates entirely on **zero-infrastructure peer-to-peer (P2P) mesh topologies**. Nodes autonomously discover peers, construct dynamically routed link-state graphs, distribute mission workloads, and secure telemetry using hardware-level AES-128 encryption.

```
       [ Node 0x1001 ] (Leader / Relay)
          /         \
        RSSI       RSSI
       -42dBm     -55dBm
        /             \
 [ Node 0x2001 ] <---> [ Node 0x2002 ]
   (Sensor Drone)       (Compute Drone)
        \             /
         \           /
       [ Node 0x3001 ] (Edge Worker)
```

---

## 🏗️ Core Architecture & Layer Stack

The system is architected into 5 modular, loosely coupled abstractions:

```text
+-----------------------------------------------------------------+
|               Application Layer (Swarm Missions)                |
+-----------------------------------------------------------------+
|           Swarm Orchestrator (Task Queue & Allocation)          |
+-----------------------------------------------------------------+
|             Dynamic Routing Engine (RSSI / Hop Metrics)         |
+-----------------------------------------------------------------+
|      Security & Integrity Layer (AES-128 Encryption & CRC16)    |
+-----------------------------------------------------------------+
|     Hardware Abstraction Layer (HAL) (ESP-NOW / LoRa / Sim)     |
+-----------------------------------------------------------------+
```

### Layer Breakdown

| Layer | Responsibility | Key Components |
| :--- | :--- | :--- |
| **Application Layer** | High-level autonomous swarm tasks and domain logic | Autonomous waypoint navigation, drone swarm mapping |
| **Swarm Orchestrator** | Decentralized task queuing, priority dispatch, and heartbeat tracking | [`SwarmOrchestrator`](include/swarm_orchestrator.h), Task priority queue |
| **Dynamic Routing Engine** | RSSI link-quality evaluation, best-path selection, TTL loop limits, and peer tables | [`MeshNode`](include/mesh_node.h), [`RoutingEngine`](include/routing_engine.h) |
| **Security & Integrity** | AES-128 symmetric block cipher & CCITT CRC16 verification | [`SecurityEngine`](include/security_engine.h), `calculate_crc16` |
| **Hardware Abstraction (HAL)** | Unified physical radio interface across hardware and host OS | [`IRadioDriver`](include/radio_driver.h), [`EspNowDriver`](include/esp_now_driver.h) |

---

## 🚀 Key Features

* **Zero Infrastructure Required:** Operates strictly on direct radio PHY (ESP-NOW / LoRa) without Wi-Fi routers, central servers, or internet access.
* **Self-Healing Mesh Topology:** Automatic peer discovery, non-blocking 2000 ms heartbeat pulses, and dynamic pruning of lost nodes.
* **Intelligent RSSI Route Selection:** Dynamically routes packets through the path with the strongest radio signal (highest dBm).
* **Broadcast Storm Prevention:** Time-To-Live (TTL) hop limits combined with sequence number deduplication caches stop infinite routing loops.
* **Deterministic Memory Footprint:** Zero dynamic allocations (`malloc` / `new`) in high-frequency packet processing paths for MCU stability.
* **Native Desktop Simulation:** Complete CMake build target allowing full protocol testing on Linux, macOS, and Windows without physical hardware.
* **Interactive Serial CLI:** Real-time UART debugging console to inspect active routing tables, node status, and send remote pings.

---

## 📦 Packet Structure & Framing

All over-the-air frames use a packed binary wire format with hardware-optimized alignment:

```text
+------------+------------+---------------+---------------+----------------+---------+-------------+--------------------+------------+
| Magic Byte | Packet Type| Sender Node ID| Receiver ID   | Sequence Number|   TTL   | Payload Len |      Payload       |   CRC-16   |
|  (1 Byte)  |  (1 Byte)  |   (2 Bytes)   |   (2 Bytes)   |   (2 Bytes)    | (1 Byte)|  (1 Byte)   |  (0 to 64 Bytes)   |  (2 Bytes) |
+------------+------------+---------------+---------------+----------------+---------+-------------+--------------------+------------+
```

| Field | Size | Description |
| :--- | :---: | :--- |
| `magic` | 1 Byte | Protocol validation byte (`0xD7`). Non-matching packets are discarded immediately. |
| `type` | 1 Byte | Frame type: `BEACON` (0x01), `HEARTBEAT` (0x02), `ROUTING_TABLE` (0x03), `TELEMETRY` (0x04), `TASK_ALLOCATION` (0x05), `ACK` (0x06). |
| `sender_id` | 2 Bytes | Unique 16-bit address of the transmitting node. |
| `receiver_id` | 2 Bytes | Destination node address (or `0xFFFF` for broadcast flooding). |
| `sequence_num` | 2 Bytes | Monotonically increasing packet ID used for loop and deduplication checks. |
| `ttl` | 1 Byte | Time-To-Live hop counter (default: `10`). Decremented on each relay hop. |
| `payload_len` | 1 Byte | Active size of the variable data buffer (up to 64 bytes). |
| `payload` | 0–64 Bytes | Mission data, sensor telemetry, or encrypted payload bytes. |
| `crc16` | 2 Bytes | ITU-T CRC-16 checksum covering the entire header and payload. |

For full framing specifications and byte offsets, see [docs/packet_structure.md](docs/packet_structure.md).

---

## 🛠️ Prerequisites

### Hardware Requirements
* **Microcontroller:** ESP32 DevKit / ESP32-WROOM-32 / ESP32-S3 (minimum 2 nodes recommended for live physical mesh testing).
* **Connection:** Standard Micro-USB or USB-C data cable.
* **Optional:** LoRa SX1276/SX1262 SPI Transceiver modules for long-range testing.

### Software Requirements
* **Desktop Host:**
  * CMake (>= 3.16)
  * C++17 compliant compiler (`g++`, `clang++`, or MSVC)
  * Git
* **Embedded Development:**
  * [VS Code](https://code.visualstudio.com/) with [PlatformIO IDE Extension](https://platformio.org/) installed
  * Python 3.8+

---

## 💻 Step-by-Step Setup & Quickstart

### Option A: Native Host Simulation (No Hardware Required)

You can build, execute, and verify the entire swarm mesh stack directly on your PC:

```bash
# 1. Clone the repository
git clone https://github.com/Asmit-Singh-01/Decentralized-Mesh-Protocol.git
cd Decentralized-Mesh-Protocol

# 2. Configure the CMake build environment
cmake -B build -S .

# 3. Build the native executable
cmake --build build

# 4. Run the Swarm Simulation & Test Harness
# On Linux/macOS:
./build/mesh_node_sim

# On Windows (PowerShell):
.\build\mesh_node_sim.exe
```

Expected output:
```text
>>> DECENTRALIZED SWARM OS / MESH CORE INITIALIZED <<<
[SYSTEM] Assigning autonomous swarm mission tasks...
[SYSTEM] Running initial orchestration cycle...
[SUCCESS] Active task queue size: 2
[SECURITY] Initializing AES-128 Encryption Engine...
[SECURITY SUCCESS] Encrypted packet decrypted with 100% integrity!
>>> SYSTEM CORE & SECURITY LAYER FULLY OPERATIONAL <<<
```

---

### Option B: Flashing to Physical ESP32 Hardware (PlatformIO)

#### 1. Open Project in VS Code
1. Open VS Code.
2. Ensure the **PlatformIO IDE** extension is installed and active.
3. Select **File -> Open Folder...** and choose the `Decentralized-Mesh-Protocol` directory.

#### 2. Connect Your Hardware
* Connect your ESP32 board to your computer via USB.
* PlatformIO will auto-detect the serial COM port.

#### 3. Build & Flash via PlatformIO CLI
```bash
# Compile firmware for ESP32
pio run -e esp32dev

# Flash firmware onto connected ESP32 board
pio run -e esp32dev -t upload

# Open the live Serial Monitor (115200 Baud)
pio device monitor -b 115200
```

#### 4. GUI Method
* Click the **PlatformIO Alien icon** on the left sidebar.
* Under `Project Tasks -> esp32dev`:
  * Click **Build** to compile.
  * Click **Upload** to flash.
  * Click **Monitor** to view live serial output.

---

## ⌨️ Interactive Serial CLI Commands

When connected to a node over Serial (115200 baud), the built-in CLI provides live inspection tools:

| Command | Description | Example Output |
| :--- | :--- | :--- |
| `mesh status` | Displays local node ID, active peer count, and system uptime. | Formatted status table |
| `mesh routes` | Prints formatted routing table with RSSI and hop metrics. | Formatted routing table |
| `mesh ping <node_id>` | Transmits a direct probe beacon to a specific target node (hex or decimal). | `[CLI] Ping packet sent to Node ID: 0x2002` |
| `mesh help` | Displays list of all available commands and syntax. | Help guide |

Example output for `mesh routes`:
```text
+-------------+----------+------------+------------+
| Destination | Next Hop | RSSI (dBm) | Hop Count  |
+-------------+----------+------------+------------+
| 0x2002      | 0x2002   | -42        | 1          |
| 0x2001      | 0x2001   | -85        | 2          |
+-------------+----------+------------+------------+
```

---

## 📁 Repository Directory Structure

```text
Decentralized-Mesh-Protocol/
├── .github/
│   └── workflows/
│       └── build.yml             # Automated CMake compile & execution CI
├── docs/
│   └── packet_structure.md       # Detailed packet binary layout specification
├── include/
│   ├── cli.h                     # Interactive Serial CLI interface
│   ├── esp_now_driver.h          # ESP32 ESP-NOW physical driver interface
│   ├── mesh_node.h               # Core node state & routing tables
│   ├── packet_format.h           # Wire format struct definitions & constants
│   ├── radio_driver.h            # Hardware Abstraction Layer (HAL) interface
│   ├── routing_engine.h          # Multi-hop route tracking & RSSI scoring
│   ├── security_engine.h         # AES-128 CTR encryption interface
│   └── swarm_orchestrator.h      # Swarm task queue & mission state machine
├── src/
│   ├── cli.cpp                   # Serial CLI command parser implementation
│   ├── esp_now_driver.cpp        # ESP32 physical driver / desktop mock bridge
│   ├── main.cpp                  # Native test harness and protocol simulation
│   ├── mesh_node.cpp             # Core mesh node state, TTL, & routing logic
│   ├── routing_engine.cpp        # Route lookup and metric calculation
│   ├── security_engine.cpp       # AES-128 CTR implementation
│   └── swarm_orchestrator.cpp    # Mission task handling logic
├── tools/
│   └── mesh_monitor.py           # Live Python topology mapper & serial sniffer
├── CMakeLists.txt                # Desktop native host build configuration
├── platformio.ini                # Embedded PlatformIO build configurations
├── CONTRIBUTING.md               # Contribution workflow & guidelines
├── LICENSE                       # MIT License
└── README.md                     # Project documentation
```

---

## 🤝 Contributing

Contributions from the open-source community, robotics developers, and researchers are warmly welcomed!

To contribute:
1. Review our [Contribution Guidelines](CONTRIBUTING.md) for code style and PR instructions.
2. Check the [Open Issues](https://github.com/Asmit-Singh-01/Decentralized-Mesh-Protocol/issues) for tasks to claim.
3. Fork the repository, create your feature branch (`git checkout -b feature/amazing-feature`), and open a Pull Request.

---

## 📄 License

This project is licensed under the **MIT License** — see the [LICENSE](LICENSE) file for complete details. Free for academic, commercial, and research use.
