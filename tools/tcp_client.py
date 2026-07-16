#!/usr/bin/env python3
"""Minimal TCP client: print live JSON from STM32/ESP, optional history request."""
from __future__ import annotations

import argparse
import json
import socket
import sys


def main() -> int:
    p = argparse.ArgumentParser(description="TCP client for data acquisition JSON stream")
    p.add_argument("host", help="ESP-01S IP address")
    p.add_argument("port", nargs="?", type=int, default=8080, help="TCP port (default 8080)")
    p.add_argument("--history", metavar="FROM,TO", help='Request history e.g. "0,20"')
    args = p.parse_args()

    sock = socket.create_connection((args.host, args.port), timeout=10.0)
    sock.settimeout(60.0)

    if args.history:
        try:
            a, b = args.history.split(",", 1)
            frm, to = int(a.strip()), int(b.strip())
        except ValueError:
            print("bad --history, use FROM,TO e.g. 0,20", file=sys.stderr)
            return 2
        line = json.dumps({"cmd": "history", "from": frm, "to": to}) + "\n"
        sock.sendall(line.encode())

    buf = b""
    try:
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            buf += chunk
            while b"\n" in buf:
                line, buf = buf.split(b"\n", 1)
                if line:
                    print(line.decode(errors="replace"))
    except KeyboardInterrupt:
        pass
    finally:
        sock.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
