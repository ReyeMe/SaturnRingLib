param(
    [string]$ElfFile,
    [string]$Ip = "localhost",
    [int]$Port = 1234
)

if (-not $ElfFile) {
    Write-Error "Usage: start_ftx.ps1 <ElfFile> [Ip=localhost] [Port=1234]"
    exit 1
}

if (-not (Test-Path -LiteralPath $ElfFile -PathType Leaf)) {
    Write-Error "ELF file not found: $ElfFile"
    exit 1
}

if ($Port -lt 1 -or $Port -gt 65535) {
    Write-Error "Invalid port: $Port"
    exit 1
}

$listener = Get-NetTCPConnection -LocalPort $Port -State Listen -ErrorAction SilentlyContinue
if (-not $listener) {
    Start-Process -FilePath "./../../tools/bin/win/ftx/ftx.exe" -ArgumentList "-g", "$Port", "-v" -WindowStyle Hidden
    Start-Sleep -Seconds 1
    & "./../../tools/bin/win/gdb-multiarch.exe" "-batch" "-ex" "target remote ${Ip}:${Port}" "-ex" "detach" "-ex" "quit" "$ElfFile"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
