#!/usr/bin/env bash

set -e

usage() {
    echo "Usage: $(basename "$0") <elf_file> [port=1234]" >&2
    exit 1
}

if [[ $# -lt 1 || $# -gt 2 ]]; then
    usage
fi

ELF_FILE="$1"
# This script starts (and health-checks) the ftx on THIS machine, so it only ever talks to localhost.
HOST=localhost
PORT="${2:-1234}"

if [[ ! -f "$ELF_FILE" ]]; then
    echo "Error: ELF file not found: $ELF_FILE" >&2
    exit 1
fi

if ! [[ "$PORT" =~ ^[0-9]+$ ]] || (( PORT < 1 || PORT > 65535 )); then
    echo "Error: invalid port: $PORT" >&2
    exit 1
fi

FTX_PATTERN="^ftx -g ${PORT}( |$)"

if pgrep -f "$FTX_PATTERN" > /dev/null; then
    if [[ "$(uname -s)" == Darwin* ]]; then
        if ! nc -z "$HOST" "$PORT" > /dev/null 2>&1; then
            pkill -9 -f "$FTX_PATTERN" || true
            sleep 0.5
        fi
    else
        if ! (echo > "/dev/tcp/$HOST/$PORT") 2>/dev/null; then
            pkill -9 -f "$FTX_PATTERN" || true
            sleep 0.5
        fi
    fi
fi

if ! pgrep -f "$FTX_PATTERN" > /dev/null; then
    nohup ftx -g "$PORT" -v > /tmp/ftx.log 2>&1 &
    sleep 1
    if [[ "$(uname -s)" == Darwin* ]]; then
        perl -e 'alarm 8; exec @ARGV' gdb-multiarch -batch -ex "target remote $HOST:$PORT" -ex "detach" -ex "quit" "$ELF_FILE" > /dev/null 2>&1 || true
    else
        timeout 8 gdb-multiarch -batch -ex "target remote $HOST:$PORT" -ex "detach" -ex "quit" "$ELF_FILE" > /dev/null 2>&1 || true
    fi
fi
