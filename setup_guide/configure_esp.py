#!/usr/bin/env python3
"""
Minimal serial console for XYZ-Switch (ESP32).

The ESP32 provides its own menu over Serial; this script just:
- lets you pick a serial port
- opens the connection
- forwards input/output between your terminal and the device

Exit: type `~.` on its own line, or press Ctrl+C.
"""

from __future__ import annotations

import argparse
import sys
import time
import threading

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    print("Missing dependency: pip install pyserial")
    sys.exit(1)

BAUD = 115200
READ_TIMEOUT = 0.5
WRITE_TIMEOUT = 2.0


def list_serial_ports():
    """Return list of (port, description) sorted by port name."""
    ports = list(list_ports.comports())
    ports.sort(key=lambda p: p.device)
    return [(p.device, p.description or p.device) for p in ports]


def choose_port() -> str | None:
    """Let user choose a serial port by number; 0 = refresh. Returns port string or None to exit."""
    while True:
        ports = list_serial_ports()
        if not ports:
            print("No serial ports found. Connect the device (use the right USB-C port) and try again.")
            print("Enter 0 to refresh, or q to quit: ", end="")
            choice = input().strip().lower()
            if choice == "q":
                return None
            continue
        print("\nAvailable serial ports:")
        for i, (port, desc) in enumerate(ports, 1):
            print(f"  {i}) {port}  — {desc}")
        print("  0) Refresh list")
        print("  q) Quit")
        try:
            raw = input("Select port number (or 0 / q): ").strip().lower()
        except (EOFError, KeyboardInterrupt):
            return None
        if raw == "q":
            return None
        if raw == "0":
            continue
        try:
            idx = int(raw)
            if 1 <= idx <= len(ports):
                return ports[idx - 1][0]
        except ValueError:
            pass
        print("Invalid choice.")


def open_serial(port: str, baud: int) -> "serial.Serial" | None:
    """Open serial port at BAUD. Returns serial object or None."""
    try:
        ser = serial.Serial(
            port=port,
            baudrate=baud,
            timeout=READ_TIMEOUT,
            write_timeout=WRITE_TIMEOUT,
        )
        return ser
    except serial.SerialException as e:
        print(f"Could not open {port}: {e}")
        return None


def _reader_loop(ser: "serial.Serial", stop: threading.Event) -> None:
    """Continuously read from serial and write to stdout (no extra formatting)."""
    while not stop.is_set():
        try:
            n = ser.in_waiting
            chunk = ser.read(n or 1)
        except serial.SerialException:
            stop.set()
            break

        if not chunk:
            continue

        try:
            text = chunk.decode("utf-8", errors="replace")
        except Exception:
            text = chunk.decode("latin-1", errors="replace")

        sys.stdout.write(text)
        sys.stdout.flush()


def run():
    parser = argparse.ArgumentParser(description="XYZ-Switch serial console (passthrough).")
    parser.add_argument("--port", help="Serial port (e.g. COM5). If omitted, you'll be prompted.")
    parser.add_argument("--baud", type=int, default=BAUD, help=f"Baud rate (default: {BAUD}).")
    args = parser.parse_args()

    print("XYZ-Switch serial console")
    print("Connect via the right USB-C port. If the device is sleeping, press the button to wake it.")
    print("Tip: press Enter to make the ESP show its menu.")
    print("Exit: type `~.` on its own line, or press Ctrl+C.\n")

    port = args.port or choose_port()
    if not port:
        print("Bye.")
        return

    ser = open_serial(port, args.baud)
    if not ser:
        return
    try:
        print(f"\nOpened {port} at {args.baud} baud.\n")
        stop = threading.Event()
        reader = threading.Thread(target=_reader_loop, args=(ser, stop), daemon=True)
        reader.start()

        # Give the device a nudge so you see something if it's already awake.
        try:
            ser.write(b"\n")
            ser.flush()
        except serial.SerialException:
            return

        while not stop.is_set():
            try:
                line = sys.stdin.readline()
            except KeyboardInterrupt:
                break
            if line == "":
                break
            if line.rstrip("\r\n") == "~.":
                break
            try:
                ser.write(line.encode("utf-8", errors="replace"))
                ser.flush()
            except serial.SerialException:
                break
    finally:
        try:
            stop.set()
        except Exception:
            pass
        ser.close()
    print("Done.")


if __name__ == "__main__":
    run()
