#!/usr/bin/env bash
# Tests/GDB/run_gdb_tests.sh
#
# Host-side integration test runner for the SRL GDB stub (srl_gdbstub.hpp).
# Builds and uploads the "Debug - GDB Stub" sample (reused as-is: it already
# provides a hardware-validated set of breakpoint/watchpoint/exception/slave
# trigger points -- see its own readme.md), then drives a real gdb-multiarch
# client against it through gdb_protocol_tests.py, covering every RSP command
# the stub implements plus realistic command combinations.
#
# Requires: real Saturn hardware + USB DevCart (ftx, usbreset, gdb-multiarch,
# python3 all on PATH -- e.g. inside this repo's docker toolchain container).
# This does NOT run under the emulator: srl_gdbstub.hpp's transport is the
# physical DevCart serial link, which Kronos/mednafen don't expose.
#
# Usage: ./run_gdb_tests.sh [DEVICE_IP]
#   DEVICE_IP  optional IPv4 of a network PSU REST API (relay_status/on/off/
#              toggle, matching ../run_tests.bat's USBGamers mode) used to
#              power-cycle the console before the run. Omit to reuse
#              whatever's already running.

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SAMPLE_DIR="$SCRIPT_DIR/../../Samples/Debug - GDB Stub"
ELF_PATH="$SAMPLE_DIR/BuildDrop/Debug_GDBStub.elf"
BIN_PATH="$SAMPLE_DIR/cd/data/0.bin"
LOAD_ADDR=0x06004000
GDB_PORT=1234
LOG_FILE="$SCRIPT_DIR/gdb_uts.log"

DEVICE_IP="${1:-}"

power_cycle() {
  local ip="$1"
  echo "Power-cycling target at $ip..."
  curl -s -X POST "http://$ip/api/v1/off" > /dev/null
  sleep 6
  curl -s -X POST "http://$ip/api/v1/on" > /dev/null
  sleep 12
}

echo "=== Building Debug - GDB Stub sample ==="
(cd "$SAMPLE_DIR" && make) || { echo "Build failed"; exit 1; }

if [ -n "$DEVICE_IP" ]; then
  power_cycle "$DEVICE_IP"
fi

echo "=== Uploading to Saturn ==="
usbreset "FT245R USB FIFO" || { echo "usbreset failed"; exit 1; }
sleep 1
if ! ftx -x "$BIN_PATH" "$LOAD_ADDR"; then
  echo "First upload attempt failed, retrying after a fresh usbreset..."
  usbreset "FT245R USB FIFO"
  sleep 2
  ftx -x "$BIN_PATH" "$LOAD_ADDR" || { echo "Upload failed"; exit 1; }
fi

echo "=== Starting ftx GDB proxy on port $GDB_PORT ==="
pkill -9 -f "^ftx -g $GDB_PORT" 2>/dev/null
sleep 1
ftx -g "$GDB_PORT" -v > "$SCRIPT_DIR/ftx_gdb.log" 2>&1 &
FTX_PID=$!
sleep 2

cleanup() {
  kill -9 "$FTX_PID" 2>/dev/null
}
trap cleanup EXIT

echo "=== Running GDB protocol test suite ==="
python3 "$SCRIPT_DIR/gdb_protocol_tests.py" "$ELF_PATH" --port "$GDB_PORT" --log "$LOG_FILE"
STATUS=$?

echo "Log written to $LOG_FILE"
exit $STATUS
