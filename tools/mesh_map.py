import argparse
import json
import serial
import networkx as nx
import matplotlib.pyplot as plt


class MeshMap:
    def __init__(self):
        self.graph = nx.Graph()

    def parse_telemetry(self, line):
        try:
            data = json.loads(line)

            sender_id = data["sender_id"]
            receiver_id = data["receiver_id"]
            rssi = data["rssi"]

            return sender_id, receiver_id, rssi

        except (json.JSONDecodeError, KeyError, TypeError, ValueError):
            return None

    def update_graph(self, sender_id, receiver_id, rssi):
        self.graph.add_node(sender_id)
        self.graph.add_node(receiver_id)

        self.graph.add_edge(
            sender_id,
            receiver_id,
            rssi=rssi
        )

    def get_edge_color(self, rssi):
        if rssi >= -60:
            return "green"
        elif rssi >= -80:
            return "orange"
        else:
            return "red"

    def draw_graph(self):
        plt.clf()

        if not self.graph.nodes:
            plt.title("Mesh Topology - Waiting for telemetry...")
            plt.pause(0.1)
            return

        pos = nx.spring_layout(
            self.graph,
            seed=42
        )

        edge_colors = []

        for sender, receiver, data in self.graph.edges(data=True):
            rssi = data.get("rssi", -100)
            edge_colors.append(
                self.get_edge_color(rssi)
            )

        nx.draw(
            self.graph,
            pos,
            with_labels=True,
            node_size=1200,
            node_color="lightblue",
            edge_color=edge_colors,
            font_weight="bold"
        )

        edge_labels = {}

        for sender, receiver, data in self.graph.edges(data=True):
            rssi = data.get("rssi", -100)
            edge_labels[(sender, receiver)] = f"{rssi} dBm"

        nx.draw_networkx_edge_labels(
            self.graph,
            pos,
            edge_labels=edge_labels
        )

        plt.title("Live Mesh Network Topology")
        plt.pause(0.1)

    def run_test(self):
        print("Running mesh topology test...")

        test_data = [
            {
                "sender_id": 4097,
                "receiver_id": 4098,
                "rssi": -45
            },
            {
                "sender_id": 4098,
                "receiver_id": 4099,
                "rssi": -65
            },
            {
                "sender_id": 4097,
                "receiver_id": 4099,
                "rssi": -85
            }
        ]

        plt.ion()

        for telemetry in test_data:
            line = json.dumps(telemetry)

            result = self.parse_telemetry(line)

            if result:
                sender_id, receiver_id, rssi = result

                print(
                    f"Telemetry: "
                    f"{sender_id} -> {receiver_id}, "
                    f"RSSI = {rssi} dBm"
                )

                self.update_graph(
                    sender_id,
                    receiver_id,
                    rssi
                )

                self.draw_graph()

        print("\nTest data processed successfully.")
        print(f"Nodes: {self.graph.number_of_nodes()}")
        print(f"Connections: {self.graph.number_of_edges()}")

        plt.ioff()
        plt.show()

    def monitor_serial(self, port, baudrate):
        try:
            ser = serial.Serial(
                port,
                baudrate,
                timeout=1
            )

            print(
                f"Connected to {port} "
                f"at {baudrate} baud."
            )

        except serial.SerialException as e:
            print(f"Serial connection error: {e}")
            return

        plt.ion()

        try:
            while True:
                if ser.in_waiting:
                    line = (
                        ser.readline()
                        .decode(
                            "utf-8",
                            errors="ignore"
                        )
                        .strip()
                    )

                    result = self.parse_telemetry(line)

                    if result:
                        sender_id, receiver_id, rssi = result

                        self.update_graph(
                            sender_id,
                            receiver_id,
                            rssi
                        )

                        self.draw_graph()

        except KeyboardInterrupt:
            print("\nMonitoring stopped.")

        finally:
            ser.close()
            plt.ioff()
            plt.show()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Live Mesh Network Topology Visualizer"
    )

    parser.add_argument(
        "--port",
        help="Serial port, for example COM3"
    )

    parser.add_argument(
        "--baudrate",
        type=int,
        default=115200,
        help="Serial baud rate"
    )

    parser.add_argument(
        "--test",
        action="store_true",
        help="Run visualizer using sample telemetry"
    )

    args = parser.parse_args()

    mesh_map = MeshMap()

    if args.test:
        mesh_map.run_test()

    elif args.port:
        mesh_map.monitor_serial(
            args.port,
            args.baudrate
        )

    else:
        parser.error(
            "Please provide --test or --port"
        )