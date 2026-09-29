# ⚡ Decentralized Mesh Protocol (Open-Core Swarm OS)

[![Build Status](https://github.com/Asmit-Singh-01/Decentralized-Mesh-Protocol/actions/workflows/build.yml/badge.svg)](https://github.com/Asmit-Singh-01/Decentralized-Mesh-Protocol/actions/workflows/build.yml)
![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue?style=for-the-badge&logo=cplusplus)
![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32-orange?style=for-the-badge&logo=platformio)
![License](https://img.shields.io/badge/License-MIT-green.svg?style=for-the-badge)

> **The Open-Standard Decentralized Backbone for Autonomous Swarm Robotics, Drones, and Distributed Edge Computing.**

---

## 📌 Executive Overview

The **Decentralized Mesh Protocol** is an open-core, lightweight, zero-dependency C++17 communication and orchestration layer designed to transform independent hardware nodes such as ESP32, Raspberry Pi, STM32, and custom robotics platforms into a unified, self-healing autonomous swarm.

Unlike traditional architectures that rely heavily on central network infrastructure, Wi-Fi access points, or master nodes, this protocol is designed around a zero-infrastructure **peer-to-peer (P2P) mesh topology**.

Nodes can discover peers, form dynamically routed mesh graphs, execute mission tasks, and secure payloads using AES-128 CTR encryption and CRC16 integrity verification.

---

## 🏗️ Core Architecture & Layer Stack

The system is organized into five modular layers:

```text
+-----------------------------------------------------------------+
|              Application Layer (Swarm Missions)                 |
+-----------------------------------------------------------------+
|          Swarm Orchestrator (Task Queue & Allocation)           |
+-----------------------------------------------------------------+
|            Dynamic Routing Engine (RSSI / Hop Metrics)           |
+-----------------------------------------------------------------+
|       Security & Integrity Layer (AES-128 & CRC16)              |
+-----------------------------------------------------------------+
|      Hardware Abstraction Layer (ESP-NOW / LoRa / Sim)         |
+-----------------------------------------------------------------+
```

### Architectural Breakdown

1. **Hardware Abstraction Layer (HAL)**
   Provides the unified `IRadioDriver` interface for different radio and simulation backends.

2. **Security & Integrity Engine**
   Provides AES-128 CTR payload encryption and CRC16 checksum verification.

3. **Dynamic Routing Engine**
   Handles RSSI-based link evaluation, route metrics, stale-route pruning, and multi-hop forwarding.

4. **Swarm Orchestrator**
   Handles distributed state synchronization, task assignment, mission prioritization, and heartbeat execution.

5. **Application Layer**
   Provides the interface for swarm missions and higher-level application logic.

---

## 🚀 Key Features

- **Zero Infrastructure Required:** Designed for direct radio communication such as ESP-NOW and LoRa without requiring a central Wi-Fi router.
- **Deterministic Memory Footprint:** Core hot paths minimize dynamic memory allocation for embedded stability.
- **Hardware-Independent Simulation:** Includes mock drivers and CMake desktop targets for simulation and CI testing.
- **Driver Abstraction:** Designed to support additional radio backends such as LoRa, NRF24L01, and Ethernet.
- **Security & Integrity:** Includes AES-128 CTR encryption and CRC16 packet integrity verification.
- **Multi-Hop Routing:** Supports route tracking and forwarding based on link and hop metrics.

---

## 📁 Project Structure

```text
Decentralized-Mesh-Protocol/
├── .github/
│   └── workflows/
│       └── build.yml
│
├── include/
│   ├── esp_now_driver.h
│   ├── mesh_node.h
│   ├── packet_format.h
│   ├── radio_driver.h
│   ├── routing_engine.h
│   ├── security_engine.h
│   └── swarm_orchestrator.h
│
├── src/
│   ├── esp_now_driver.cpp
│   ├── main.cpp
│   ├── routing_engine.cpp
│   ├── security_engine.cpp
│   └── swarm_orchestrator.cpp
│
├── tools/
│   └── mesh_monitor.py
│
├── CMakeLists.txt
├── platformio.ini
└── README.md
```

### Important Directories

| Directory/File | Purpose |
|---|---|
| `include/` | C++ header files and interfaces |
| `src/` | C++ implementation files |
| `tools/` | Monitoring and utility scripts |
| `.github/workflows/` | GitHub Actions CI workflow |
| `CMakeLists.txt` | Desktop build configuration |
| `platformio.ini` | ESP32 PlatformIO configuration |
| `README.md` | Project documentation |

---

# 💻 Quickstart & Setup Guide

## Prerequisites

### Desktop Simulation

Install:

- CMake 3.16 or newer
- GCC or Clang with C++17 support
- Git

### ESP32 Development

Install:

- Visual Studio Code
- PlatformIO IDE extension
- ESP32 Dev Module
- USB data cable

---

## Option A: Native Desktop Simulation

The desktop simulation can be built without physical ESP32 hardware.

### 1. Clone the repository

```bash
git clone https://github.com/Asmit-Singh-01/Decentralized-Mesh-Protocol.git
cd Decentralized-Mesh-Protocol
```

### 2. Configure the CMake build

```bash
cmake -B build -S .
```

### 3. Compile the project

```bash
cmake --build build
```

### 4. Run the simulation

On Windows:

```powershell
.\build\mesh_node_sim.exe
```

On Linux/macOS:

```bash
./build/mesh_node_sim
```

---

## Option B: ESP32 Hardware Setup

The project uses **PlatformIO** to build and upload firmware to the ESP32 Dev Module.

### 1. Install PlatformIO

Install Visual Studio Code and the PlatformIO IDE extension.

After installation, open this repository in VS Code.

### 2. Connect the ESP32

Connect the ESP32 Dev Module to your computer using a USB data cable.

### 3. Check the PlatformIO configuration

The project uses:

```text
Board: ESP32 Dev Module
Platform: Espressif32
Framework: Arduino
C++ Standard: C++17
Serial Monitor: 115200 baud
```

The configuration is stored in `platformio.ini`.

### 4. Build the firmware

From the project root:

```bash
pio run
```

### 5. Upload the firmware

Make sure the ESP32 is connected, then run:

```bash
pio run --target upload
```

### 6. Open the Serial Monitor

Run:

```bash
pio device monitor
```

The configured serial communication speed is:

```text
115200
```

You can also open the PlatformIO Serial Monitor from VS Code.

---

## 🔧 PlatformIO Configuration

The current default environment is:

```ini
[platformio]
default_envs = esp32dev

[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
build_flags =
    -std=c++17
    -Iinclude
```

---

## 🧪 Testing

### Desktop Build

Run:

```bash
cmake -B build -S .
cmake --build build
```

Then execute:

```bash
./build/mesh_node_sim
```

On Windows PowerShell:

```powershell
.\build\mesh_node_sim.exe
```

### ESP32 Build

Run:

```bash
pio run
```

### ESP32 Upload

Run:

```bash
pio run --target upload
```

---

## 📡 Mesh Monitoring

The project includes a Python monitoring utility:

```text
tools/mesh_monitor.py
```

Run it with:

```bash
python tools/mesh_monitor.py
```

Make sure the required Python dependencies are installed if the monitoring script requires additional packages.

---

## 🏛️ Architecture Diagram

The high-level communication flow is:

```text
                    +----------------------+
                    |   Swarm Application  |
                    +----------+-----------+
                               |
                               v
                    +----------------------+
                    | Swarm Orchestrator   |
                    +----------+-----------+
                               |
                               v
                    +----------------------+
                    | Dynamic Routing      |
                    | Engine               |
                    +----------+-----------+
                               |
                               v
                    +----------------------+
                    | Security & Integrity |
                    | AES-128 + CRC16      |
                    +----------+-----------+
                               |
                               v
                    +----------------------+
                    | Hardware Abstraction |
                    | Layer (HAL)          |
                    +----------+-----------+
                               |
                 +-------------+-------------+
                 |                           |
                 v                           v
          +-------------+              +-------------+
          |   ESP-NOW   |              |     LoRa    |
          +-------------+              +-------------+
                 |                           |
                 +-------------+-------------+
                               |
                               v
                       Mesh Network Nodes
```

---

## 🤝 Contributing

Contributions are welcome.

### Contribution Workflow

1. Fork the repository.
2. Clone your fork.
3. Create a feature branch.
4. Make your changes.
5. Build and test the project.
6. Commit your changes.
7. Push your branch.
8. Open a Pull Request.

For detailed contribution guidelines, see [CONTRIBUTING.md](CONTRIBUTING.md).

---

## 📄 License

This project is licensed under the MIT License.

See the [LICENSE](LICENSE) file for details.

---

## 📚 Documentation

Useful project documentation includes:

- Architecture documentation
- Simulation guide
- PlatformIO configuration
- Contribution guidelines
- Project source code

Refer to the repository documentation and `docs/` directory for additional guides.
