& "$PSScriptRoot/start_ftx.ps1"
& "./../../tools/bin/win/gdb-multiarch.exe" "./BuildDrop/Debug_GDBStub.elf" "-ex" "target remote localhost:1234"