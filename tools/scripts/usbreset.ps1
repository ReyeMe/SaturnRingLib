# Windows counterpart of Linux's `usbreset "<device name>"`: restarts a USB
# device so the DevCart's FT245R enumerates cleanly again before an ftx upload.
# There is no stock usbreset.exe, so this uses the built-in PnP tooling
# (pnputil /restart-device, Windows 10 2004+). Requires an elevated shell.
#
# Usage: usbreset.ps1 "FT245R USB FIFO"
param(
    [string]$Name = "FT245R USB FIFO"
)

# FTDI FT245R: matched by VID/PID too, because Windows lists the device under
# its driver's friendly name (e.g. "USB Serial Converter"), not the product string.
$vidPid = "VID_0403&PID_6001"

$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Error "usbreset needs an elevated (Administrator) shell to restart a USB device."
    exit 1
}

$devices = @(Get-PnpDevice -PresentOnly -ErrorAction SilentlyContinue | Where-Object {
    $_.FriendlyName -eq $Name -or $_.InstanceId -like "USB\$vidPid*"
})

if ($devices.Count -eq 0) {
    Write-Error "device not found: $Name ($vidPid)"
    exit 1
}

$failed = $false
foreach ($device in $devices) {
    Write-Host "Resetting $($device.FriendlyName) [$($device.InstanceId)]"
    & pnputil.exe /restart-device "$($device.InstanceId)" | Out-Host
    if ($LASTEXITCODE -ne 0) {
        # Older Windows without /restart-device: disable/enable does the same job.
        try {
            Disable-PnpDevice -InstanceId $device.InstanceId -Confirm:$false -ErrorAction Stop
            Enable-PnpDevice -InstanceId $device.InstanceId -Confirm:$false -ErrorAction Stop
        }
        catch {
            Write-Error $_
            $failed = $true
        }
    }
}

if ($failed) { exit 1 }
