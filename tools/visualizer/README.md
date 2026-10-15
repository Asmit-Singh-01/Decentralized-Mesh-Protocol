# Mesh Topology Visualizer

A Python tool for visualizing the active mesh topology from node telemetry.

## Features

- Parses JSON telemetry from mesh nodes
- Supports serial input using `pyserial`
- Visualizes nodes and connections using `networkx` and `matplotlib`
- Automatically refreshes the topology
- Removes inactive nodes after a configurable timeout
- Supports demo mode for testing without hardware

## Installation

From the `tools/visualizer` directory:

```bash
pip install -r requirements.txt
