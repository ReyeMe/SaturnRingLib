#!/usr/bin/env bash

set -e

bash ./../../tools/scripts/start_ftx.sh ./BuildDrop/Debug_GDBStub.elf
exec gdb-multiarch ./BuildDrop/Debug_GDBStub.elf -ex "target remote localhost:1234"