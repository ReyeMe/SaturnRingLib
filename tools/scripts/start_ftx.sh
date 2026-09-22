#!/usr/bin/env bash

set -e

if pgrep -f "^ftx -g" > /dev/null; then
    if [[ "$(uname -s)" == Darwin* ]]; then
        if ! nc -z localhost 1234 > /dev/null 2>&1; then
            pkill -9 -f "^ftx -g" || true
            sleep 0.5
        fi
    else
        if ! (echo > /dev/tcp/localhost/1234) 2>/dev/null; then
            pkill -9 -f "^ftx -g" || true
            sleep 0.5
        fi
    fi
fi

if ! pgrep -f "^ftx -g" > /dev/null; then
    nohup ftx -g 1234 -v > /tmp/ftx.log 2>&1 &
    sleep 1
    if [[ "$(uname -s)" == Darwin* ]]; then
        perl -e 'alarm 8; exec @ARGV' gdb-multiarch -batch -ex "target remote localhost:1234" -ex "detach" -ex "quit" ./BuildDrop/Debug_GDBStub.elf > /dev/null 2>&1 || true
    else
        timeout 8 gdb-multiarch -batch -ex "target remote localhost:1234" -ex "detach" -ex "quit" ./BuildDrop/Debug_GDBStub.elf > /dev/null 2>&1 || true
    fi
fi