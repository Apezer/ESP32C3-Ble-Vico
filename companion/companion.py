#!/usr/bin/env python3
"""Forward Vico status.json updates to an ESP32 serial port."""

from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path


PROJECT_DIR = Path(__file__).resolve().parents[2]
DEFAULT_STATUS_FILE = PROJECT_DIR / ".vico" / "status.json"


def import_serial():
    try:
        import serial
        from serial.tools import list_ports
    except ImportError:
        print("Missing dependency: pyserial. Install it with: pip install pyserial", file=sys.stderr)
        raise SystemExit(2)
    return serial, list_ports


def list_serial_ports() -> None:
    _, list_ports = import_serial()
    ports = list(list_ports.comports())
    if not ports:
        print("No serial ports found.")
        return

    for port in ports:
        print(f"{port.device}\t{port.description}")


def read_status(path: Path) -> dict | None:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        return None
    except json.JSONDecodeError as exc:
        print(f"Ignoring invalid status file: {exc}", file=sys.stderr)
        return None


def format_lines(status: dict) -> list[str]:
    state = str(status.get("state") or "READY").upper()
    message = str(status.get("message") or "")
    return [
        f"STATE {state}\n",
        f"TEXT {message}\n",
    ]


def run(port: str, baud: int, status_file: Path, interval: float) -> int:
    serial, _ = import_serial()
    last_mtime = None
    last_payload = None

    print(f"Opening {port} at {baud} baud")
    with serial.Serial(port, baudrate=baud, timeout=1) as ser:
        time.sleep(2.0)
        print(f"Watching {status_file}")

        while True:
            try:
                mtime = status_file.stat().st_mtime
            except FileNotFoundError:
                time.sleep(interval)
                continue

            if mtime != last_mtime:
                last_mtime = mtime
                payload = read_status(status_file)
                if payload and payload != last_payload:
                    last_payload = payload
                    for line in format_lines(payload):
                        ser.write(line.encode("utf-8"))
                    ser.flush()
                    print(f"Sent: {payload.get('state')} - {payload.get('message')}")

            time.sleep(interval)


def send_once(port: str, baud: int, status_file: Path) -> int:
    serial, _ = import_serial()
    payload = read_status(status_file)
    if not payload:
        print(f"No status found at {status_file}", file=sys.stderr)
        return 1

    print(f"Opening {port} at {baud} baud")
    with serial.Serial(port, baudrate=baud, timeout=1) as ser:
        time.sleep(2.0)
        for line in format_lines(payload):
            ser.write(line.encode("utf-8"))
        ser.flush()

    print(f"Sent once: {payload.get('state')} - {payload.get('message')}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Vico Keyboard companion MVP")
    parser.add_argument("--list", action="store_true", help="list available serial ports")
    parser.add_argument("--once", action="store_true", help="send current status once and exit")
    parser.add_argument("--port", help="serial port, for example COM5")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--status-file", type=Path, default=DEFAULT_STATUS_FILE)
    parser.add_argument("--interval", type=float, default=0.2)
    args = parser.parse_args()

    if args.list:
        list_serial_ports()
        return 0

    if not args.port:
        parser.error("--port is required unless --list is used")

    if args.once:
        return send_once(args.port, args.baud, args.status_file)

    return run(args.port, args.baud, args.status_file, args.interval)


if __name__ == "__main__":
    raise SystemExit(main())
