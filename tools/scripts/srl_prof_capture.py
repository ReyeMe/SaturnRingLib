#!/usr/bin/env python3
"""Records the profiler reports a Saturn streams over the DevCart (SRL_PROFILER_REPORT_FRAMES > 0).

Start the relay first, `ftx -g 1234`, then:
    srl_prof_capture.py capture.bin --seconds 10
and read the result with srl_prof.py. Only one client can use ftx -g at a time:
disconnect GDB first (the Saturn doesn't stream while GDB is connected anyway).
"""

import argparse
import socket
import time


def capture(host, port, seconds):
    """Returns every byte received from ftx -g during `seconds`."""
    data = bytearray()
    with socket.create_connection((host, port), timeout=5) as sock:
        sock.settimeout(0.2)
        deadline = time.time() + seconds
        while time.time() < deadline:
            try:
                chunk = sock.recv(65536)
            except socket.timeout:
                continue
            if not chunk:
                break  # ftx closed the connection
            data += chunk
    return bytes(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("output", help="file to write the raw stream to")
    parser.add_argument("--seconds", type=float, default=10.0)
    parser.add_argument("--host", default="localhost")
    parser.add_argument("--port", type=int, default=1234)
    args = parser.parse_args()

    data = capture(args.host, args.port, args.seconds)
    with open(args.output, "wb") as f:
        f.write(data)
    print(f"wrote {len(data)} bytes to {args.output}")


if __name__ == "__main__":
    main()
