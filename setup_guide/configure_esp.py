#!/usr/bin/env python3
"""
Configure the XYZ-Switch ESP32 over serial.
List serial ports (0 = refresh), then drive the device menu to set API key,
WiFi, select Govee devices, or reset.
"""

import sys
import time

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    print("Missing dependency: pip install pyserial")
    sys.exit(1)

BAUD = 115200
READ_TIMEOUT = 0.5
WRITE_TIMEOUT = 2.0
POST_SEND_WAIT = 2.0   # seconds to read after sending a command


def list_serial_ports():
    """Return list of (port, description) sorted by port name."""
    ports = list(list_ports.comports())
    ports.sort(key=lambda p: p.device)
    return [(p.device, p.description or p.device) for p in ports]


def choose_port():
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


def open_serial(port):
    """Open serial port at BAUD. Returns serial object or None."""
    try:
        ser = serial.Serial(
            port=port,
            baudrate=BAUD,
            timeout=READ_TIMEOUT,
            write_timeout=WRITE_TIMEOUT,
        )
        return ser
    except serial.SerialException as e:
        print(f"Could not open {port}: {e}")
        return None


def read_until_prompt(ser, timeout_sec=5.0, silent=False):
    """Read and optionally print lines until we see a prompt (e.g. '> '). Returns collected text."""
    start = time.monotonic()
    buf = []
    while (time.monotonic() - start) < timeout_sec:
        line = ser.readline()
        if not line:
            time.sleep(0.05)
            continue
        try:
            text = line.decode("utf-8", errors="replace").rstrip()
        except Exception:
            text = line.decode("latin-1", errors="replace").rstrip()
        buf.append(text)
        if not silent:
            print(text)
        if "> " in text or "0=back" in text or "Password> " in text or "API key> " in text:
            break
    return "\n".join(buf)


def send_and_wait(ser, s, wait_sec=POST_SEND_WAIT, silent=False):
    """Send string (newline appended if missing), then read and print for wait_sec."""
    if not s.endswith("\n"):
        s = s + "\n"
    try:
        ser.write(s.encode("utf-8"))
        ser.flush()
    except serial.SerialException as e:
        print(f"Write error: {e}")
        return ""
    time.sleep(0.1)
    return read_until_prompt(ser, timeout_sec=wait_sec, silent=silent)


def wake_menu(ser):
    """Send Enter to open the device menu. Returns True if we see menu-like output."""
    send_and_wait(ser, "\n", wait_sec=3.0)
    return True


def main_menu(ser):
    """Show our menu and handle choice. Returns False to exit."""
    print("\n--- Configuration ---")
    print("  1) Set Govee API key")
    print("  2) List Govee devices")
    print("  3) Select / deselect devices (which lights to toggle)")
    print("  4) WiFi setup")
    print("  5) Reset all (WiFi, API key, devices)")
    print("  6) Raw serial (type to device; type '---' alone to return)")
    print("  0) Exit")
    try:
        choice = input("Choice: ").strip()
    except (EOFError, KeyboardInterrupt):
        return False
    if choice == "0":
        return False
    if choice == "1":
        send_and_wait(ser, "\n", wait_sec=2.0)
        send_and_wait(ser, "1", wait_sec=2.0)
        api_key = input("Enter API key: ").strip()
        if api_key:
            send_and_wait(ser, api_key, wait_sec=2.0)
        return True
    if choice == "2":
        send_and_wait(ser, "\n", wait_sec=2.0)
        send_and_wait(ser, "2", wait_sec=8.0)
        return True
    if choice == "3":
        send_and_wait(ser, "\n", wait_sec=2.0)
        send_and_wait(ser, "3", wait_sec=10.0)
        while True:
            try:
                num = input("Device number to toggle (or 0 to go back): ").strip()
            except (EOFError, KeyboardInterrupt):
                break
            send_and_wait(ser, num, wait_sec=10.0)
            if num == "0":
                break
        return True
    if choice == "4":
        send_and_wait(ser, "\n", wait_sec=2.0)
        send_and_wait(ser, "4", wait_sec=8.0)
        ssid_or_num = input("Enter network number, or type SSID: ").strip()
        if not ssid_or_num or ssid_or_num == "0":
            send_and_wait(ser, "0", wait_sec=2.0)
            return True
        send_and_wait(ser, ssid_or_num, wait_sec=2.0)
        pwd = input("Password: ").strip()
        send_and_wait(ser, pwd, wait_sec=18.0)
        return True
    if choice == "5":
        send_and_wait(ser, "\n", wait_sec=2.0)
        send_and_wait(ser, "5", wait_sec=2.0)
        confirm = input("Type YES to confirm reset: ").strip()
        send_and_wait(ser, confirm, wait_sec=3.0)
        return True
    if choice == "6":
        print("Raw serial. Type to send; '---' alone returns to menu.")
        while True:
            try:
                line = input()
            except (EOFError, KeyboardInterrupt):
                break
            if line.strip() == "---":
                break
            send_and_wait(ser, line, wait_sec=1.0, silent=False)
        return True
    print("Unknown option.")
    return True


def run():
    print("XYZ-Switch configuration tool")
    print("Make sure the device is awake (press the button if needed) and connected via the right USB-C port.\n")

    port = choose_port()
    if not port:
        print("Bye.")
        return
    ser = open_serial(port)
    if not ser:
        return
    try:
        print(f"\nOpened {port} at {BAUD} baud.")
        print("Waking device menu... (if nothing happens, press the button on the device and try again.)")
        wake_menu(ser)
        while main_menu(ser):
            pass
    finally:
        ser.close()
    print("Done.")


if __name__ == "__main__":
    run()
