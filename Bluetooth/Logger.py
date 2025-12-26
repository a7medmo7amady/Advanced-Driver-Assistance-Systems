"""
ADAS Bluetooth Data Logger
==========================

This script receives telemetry data from the Arduino ADAS system
via Bluetooth (HC-05 module) and logs it to a CSV file.

Requirements:
- pip install pyserial

Usage:
1. Pair your computer with the HC-05 module (PIN: 1234 or 0000)
2. Find the COM port (Windows) or /dev/rfcomm0 (Linux)
3. Run this script
4. Data will be logged to adas_log.csv

On Windows:
- Go to Bluetooth settings > More Bluetooth options > COM Ports
- Note the "Outgoing" COM port number (e.g., COM6)

On Linux:
- sudo rfcomm bind 0 XX:XX:XX:XX:XX:XX  (HC-05 MAC address)
- Use /dev/rfcomm0
"""

import serial
import csv
import datetime
import os
import sys
import time

# ============== CONFIGURATION ==============

# Windows: "COM6" (check Device Manager > Ports)
# Linux: "/dev/rfcomm0"
# macOS: "/dev/tty.HC-05" or similar
BLUETOOTH_PORT = "COM6"
BAUD_RATE = 9600

# Log file
LOG_FILE = "adas_log.csv"

# ============================================


def find_available_ports():
    """List available serial ports"""
    import serial.tools.list_ports
    ports = serial.tools.list_ports.comports()
    return [p.device for p in ports]


def main():
    print("=" * 50)
    print("ADAS Bluetooth Data Logger")
    print("=" * 50)
    
    # Show available ports
    available = find_available_ports()
    print(f"\nAvailable ports: {available}")
    print(f"Configured port: {BLUETOOTH_PORT}")
    
    if BLUETOOTH_PORT not in available:
        print(f"\nWARNING: {BLUETOOTH_PORT} not found!")
        print("Edit BLUETOOTH_PORT in this script to match your system.")
        print("\nOn Windows:")
        print("  1. Open 'Bluetooth & other devices' settings")
        print("  2. Click 'More Bluetooth options' (right side)")
        print("  3. Go to 'COM Ports' tab")
        print("  4. Note the Outgoing port for HC-05")
        response = input("\nTry to connect anyway? (y/n): ")
        if response.lower() != 'y':
            return
    
    # Connect to Bluetooth
    print(f"\nConnecting to {BLUETOOTH_PORT}...")
    try:
        bt = serial.Serial(BLUETOOTH_PORT, BAUD_RATE, timeout=1)
        print("Connected successfully!")
    except serial.SerialException as e:
        print(f"ERROR: Could not connect: {e}")
        print("\nMake sure:")
        print("  1. HC-05 is paired with your computer")
        print("  2. No other program is using the port")
        print("  3. The correct COM port is specified")
        return
    
    # Open log file
    file_exists = os.path.exists(LOG_FILE)
    log_file = open(LOG_FILE, 'a', newline='')
    csv_writer = csv.writer(log_file)
    
    if not file_exists:
        # Write header for new file
        csv_writer.writerow([
            'Timestamp', 'ArduinoTime', 'Speed%', 'Distance_cm', 
            'TTC', 'Brake', 'Lights', 'Belt1', 'Belt2', 
            'Door1', 'Door2', 'State'
        ])
    
    print(f"\nLogging to: {LOG_FILE}")
    print("\nReceiving data... (Press Ctrl+C to stop)\n")
    print("-" * 70)
    
    line_count = 0
    
    try:
        while True:
            if bt.in_waiting > 0:
                try:
                    line = bt.readline().decode('utf-8', errors='ignore').strip()
                except UnicodeDecodeError:
                    continue
                
                if not line:
                    continue
                
                # Skip header lines from Arduino
                if line.startswith("TIME,") or line.startswith("ADAS") or line.startswith("==="):
                    print(f"[HEADER] {line}")
                    continue
                
                # Parse data line
                parts = line.split(',')
                
                if len(parts) >= 10:
                    timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                    
                    # Add timestamp to data
                    row = [timestamp] + parts
                    csv_writer.writerow(row)
                    log_file.flush()
                    
                    line_count += 1
                    
                    # Pretty print
                    try:
                        arduino_time = int(parts[0]) / 1000  # ms to seconds
                        speed = parts[1]
                        distance = parts[2]
                        ttc = parts[3]
                        brake = "BRAKE" if parts[4] == "1" else ""
                        lights = "LIGHTS" if parts[5] == "1" else ""
                        state = parts[10] if len(parts) > 10 else ""
                        
                        print(f"[{line_count:4d}] Speed:{speed:>3}% | Dist:{distance:>3}cm | "
                              f"TTC:{ttc:>5}s | {brake:5} | {lights:6} | {state}")
                    except (ValueError, IndexError):
                        print(f"[RAW] {line}")
                else:
                    print(f"[MSG] {line}")
            
            time.sleep(0.01)  
            
    except KeyboardInterrupt:
        print("\n\nStopping...")
    finally:
        bt.close()
        log_file.close()
        print(f"\nLogged {line_count} data points to {LOG_FILE}")
        print("Done!")


if __name__ == "__main__":
    main()