# Mesh Telemetry Monitor

The monitor in `tools/mesh_monitor.py` reads telemetry from standard input, a growing log file, or a built-in mock generator.

## Basic usage

Read from simulation output:

```bash
./build/mesh_node_sim | python tools/mesh_monitor.py
```

Follow a file as it grows:

```bash
python tools/mesh_monitor.py --file ./mesh.log --interval 1.0
```

Run a hardware-free demo:

```bash
python tools/mesh_monitor.py --demo --interval 0.5 --duration 10
```

Save JSON telemetry while monitoring input:

```bash
./build/mesh_node_sim | python tools/mesh_monitor.py --log-file telemetry.json
```

## Useful flags

- `--demo`: generate synthetic mesh events for local testing
- `--file PATH`: follow a log file like `tail -f`
- `--interval SECONDS`: mock or polling interval
- `--duration SECONDS`: stop demo mode automatically
- `--idle-timeout SECONDS`: stop file mode when no new data appears
- `--log-file PATH`: append structured JSON entries for each parsed line

## Monitoring behavior

The tool tracks:

- total processed lines
- mesh events
- success and error totals
- malformed line counts
- per-node packet counts and average RSSI
- packet type distribution

Malformed input is surfaced as `[MALFORMED]` instead of crashing the monitor, which makes it easier to diagnose broken serial or log output.

## Compatibility

- Python 3.8+
- Standard library only
- No internet access required
- Works with desktop simulation and local log debugging
