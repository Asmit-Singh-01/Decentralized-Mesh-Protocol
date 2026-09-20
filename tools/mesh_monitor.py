#!/usr/bin/env python3
"""Lightweight telemetry monitor for the decentralized mesh protocol."""

import argparse
import json
import random
import re
import signal
import sys
import time
from datetime import datetime

RESET = "\033[0m"
BLUE = "\033[94m"
GREEN = "\033[92m"
RED = "\033[91m"
YELLOW = "\033[93m"
DIM = "\033[2m"
BOLD = "\033[1m"

_RE_NODE = re.compile(r"(?:Node|node)\s+(0x[0-9A-Fa-f]+|\d+)")
_RE_RSSI = re.compile(r"(?:RSSI|rssi)\s*[=:]?\s*(-?\d+)")
_RE_TYPE = re.compile(r"(?:Packet Type|packet type)\s*[=:]?\s*([A-Z_]+)", re.IGNORECASE)


class NodeStat:
    def __init__(self, node_id):
        self.node_id = node_id
        self.packet_count = 0
        self.rssi_samples = []
        self.first_seen = datetime.now()
        self.last_seen = datetime.now()

    @property
    def avg_rssi(self):
        if not self.rssi_samples:
            return "N/A"
        return f"{sum(self.rssi_samples) / len(self.rssi_samples):.1f} dBm"

    @property
    def last_rssi(self):
        if not self.rssi_samples:
            return "N/A"
        return f"{self.rssi_samples[-1]} dBm"


class MeshMonitor:
    PACKET_TYPES = ("BEACON", "HEARTBEAT", "ROUTING_TABLE", "TELEMETRY", "TASK", "ACK", "OTHER")

    def __init__(self, log_file=None):
        self.log_file = log_file
        self.start_time = time.monotonic()
        self.total_lines = 0
        self.total_packets = 0
        self.errors = 0
        self.success = 0
        self.malformed = 0
        self.packet_type_counts = {name: 0 for name in self.PACKET_TYPES}
        self.nodes = {}

    def _classify(self, line):
        if "[SIM]" in line or "[SYSTEM]" in line:
            return "SYSTEM"
        if "[STATUS]" in line or "[SUCCESS]" in line or "SECURITY SUCCESS" in line:
            return "SUCCESS"
        if "ERROR" in line or "FAIL" in line or "FAILED" in line:
            return "ERROR"
        if "[NETWORK]" in line or "[ROUTING]" in line:
            return "NETWORK"
        if "[SECURITY]" in line:
            return "SECURITY"
        if "[SWARM]" in line or "[TASK]" in line:
            return "SWARM"
        if "[TELEMETRY]" in line:
            return "TELEMETRY"
        return "INFO"

    def _extract_node_id(self, line):
        match = _RE_NODE.search(line)
        return match.group(1) if match else None

    def _extract_rssi(self, line):
        match = _RE_RSSI.search(line)
        return int(match.group(1)) if match else None

    def _extract_packet_type(self, line):
        match = _RE_TYPE.search(line)
        if match:
            value = match.group(1).upper()
            if value in self.PACKET_TYPES:
                return value
        return "OTHER"

    def _update_node(self, node_id, rssi):
        if not node_id:
            return
        if node_id not in self.nodes:
            self.nodes[node_id] = NodeStat(node_id)
            print(f"  {BLUE}[DISCOVERY]{RESET} New mesh node detected: {BOLD}{node_id}{RESET}")
        stats = self.nodes[node_id]
        stats.packet_count += 1
        stats.last_seen = datetime.now()
        if rssi is not None:
            stats.rssi_samples.append(rssi)

    def _count_packet_type(self, packet_type):
        if packet_type in self.packet_type_counts:
            self.packet_type_counts[packet_type] += 1
        else:
            self.packet_type_counts["OTHER"] += 1

    def _looks_like_noise(self, line):
        lowered = line.lower()
        return any(marker in lowered for marker in (
            "warning",
            "notice",
            "debug",
            "traceback",
            "info",
            "logging",
        ))

    def parse_line(self, line):
        line = line.strip()
        if not line:
            return

        self.total_lines += 1
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        line_type = self._classify(line)
        node_id = self._extract_node_id(line)
        rssi = self._extract_rssi(line)
        packet_type = self._extract_packet_type(line)

        if line_type == "ERROR":
            self.errors += 1
        elif line_type == "SUCCESS":
            self.success += 1

        is_mesh_event = any(tag in line for tag in (
            "[SIM]",
            "[SYSTEM]",
            "[STATUS]",
            "[SUCCESS]",
            "[NETWORK]",
            "[SECURITY]",
            "[ROUTING]",
            "[SWARM]",
            "[TELEMETRY]",
            "[TASK]",
            "RSSI",
            "Packet",
            "Node",
            "node",
        ))

        if is_mesh_event:
            self.total_packets += 1
            self._update_node(node_id, rssi)
            self._count_packet_type(packet_type)
        elif not self._looks_like_noise(line):
            self.malformed += 1
            print(f"{RED}[MALFORMED]{RESET} {line}")
            if self.log_file:
                self._save_to_log({
                    "timestamp": timestamp,
                    "type": "MALFORMED",
                    "message": line,
                    "node_id": node_id,
                    "rssi_dbm": rssi,
                    "packet_type": packet_type,
                })
            return

        color = {
            "SYSTEM": BLUE,
            "SUCCESS": GREEN,
            "ERROR": RED,
            "NETWORK": BLUE,
            "SECURITY": YELLOW,
            "SWARM": YELLOW,
            "TELEMETRY": BLUE,
        }.get(line_type, "")
        print(f"{DIM}[{timestamp}]{RESET} {color}{line}{RESET}")

        if self.log_file:
            self._save_to_log({
                "timestamp": timestamp,
                "type": line_type,
                "message": line,
                "node_id": node_id,
                "rssi_dbm": rssi,
                "packet_type": packet_type,
            })

    def _save_to_log(self, data):
        try:
            with open(self.log_file, "a", encoding="utf-8") as handle:
                handle.write(json.dumps(data) + "\n")
        except OSError as exc:
            print(f"{RED}[LOG ERROR]{RESET} Could not write to log file: {exc}")

    def display_summary(self):
        elapsed = time.monotonic() - self.start_time
        print(f"\n{BOLD}{'=' * 54}{RESET}")
        print(f"{BOLD}          MESH TELEMETRY SUMMARY REPORT{RESET}")
        print(f"{BOLD}{'=' * 54}{RESET}")
        print(f"  {'Session duration':<24} {elapsed:.1f}s")
        print(f"  {'Lines processed':<24} {self.total_lines}")
        print(f"  {'Mesh events detected':<24} {self.total_packets}")
        print(f"  {'Successful transmissions':<24} {self.success}")
        print(f"  {'Errors / failures':<24} {self.errors}")
        print(f"  {'Malformed lines':<24} {self.malformed}")
        print(f"  {'Observed nodes':<24} {len(self.nodes)}")
        if self.packet_type_counts:
            print(f"\n{BOLD}  Packet Type Distribution:{RESET}")
            for packet_type, count in sorted(self.packet_type_counts.items(), key=lambda item: (-item[1], item[0])):
                if count:
                    print(f"    {packet_type:<18} {count:>4}")
        if self.nodes:
            print(f"\n{BOLD}  Active Mesh Nodes:{RESET}")
            print(f"    {'Node ID':<12} {'Pkts':>5}  {'Avg RSSI':>12}  {'Last RSSI':>12}")
            for node_id in sorted(self.nodes):
                stats = self.nodes[node_id]
                print(f"    {node_id:<12} {stats.packet_count:>5}  {stats.avg_rssi:>12}  {stats.last_rssi:>12}")
        print(f"{BOLD}{'=' * 54}{RESET}\n")


def run_stdin_mode(monitor):
    print(f"{BLUE}[INFO]{RESET} Reading from stdin — pipe mesh output here.\n")
    try:
        for raw in sys.stdin:
            monitor.parse_line(raw)
    except (KeyboardInterrupt, EOFError):
        pass


def run_file_mode(monitor, path, interval, idle_timeout=None):
    print(f"{BLUE}[INFO]{RESET} Following file: {path}\n")
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as handle:
            handle.seek(0, 2)
            last_activity = time.monotonic()
            while True:
                line = handle.readline()
                if line:
                    monitor.parse_line(line)
                    last_activity = time.monotonic()
                elif idle_timeout is not None and (time.monotonic() - last_activity) >= idle_timeout:
                    print(f"{YELLOW}[INFO]{RESET} No new data for {idle_timeout:.1f}s; exiting cleanly.")
                    break
                else:
                    time.sleep(interval)
    except FileNotFoundError:
        print(f"{RED}[ERROR]{RESET} File not found: {path}", file=sys.stderr)
        sys.exit(1)
    except (KeyboardInterrupt, EOFError):
        pass


def generate_mock_line():
    events = [
        "[SIM] Swarm mesh network initialized",
        "[SYSTEM] Node {node} initialized successfully",
        "[STATUS] Node {node} joined mesh RSSI={rssi}",
        "[NETWORK] Packet Type=BEACON from Node {node} RSSI={rssi}",
        "[NETWORK] Packet Type=HEARTBEAT from Node {node}",
        "[SWARM] Task 0x{task:04X} assigned to Node {node} priority={prio}",
        "[ROUTING] Route to Node {node} updated hop_count=2 RSSI={rssi}",
        "[SUCCESS] Packet serialized and delivered to Node {node}",
        "[SECURITY] Encrypted packet verified for Node {node}",
        "[ERROR] CRC mismatch on packet from Node {node} — dropping",
        "[NETWORK] Packet Type=ACK from Node {node}",
    ]
    node = random.choice(["0x1001", "0x1002", "0x1003", "0x2001"])
    rssi = random.randint(-90, -30)
    task = random.randint(100, 999)
    prio = random.randint(1, 9)
    template = random.choice(events)
    return template.format(node=node, rssi=rssi, task=task, prio=prio)


def run_demo_mode(monitor, interval, duration=None):
    print(f"{YELLOW}[DEMO]{RESET} Running mock telemetry generator (Ctrl+C to stop).\n")
    start = time.monotonic()
    try:
        while True:
            monitor.parse_line(generate_mock_line())
            time.sleep(interval)
            if duration is not None and (time.monotonic() - start) >= duration:
                break
    except KeyboardInterrupt:
        pass


def main():
    parser = argparse.ArgumentParser(description="Swarm Mesh Protocol telemetry monitor")
    parser.add_argument("--log-file", type=str, default=None, help="Path to save JSON telemetry log")
    parser.add_argument("--file", type=str, default=None, help="Follow a log file similar to tail -f")
    parser.add_argument("--demo", action="store_true", help="Generate mock mesh traffic without hardware")
    parser.add_argument("--interval", type=float, default=1.0, help="Polling or demo interval in seconds")
    parser.add_argument("--duration", type=float, default=None, help="Stop demo mode after this many seconds")
    parser.add_argument("--idle-timeout", type=float, default=None, help="Exit file mode after this many idle seconds")
    args = parser.parse_args()

    print(f"\n{BOLD}{'=' * 54}{RESET}")
    print(f"{BOLD}   SWARM MESH PROTOCOL — TELEMETRY MONITOR v2.1{RESET}")
    print(f"{BOLD}{'=' * 54}{RESET}")
    if args.log_file:
        print(f"{BLUE}[INFO]{RESET} Logging to {args.log_file}")
    print(f"{BLUE}[INFO]{RESET} Press Ctrl+C to stop and display summary.\n")

    monitor = MeshMonitor(log_file=args.log_file)

    def _shutdown(signum, frame):
        raise KeyboardInterrupt

    signal.signal(signal.SIGINT, _shutdown)

    try:
        if args.demo:
            run_demo_mode(monitor, args.interval, args.duration)
        elif args.file:
            run_file_mode(monitor, args.file, args.interval, args.idle_timeout)
        else:
            run_stdin_mode(monitor)
    except KeyboardInterrupt:
        pass
    finally:
        monitor.display_summary()
        print(f"{BLUE}[INFO]{RESET} Monitor shut down safely.")

    return 0 if monitor.errors == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
        
