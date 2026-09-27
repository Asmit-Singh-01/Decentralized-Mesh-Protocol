import argparse
import json
import random
import time
from datetime import datetime

import matplotlib.pyplot as plt
import networkx as nx
import serial


class MeshVisualizer:
    """Live visualization of active mesh nodes and links."""

    def __init__(self, timeout=10):
        self.timeout = timeout

        # node_id -> last time the node was seen
        self.nodes = {}

        # (node_a, node_b) -> link information
        self.links = {}

    def process_telemetry(self, message):
        """
        Process one telemetry message.

        Expected JSON format:

        {
            "sender": 1,
            "receiver": 2,
            "rssi": -58
        }
        """

        try:
            sender = int(message["sender"])
            receiver = int(message["receiver"])
            rssi = int(message["rssi"])

        except (KeyError, TypeError, ValueError):
            print(f"[WARNING] Invalid telemetry: {message}")
            return

        now = time.time()

        # Mark both nodes as active.
        self.nodes[sender] = now
        self.nodes[receiver] = now

        # Store the link using a sorted tuple so
        # 1 -> 2 and 2 -> 1 are treated as the same link.
        link = tuple(sorted((sender, receiver)))

        self.links[link] = {
            "rssi": rssi,
            "last_seen": now,
        }

    def cleanup(self):
        """Remove nodes and links that have been inactive too long."""

        now = time.time()

        # Remove inactive nodes.
        dead_nodes = [
            node
            for node, last_seen in self.nodes.items()
            if now - last_seen > self.timeout
        ]

        for node in dead_nodes:
            del self.nodes[node]

        # Remove inactive links.
        dead_links = [
            link
            for link, data in self.links.items()
            if now - data["last_seen"] > self.timeout
        ]

        for link in dead_links:
            del self.links[link]

    def draw(self):
        """Draw the current mesh topology."""

        self.cleanup()

        graph = nx.Graph()

        # Add active nodes.
        for node in self.nodes:
            graph.add_node(node)

        # Add active links.
        for (node_a, node_b), data in self.links.items():

            if node_a in self.nodes and node_b in self.nodes:

                graph.add_edge(
                    node_a,
                    node_b,
                    rssi=data["rssi"],
                )

        # Clear previous graph.
        plt.clf()

        # Nothing received yet.
        if graph.number_of_nodes() == 0:
            plt.title("Mesh Topology - Waiting for telemetry...")
            plt.axis("off")
            plt.pause(0.1)
            return

        # Calculate node positions.
        positions = nx.spring_layout(
            graph,
            seed=42,
        )

        # Draw nodes.
        nx.draw_networkx_nodes(
            graph,
            positions,
            node_size=900,
        )

        # Draw node labels.
        nx.draw_networkx_labels(
            graph,
            positions,
            font_size=10,
            font_weight="bold",
        )

        # Draw connections.
        nx.draw_networkx_edges(
            graph,
            positions,
            width=2,
        )

        # Get RSSI values for edges.
        rssi_attributes = nx.get_edge_attributes(
            graph,
            "rssi",
        )

        # Convert RSSI values into labels.
        edge_labels = {
            edge: f"{rssi} dBm"
            for edge, rssi in rssi_attributes.items()
        }

        # Draw RSSI labels.
        nx.draw_networkx_edge_labels(
            graph,
            positions,
            edge_labels=edge_labels,
            font_size=9,
        )

        # Display graph statistics.
        plt.title(
            f"Active Mesh Topology | "
            f"Nodes: {graph.number_of_nodes()} | "
            f"Links: {graph.number_of_edges()}"
        )

        plt.axis("off")
        plt.tight_layout()

        # Refresh the window.
        plt.pause(0.1)


def demo_stream(visualizer):
    """
    Generate simulated mesh telemetry.

    This allows the visualizer to be tested without
    connecting an ESP32 or other physical hardware.
    """

    links = [
        (1, 2),
        (2, 3),
        (1, 3),
        (3, 4),
    ]

    print("[INFO] Demo telemetry stream started.")
    print("[INFO] Press Ctrl+C to stop.")

    while True:

        sender, receiver = random.choice(links)

        telemetry = {
            "sender": sender,
            "receiver": receiver,
            "rssi": random.randint(-80, -40),
            "timestamp": datetime.now().isoformat(),
        }

        print(
            f"[TELEMETRY] "
            f"Node {sender} -> Node {receiver} | "
            f"RSSI: {telemetry['rssi']} dBm"
        )

        visualizer.process_telemetry(telemetry)

        visualizer.draw()

        time.sleep(1)


def serial_stream(port, baudrate, visualizer):
    """
    Read JSON telemetry from a serial/USB device.

    Expected serial line:

    {"sender":1,"receiver":2,"rssi":-58}
    """

    print(
        f"[INFO] Opening serial port "
        f"{port} at {baudrate} baud..."
    )

    try:

        with serial.Serial(
            port,
            baudrate,
            timeout=1,
        ) as connection:

            print("[INFO] Serial connection established.")
            print("[INFO] Waiting for mesh telemetry...")

            while True:

                raw_line = connection.readline()

                # No data received during this cycle.
                if not raw_line:
                    visualizer.draw()
                    continue

                try:

                    line = raw_line.decode(
                        "utf-8",
                        errors="ignore",
                    ).strip()

                    if not line:
                        continue

                    telemetry = json.loads(line)

                    print(
                        f"[SERIAL] {telemetry}"
                    )

                    visualizer.process_telemetry(
                        telemetry
                    )

                    visualizer.draw()

                except json.JSONDecodeError:
                    # Ignore normal non-JSON serial output.
                    continue

    except serial.SerialException as error:

        print(
            f"[ERROR] Could not open serial port "
            f"{port}: {error}"
        )

        raise


def main():
    """Program entry point."""

    parser = argparse.ArgumentParser(
        description="Live mesh topology visualizer"
    )

    parser.add_argument(
        "--port",
        help="Serial port, for example COM3",
    )

    parser.add_argument(
        "--baudrate",
        type=int,
        default=115200,
        help="Serial baud rate",
    )

    parser.add_argument(
        "--timeout",
        type=int,
        default=10,
        help="Seconds before inactive nodes/links disappear",
    )

    parser.add_argument(
        "--demo",
        action="store_true",
        help="Run with simulated mesh telemetry",
    )

    args = parser.parse_args()

    # Create visualizer.
    visualizer = MeshVisualizer(
        timeout=args.timeout
    )

    # Enable interactive Matplotlib mode.
    plt.ion()

    plt.figure(
        figsize=(10, 7)
    )

    try:

        if args.demo:

            print(
                "[INFO] Starting mesh visualizer demo..."
            )

            demo_stream(
                visualizer
            )

        elif args.port:

            serial_stream(
                args.port,
                args.baudrate,
                visualizer,
            )

        else:

            parser.error(
                "Specify --port COMx or use --demo"
            )

    except KeyboardInterrupt:

        print(
            "\n[INFO] Visualizer stopped."
        )

    finally:

        plt.close("all")


if __name__ == "__main__":
    main()