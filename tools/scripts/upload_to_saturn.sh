#!/usr/bin/env bash

set -e

if [[ "$(uname -s)" == Darwin* ]]; then
    usbreset 0x0403:0x6001 || echo 'usbreset failed (may need sudo); continuing'
else
    usbreset "FT245R USB FIFO"
fi

sleep 2
ftx -x ./cd/data/0.bin 0x06004000
sleep 1