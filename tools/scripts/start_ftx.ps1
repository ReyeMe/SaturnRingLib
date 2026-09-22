$listener = Get-NetTCPConnection -LocalPort 1234 -State Listen -ErrorAction SilentlyContinue
if (-not $listener) {
    Start-Process -FilePath "./../../tools/bin/win/ftx/ftx.exe" -ArgumentList "-g", "1234", "-v" -WindowStyle Hidden
    Start-Sleep -Seconds 1
    & "./../../tools/bin/win/gdb-multiarch.exe" "-batch" "-ex" "target remote localhost:1234" "-ex" "detach" "-ex" "quit" "./BuildDrop/Debug_GDBStub.elf"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}