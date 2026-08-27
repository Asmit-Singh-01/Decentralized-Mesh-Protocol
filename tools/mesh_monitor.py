import sys
import time
import re

print("==================================================")
print("   SWARM MESH PROTOCOL - LIVE MONITORING CLI      ")
print("==================================================")

def parse_line(line):
    line = line.strip()
    if "[SIM]" in line or "[SYSTEM]" in line:
        print(f"\033[94m{time.strftime('%H:%M:%S')} {line}\033[0m")
    elif "[STATUS]" in line or "[SUCCESS]" in line:
        print(f"\033[92m{time.strftime('%H:%M:%S')} {line}\033[0m")
    elif "ERROR" in line or "FAIL" in line:
        print(f"\033[91m{time.strftime('%H:%M:%S')} {line}\033[0m")
    else:
        print(f"{time.strftime('%H:%M:%S')} | {line}")

if __name__ == "__main__":
    print("[INFO] Listening on virtual mesh interface...")
    print("[INFO] Press Ctrl+C to stop.\n")
    try:
        while True:
            # Mocking live packet stream telemetry check
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n[INFO] Monitor shut down safely.")
  
