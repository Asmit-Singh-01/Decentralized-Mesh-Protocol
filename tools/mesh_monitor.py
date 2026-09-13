import sys
import time
import re
import json
import argparse
from datetime import datetime

class MeshMonitor:
    def __init__(self, log_file=None):
        self.log_file = log_file
        self.stats = {"total_packets": 0, "errors": 0, "success": 0}
        
    def parse_line(self, line):
        line = line.strip()
        if not line:
            return

        timestamp = datetime.now().strftime('%H:%M:%S')
        log_entry = {"timestamp": timestamp, "message": line, "type": "INFO"}
        self.stats["total_packets"] += 1

        if "[SIM]" in line or "[SYSTEM]" in line:
            log_entry["type"] = "SYSTEM"
            print(f"\033[94m[{timestamp}] {line}\033[0m")
        elif "[STATUS]" in line or "[SUCCESS]" in line:
            log_entry["type"] = "SUCCESS"
            self.stats["success"] += 1
            print(f"\033[92m[{timestamp}] {line}\033[0m")
        elif "ERROR" in line or "FAIL" in line:
            log_entry["type"] = "ERROR"
            self.stats["errors"] += 1
            print(f"\033[91m[{timestamp}] {line}\033[0m")
        else:
            print(f"[{timestamp}] | {line}")

        if self.log_file:
            self._save_to_log(log_entry)

    def _save_to_log(self, data):
        try:
            with open(self.log_file, "a") as f:
                f.write(json.dumps(data) + "\n")
        except Exception as e:
            print(f"\033[91m[LOG ERROR] Could not save log: {e}\033[0m")

    def display_summary(self):
        print("\n==================================================")
        print("          MESH TELEMETRY SUMMARY REPORT           ")
        print("==================================================")
        print(f" Total Packets Processed : {self.stats['total_packets']}")
        print(f" Successful Transmissions: {self.stats['success']}")
        print(f" Packet Errors / Failures : {self.stats['errors']}")
        print("==================================================")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Swarm Mesh Protocol - Advanced CLI Monitor")
    parser.add_argument("--log-file", type=str, default="mesh_telemetry.json", help="Path to save JSON output logs")
    args = parser.parse_args()

    print("==================================================")
    print("   SWARM MESH PROTOCOL - LIVE MONITORING CLI v2.0 ")
    print("==================================================")
    print(f"[INFO] Logging active stream to: {args.log-file}")
    print("[INFO] Listening on virtual mesh interface...")
    print("[INFO] Press Ctrl+C to stop.\n")

    monitor = MeshMonitor(log_file=args.log_file)
    try:
        while True:
            # Mocking live packet stream telemetry check
            time.sleep(1)
    except KeyboardInterrupt:
        monitor.display_summary()
        print("\n[INFO] Monitor shut down safely.")
        
