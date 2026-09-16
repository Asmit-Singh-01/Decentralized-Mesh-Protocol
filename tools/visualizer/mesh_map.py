#!/usr/bin/env python3
"""
Mesh topology visualizer.

Reads mesh telemetry from:
  - a serial port (--port)
  - a JSON-lines telemetry file (--file)
  - a built-in demo stream (--demo)

Expected JSON examples:

{"node_id": "node1", "neighbors": [{"id": "node2", "rssi": -55}]}
{"node": "node1", "peer": "node2", "rssi": -70}
{"src": "node1", "dst": "node2", "rssi": -45}

The graph refreshes automatically as nodes join/drop.
Edge color represents RSSI strength.
"""

import argparse
import json
import random
import time
from pathlib import Path

import matplotlib.pyplot as plt
import networkx as nx

try:
    import serial
except ImportError:
    serial = None


class MeshVisualizer:
    """Maintain and draw the current mesh topology."""

    def __init__(self, timeout=10):
        self.graph = nx.Graph()
        self.last_seen = {}
        self.timeout = timeout

    @staticmethod
    def _first(data, keys):
        """Return the first non-empty value for the supplied keys."""
        for key in keys:
            value = data.get(key)
            if value is not None and value != "":
                return value
        return None

    @staticmethod
    def _rssi(value):
        """Convert an RSSI value to float when possible."""
        try:
            return float(value)
        except (TypeError, ValueError):
            return None

    def _add_link(self, source, target, rssi=None):
        """Add/update an edge between two nodes."""
        if source is None or target is None:
            return

        source = str(source)
        target = str(target)

        if source == target:
            return

        self.graph.add_node(source)
        self.graph.add_node(target)

        attributes = {"last_seen": time.time()}

        if rssi is not None:
            attributes["rssi"] = rssi

        self.graph.add_edge(source, target, **attributes)

        now = time.time()
        self.last_seen[source] = now
        self.last_seen[target] = now

    def _process_neighbor(self, source, neighbor):
        """Process one neighbor entry."""
        if isinstance(neighbor, dict):
            target = self._first(
                neighbor,
                ["id", "node_id", "node", "peer", "neighbor", "target"],
            )
            rssi = self._rssi(
                self._first(neighbor, ["rssi", "RSSI", "signal", "signal_strength"])
            )
            self._add_link(source, target, rssi)

        elif isinstance(neighbor, str):
            self._add_link(source, neighbor)

    def process_message(self, message):
        """
        Process one JSON telemetry message.

        The parser accepts several common field names so it can work
        with different mesh firmware telemetry formats.
        """
        if not isinstance(message, dict):
            return False

        now = time.time()

        # Direct source -> destination/peer message.
        source = self._first(
            message,
            ["src", "source", "from", "node_id", "node", "sender"],
        )

        target = self._first(
            message,
            ["dst", "destination", "to", "peer", "neighbor", "target"],
        )

        rssi = self._rssi(
            self._first(
                message,
                ["rssi", "RSSI", "signal", "signal_strength"],
            )
        )

        if source is not None:
            source = str(source)
            self.graph.add_node(source)
            self.last_seen[source] = now

        if source is not None and target is not None:
            self._add_link(source, target, rssi)

        # Process a neighbors list.
        neighbors = message.get("neighbors")

        if neighbors is not None and source is not None:
            if isinstance(neighbors, list):
                for neighbor in neighbors:
                    self._process_neighbor(source, neighbor)

            elif isinstance(neighbors, dict):
                for node_id, neighbor_data in neighbors.items():
                    if isinstance(neighbor_data, dict):
                        neighbor = dict(neighbor_data)
                        neighbor.setdefault("id", node_id)
                    else:
                        neighbor = {
                            "id": node_id,
                            "rssi": neighbor_data,
                        }

                    self._process_neighbor(source, neighbor)

        # Some telemetry uses "links" instead of "neighbors".
        links = message.get("links")

        if isinstance(links, list):
            for link in links:
                if isinstance(link, dict):
                    link_source = self._first(
                        link,
                        ["src", "source", "from", "node_id", "node"],
                    )
                    link_target = self._first(
                        link,
                        ["dst", "destination", "to", "peer", "target"],
                    )
                    link_rssi = self._rssi(
                        self._first(
                            link,
                            ["rssi", "RSSI", "signal", "signal_strength"],
                        )
                    )
                    self._add_link(link_source, link_target, link_rssi)

        self.remove_stale_nodes()
        return True

    def remove_stale_nodes(self):
        """Remove nodes that have not been seen recently."""
        if self.timeout <= 0:
            return

        cutoff = time.time() - self.timeout

        stale_nodes = [
            node
            for node, timestamp in self.last_seen.items()
            if timestamp < cutoff
        ]

        for node in stale_nodes:
            if node in self.graph:
                self.graph.remove_node(node)
            self.last_seen.pop(node, None)

    @staticmethod
    def edge_color(rssi):
        """
        Convert RSSI to a visual color.

        Strong signal  -> green
        Medium signal  -> orange
        Weak signal    -> red
        Unknown RSSI   -> gray
        """
        if rssi is None:
            return "gray"

        if rssi >= -55:
            return "green"

        if rssi >= -70:
            return "orange"

        return "red"

    def draw(self, axis):
        """Draw the current topology."""
        axis.clear()

        self.remove_stale_nodes()

        if self.graph.number_of_nodes() == 0:
            axis.set_title("Mesh Topology - waiting for telemetry...")
            axis.axis("off")
            return

        positions = nx.spring_layout(
            self.graph,
            seed=42,
            iterations=30,
        )

        edge_colors = []
        edge_widths = []

        for source, target, data in self.graph.edges(data=True):
            rssi = data.get("rssi")
            edge_colors.append(self.edge_color(rssi))
            edge_widths.append(2.5)

        nx.draw_networkx_nodes(
            self.graph,
            positions,
            ax=axis,
            node_size=900,
        )

        nx.draw_networkx_edges(
            self.graph,
            positions,
            ax=axis,
            edge_color=edge_colors,
            width=edge_widths,
        )

        nx.draw_networkx_labels(
            self.graph,
            positions,
            ax=axis,
            font_size=9,
        )

        edge_labels = {}

        for source, target, data in self.graph.edges(data=True):
            rssi = data.get("rssi")

            if rssi is not None:
                edge_labels[(source, target)] = f"{rssi:.0f} dBm"

        if edge_labels:
            nx.draw_networkx_edge_labels(
                self.graph,
                positions,
                edge_labels=edge_labels,
                ax=axis,
                font_size=8,
            )

        axis.set_title(
            f"Mesh Topology | "
            f"Nodes: {self.graph.number_of_nodes()} | "
            f"Links: {self.graph.number_of_edges()}"
        )

        axis.axis("off")


class SerialSource:
    """Read JSON lines from a serial port."""

    def __init__(self, port, baud):
        if serial is None:
            raise RuntimeError(
                "pyserial is required for serial input. "
                "Install it with: pip install pyserial"
            )

        self.connection = serial.Serial(
            port=port,
            baudrate=baud,
            timeout=0.1,
        )

    def read(self):
        """Return one decoded line or None."""
        if self.connection.in_waiting <= 0:
            return None

        line = self.connection.readline()

        if not line:
            return None

        try:
            return line.decode("utf-8", errors="ignore").strip()
        except UnicodeDecodeError:
            return None

    def close(self):
        self.connection.close()


class FileSource:
    """Read newly appended JSON lines from a telemetry file."""

    def __init__(self, filename):
        self.path = Path(filename)
        self.position = 0

    def read(self):
        """Return one new line from the file."""
        if not self.path.exists():
            return None

        with self.path.open("r", encoding="utf-8") as file:
            file.seek(self.position)
            line = file.readline()
            self.position = file.tell()

        return line.strip() if line else None


class DemoSource:
    """Generate simulated mesh telemetry for testing."""

    def __init__(self):
        self.nodes = ["node-A", "node-B", "node-C", "node-D"]
        self.counter = 0

    def read(self):
        """Return a demo telemetry message periodically."""
        self.counter += 1

        if self.counter % 5 != 0:
            return None

        source = random.choice(self.nodes)

        possible_targets = [
            node for node in self.nodes if node != source
        ]

        target = random.choice(possible_targets)

        rssi = random.randint(-85, -40)

        message = {
            "node_id": source,
            "neighbors": [
                {
                    "id": target,
                    "rssi": rssi,
                }
            ],
        }

        return json.dumps(message)


def parse_json_line(line):
    """Parse a JSON telemetry line."""
    if not line:
        return None

    try:
        data = json.loads(line)

        if isinstance(data, dict):
            return data

    except json.JSONDecodeError:
        pass

    return None


def create_source(args):
    """Create the requested telemetry source."""
    if args.demo:
        return DemoSource()

    if args.file:
        return FileSource(args.file)

    if args.port:
        return SerialSource(args.port, args.baud)

    raise ValueError(
        "Choose an input source with --port, --file, or --demo."
    )


def run(args):
    """Run the visualizer."""
    visualizer = MeshVisualizer(timeout=args.timeout)
    source = create_source(args)

    figure, axis = plt.subplots(figsize=(10, 7))

    def update(_frame):
        # Read several available messages per refresh.
        for _ in range(50):
            line = source.read()

            if line is None:
                break

            message = parse_json_line(line)

            if message is not None:
                visualizer.process_message(message)

        visualizer.draw(axis)

    timer = figure.canvas.new_timer(
        interval=max(100, int(args.refresh * 1000))
    )

    timer.add_callback(update, None)
    timer.start()

    update(None)

    try:
        plt.show()
    finally:
        if hasattr(source, "close"):
            source.close()


def build_parser():
    parser = argparse.ArgumentParser(
        description="Live mesh topology visualizer"
    )

    parser.add_argument(
        "--port",
        help="Serial port, e.g. COM3 or /dev/ttyUSB0",
    )

    parser.add_argument(
        "--baud",
        type=int,
        default=115200,
        help="Serial baud rate (default: 115200)",
    )

    parser.add_argument(
        "--file",
        help="JSON-lines telemetry file",
    )

    parser.add_argument(
        "--demo",
        action="store_true",
        help="Run with simulated mesh telemetry",
    )

    parser.add_argument(
        "--refresh",
        type=float,
        default=1.0,
        help="Graph refresh interval in seconds (default: 1)",
    )

    parser.add_argument(
        "--timeout",
        type=float,
        default=10.0,
        help="Remove nodes after this many seconds without telemetry",
    )

    return parser


def main():
    parser = build_parser()
    args = parser.parse_args()

    if not any([args.port, args.file, args.demo]):
        parser.error(
            "Specify --port, --file, or --demo."
        )

    try:
        run(args)
    except KeyboardInterrupt:
        print("\nVisualizer stopped.")
    except Exception as exc:
        print(f"Error: {exc}")


if __name__ == "__main__":
    main()
