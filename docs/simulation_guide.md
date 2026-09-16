# Native Mesh Node Simulation Guide

This guide explains how to build and run the mesh node simulator locally without physical ESP32 or LoRa hardware.

The project provides a native CMake build that produces the `mesh_node_sim` executable.

## Prerequisites

Install:

- CMake 3.16 or newer
- A C++17-compatible compiler
- Git

The project requires C++17 for the native build. 

### Windows

Install CMake and a C++ compiler such as the Visual Studio C++ build tools.

Make sure `cmake` is available from the command prompt.

### Linux

On Debian/Ubuntu:

```bash
sudo apt update
sudo apt install cmake g++
