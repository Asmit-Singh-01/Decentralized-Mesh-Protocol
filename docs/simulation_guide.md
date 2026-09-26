# Native Mesh Node Simulation Guide

## 1. Requirements

This simulator can be built and run without physical ESP32 or LoRa
hardware.

Required tools:

- CMake
- MinGW/GCC
- MinGW Make

Check the installed tools:

```powershell
gcc --version
mingw32-make --version
cmake --version



## 2. Build the Simulator

Open PowerShell in the project root:

```text
Decentralized-Mesh-Protocol


## 3. Run the Simulator

Run the simulator with:

```powershell
.\build\mesh_node_sim.exe


## 4. Running Multiple Local Simulator Processes

The simulator can be started as multiple local processes.

Open separate PowerShell terminals and run:

```powershell
.\build\mesh_node_sim.exe


## 5. Packet and Security Verification

The simulator tests packet serialization and CRC16:

```text
[NETWORK] Testing Packet Serialization & CRC16...
[SUCCESS] Packet Serialized! Size: 24 bytes



## 6. Troubleshooting

### CMake selects NMake

If CMake reports an error involving `nmake`, explicitly select MinGW:

```powershell
cmake -S . -B build -G "MinGW Makefiles"


## 7. Clean Rebuild

To completely rebuild the simulator:

```powershell
Remove-Item -Recurse -Force build
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build