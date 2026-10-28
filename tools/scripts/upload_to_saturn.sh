#!/usr/bin/env bash

set -e

fail() {
    echo "" >&2
    echo "ERROR: $1" >&2
    echo "  - Is the Saturn powered on with the DevCart seated? (check that it boots)" >&2
    echo "  - Unplug/replug the DevCart USB cable, or try another port or cable." >&2
    echo "  - 'lsusb | grep 0403' must list an FTDI device; 'dmesg | tail' shows USB errors." >&2
    exit 1
}

if [[ "$(uname -s)" == Darwin* ]]; then
    usbreset 0x0403:0x6001 || echo 'usbreset failed (may need sudo); continuing'
else
    usbreset "FT245R USB FIFO" || fail "DevCart (FT245R USB FIFO) not found on USB."
fi

sleep 2
ftx -x ./cd/data/0.bin 0x06004000 || fail "Upload to the Saturn failed (ftx -x)."
sleep 1
