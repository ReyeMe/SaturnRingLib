function Fail($message) {
    [Console]::Error.WriteLine("")
    [Console]::Error.WriteLine("ERROR: $message")
    [Console]::Error.WriteLine("  - Is the Saturn powered on with the DevCart seated? (check that it boots)")
    [Console]::Error.WriteLine("  - Unplug/replug the DevCart USB cable, or try another port or cable.")
    [Console]::Error.WriteLine("  - Device Manager must list the DevCart (FT245R USB FIFO / USB Serial Converter).")
    exit 1
}

& "./../../tools/bin/win/usbreset/usbreset.cmd" "FT245R USB FIFO"
if ($LASTEXITCODE -ne 0) { Fail "DevCart (FT245R USB FIFO) not found on USB." }

Start-Sleep -Seconds 2
& "./../../tools/bin/win/ftx/ftx.exe" "-x" "./cd/data/0.bin" "0x06004000"
if ($LASTEXITCODE -ne 0) { Fail "Upload to the Saturn failed (ftx -x)." }

Start-Sleep -Seconds 1
