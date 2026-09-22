& "./../../tools/bin/win/usbreset/usbreset.cmd" "FT245R USB FIFO"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Start-Sleep -Seconds 2
& "./../../tools/bin/win/ftx/ftx.exe" "-x" "./cd/data/0.bin" "0x06004000"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Start-Sleep -Seconds 1