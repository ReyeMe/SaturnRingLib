param(
    [string]$ElfFile,
    [string]$Port = "1234"
)

function Usage {
    [Console]::Error.WriteLine("Usage: start_ftx.ps1 <ElfFile> [Port=1234]")
    exit 1
}

if (-not $ElfFile) { Usage }

if (-not (Test-Path -LiteralPath $ElfFile -PathType Leaf)) {
    [Console]::Error.WriteLine("Error: ELF file not found: $ElfFile")
    exit 1
}

if ($Port -notmatch '^\d+$' -or [int]$Port -lt 1 -or [int]$Port -gt 65535) {
    [Console]::Error.WriteLine("Error: invalid port: $Port")
    exit 1
}

# This script starts (and health-checks) the ftx on THIS machine, so it only ever talks to localhost.
$TargetHost = "localhost"
$FtxExe = (Resolve-Path "$PSScriptRoot/../bin/win/ftx/ftx.exe").Path
$GdbExe = (Resolve-Path "$PSScriptRoot/../bin/win/gdb-multiarch.exe").Path

# The ftx proxy already serving this port, if any.
function Get-Ftx {
    Get-CimInstance Win32_Process -Filter "Name = 'ftx.exe'" |
        Where-Object { $_.CommandLine -match "\s-g\s+$Port(\s|$)" }
}

function Test-Port {
    $client = New-Object System.Net.Sockets.TcpClient
    try { return $client.ConnectAsync($TargetHost, [int]$Port).Wait(1000) } catch { return $false } finally { $client.Dispose() }
}

# A running ftx that no longer accepts connections is stale: kill it, or it keeps the DevCart busy.
if ((Get-Ftx) -and -not (Test-Port)) {
    Get-Ftx | ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }
    Start-Sleep -Milliseconds 500
}

if (-not (Get-Ftx)) {
    $log = Join-Path $env:TEMP "ftx.log"
    Start-Process -FilePath "cmd.exe" -ArgumentList "/c `"`"$FtxExe`" -g $Port -v > `"$log`" 2>&1`"" -WindowStyle Hidden
    Start-Sleep -Seconds 1
    # Connect once and detach; a failure or hang here is not fatal (8 s limit).
    $gdb = Start-Process -FilePath $GdbExe -WorkingDirectory $PWD -WindowStyle Hidden -PassThru `
        -ArgumentList "-batch -ex `"target remote ${TargetHost}:${Port}`" -ex detach -ex quit `"$ElfFile`""
    if (-not $gdb.WaitForExit(8000)) { Stop-Process -Id $gdb.Id -Force -ErrorAction SilentlyContinue }
}

exit 0
