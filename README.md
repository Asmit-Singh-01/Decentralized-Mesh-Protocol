# ⚡ Decentralized Mesh Protocol (Open-Core Swarm OS)

![Build Status](https://img.shields.io/github/actions/workflow/status/Asmit-Singh-01/Decentralized-Mesh-Protocol/build.yml?branch=main&style=for-the-badge&logo=github)
![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.style=for-the-badge&logo=cplusplus)
![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32-orange?style=for-the-badge&logo=platformio)
![License](https://img.shields.io/badge/License-MIT-green.svg?style=for-the-badge)
![PRs Welcome](https://img.shields.io/badge/PRs-Welcome-brightgreen.svg?style=for-the-badge)

> **The Open-Standard Decentralized Backbone for Autonomous Swarm Robotics, Drones, and Distributed Edge Computing.**

---

## 📌 Executive Overview

The **Decentralized Mesh Protocol** is an open-core, lightweight, zero-dependency C++17 communication and orchestration layer designed to transform independent hardware nodes (ESP32, Raspberry Pi, STM32, Custom Robotics Platforms) into a unified, self-healing autonomous swarm.

Unlike traditional ROS (Robot Operating System) architectures that rely heavily on central network infrastructure, Wi-Fi access points, or master nodes, this protocol operates on zero-infrastructure **peer-to-peer (P2P) mesh topology**. Nodes auto-discover, form dynamically routed mesh graphs, execute mission tasks, and secure payloads over air using hardware-level AES-128 CTR encryption.

---

## 🏗️ Core Architecture & Layer Stack

The system is architected into 5 modular, loosely coupled abstractions:

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


### Architectural Breakdown
1. **Hardware Abstraction Layer (HAL):** Unified `IRadioDriver` interface. Abstracted drivers allow identical code to run on physical ESP32 Wi-Fi PHY, LoRa modules, or Desktop Virtual Sockets.
2. **Security & Integrity Engine:** Symmetric AES-128 CTR payload encryption paired with lightweight CRC16 checksum verification for low-latency embedded microcontrollers.
3. **Dynamic Routing Engine:** RSSI link-quality evaluation, dynamic metric calculation, auto-pruning of stale routes, and multi-hop forward decision-making.
4. **Swarm Orchestrator:** Distributed state sync, decentralized task assignment, mission prioritization, and heartbeat execution.

---

## 🚀 Key Features

* **Zero Infrastructure Required:** Operates strictly on direct radio layer (ESP-NOW / LoRa PHY) without Wi-Fi routers or cellular networks.
* **Deterministic Memory Footprint:** Built with zero dynamic heap allocations in core hot-paths for maximum micro-controller stability.
* **Hardware Independent Simulation:** Includes mock drivers and CMake desktop targets for automated CI/CD simulation pipelines without physical hardware.
* **Plug-and-Play Driver Abstraction:** Easily extendable to add LoRa (SX1276/SX1262), NRF24L01, or Ethernet adapters.
* **Enterprise Security:** Built-in lightweight AES-128 CTR block encryption preventing air-sniffing and network injection attacks.

---

## 📂 Repository File Structure

Decentralized-Mesh-Protocol/
├── .github/
│   └── workflows/
│       └── build.yml             # Automated CMake compile & execution CI
├── include/
│   ├── esp_now_driver.h          # ESP32 ESP-NOW physical driver interface
│   ├── mesh_node.h               # Core node state abstraction
│   ├── packet_format.h           # Low-level bitfield packet definitions
│   ├── radio_driver.h            # Pure virtual Hardware Abstraction Layer (HAL)
│   ├── routing_engine.h          # Multi-hop mesh route tracking and RSSI pruning
│   ├── security_engine.h         # AES-128 Encryption engine interface
│   └── swarm_orchestrator.h      # Swarm task queue & state machine
├── src/
│   ├── esp_now_driver.cpp        # ESP32 physical / desktop mock bridge logic
│   ├── main.cpp                  # Primary entry point & test harness
│   ├── routing_engine.cpp        # Route lookup and metric calculation
│   ├── security_engine.cpp       # AES-128 CTR implementation
│   └── swarm_orchestrator.cpp    # Mission task handling logic
├── tools/
│   └── mesh_monitor.py           # Live python telemetry sniffer & logger
├── CMakeLists.txt                # Desktop compilation configuration
├── platformio.ini                # Embedded PlatformIO build flags
└── README.md                     # Documentation

---

## 💻 Quickstart & Setup Guide

### Prerequisites
* **For Desktop Compilation & CI:** `cmake` (>= 3.16), `g++` / `clang++` with C++17 support.
* **For Microcontroller Deployment:** [PlatformIO IDE](https://platformio.org/) or PlatformIO Core CLI.

---

### Option A: Native Desktop Simulation (No Hardware Needed)

Clone the repository and run the simulated build target:

```bash
# 1. Clone the repository
git clone [https://github.com/Asmit-Singh-01/Decentralized-Mesh-Protocol.git](https://github.com/Asmit-Singh-01/Decentralized-Mesh-Protocol.git)
cd Decentralized-Mesh-Protocol

# 2. Configure CMake build directory
cmake -B build -S .

# 3. Compile the simulation target
cmake --build build

# 4. Execute the Swarm Protocol Test Harness
./build/mesh_node_sim

