#!/usr/bin/env bash
# Tests/GDB/run_gdb_tests.sh
#
# Host-side integration test runner for the SRL GDB stub (srl_gdbstub.hpp).
# Builds the "Debug - GDB Stub" sample (reused as-is: it already provides a
# hardware-validated set of breakpoint/watchpoint/exception/slave trigger
# points -- see its own readme.md), then drives a real gdb-multiarch client
# against it through gdb_protocol_tests.py. The runner reloads the target
# (power-cycle, upload, fresh ftx -g proxy) before EVERY test, so each test
# starts from a freshly booted console.
#
# Requires: real Saturn hardware + USB DevCart (ftx, usbreset, gdb-multiarch,
# python3 all on PATH -- e.g. inside this repo's docker toolchain container).
# This does NOT run under the emulator: srl_gdbstub.hpp's transport is the
# physical DevCart serial link, which Kronos/mednafen don't expose.
#
# Usage: ./run_gdb_tests.sh [DEVICE_IP] [gdb_protocol_tests.py options...]
#   DEVICE_IP  optional IPv4 of a network PSU REST API (relay_status/on/off,
#              matching ../run_tests.bat's USBGamers mode) used to power-cycle
#              the console before every test. Without it, each test only
#              re-uploads the sample.
#   e.g.       ./run_gdb_tests.sh 192.168.1.50 --filter scenario

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SAMPLE_DIR="$SCRIPT_DIR/../../Samples/Debug - GDB Stub"
ELF_PATH="$SAMPLE_DIR/BuildDrop/Debug_GDBStub.elf"
BIN_PATH="$SAMPLE_DIR/cd/data/0.bin"
LOG_FILE="$SCRIPT_DIR/gdb_uts.log"

PSU_ARGS=()
if [[ "${1:-}" =~ ^[0-9]+(\.[0-9]+){3}$ ]]; then
  PSU_ARGS=(--psu-ip "$1")
  shift
fi

echo "=== Building Debug - GDB Stub sample ==="
(cd "$SAMPLE_DIR" && make) || { echo "Build failed"; exit 1; }

echo "=== Running GDB protocol test suite (target reloaded before every test) ==="
python3 "$SCRIPT_DIR/gdb_protocol_tests.py" "$ELF_PATH" --port 1234 --log "$LOG_FILE" \
  --bin "$BIN_PATH" --proxy-log "$SCRIPT_DIR/ftx_gdb.log" "${PSU_ARGS[@]}" "$@"
STATUS=$?

echo "Log written to $LOG_FILE"
exit $STATUS
