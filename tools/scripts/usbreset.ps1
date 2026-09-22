# Windows counterpart of Linux's `usbreset "<device name>"`: restarts a USB
# device so the DevCart's FT245R enumerates cleanly again before an ftx upload.
# There is no stock usbreset.exe, so this uses the built-in PnP tooling
# (pnputil /restart-device, Windows 10 2004+).
#
# Resetting a device driver needs Administrator rights, but the VS Code task
# that calls this runs in whatever shell the user has open -- normally not
# elevated. Rather than require the whole terminal to run as Administrator,
# an unelevated run here re-launches itself elevated (one UAC prompt) in a
# background job and waits on that job with a timeout.
#
# This script is a best-effort step in a larger upload task, never the whole
# task, so it is designed to NEVER block or fail that task: a declined/
# unanswered UAC prompt, a reset failure, or the device not being found are
# all reported as warnings, not errors -- this always exits 0.
#
# Usage: usbreset.ps1 "FT245R USB FIFO"
param(
    [Parameter(Position = 0)]
    [string]$Name = "FT245R USB FIFO",

    # Internal: set by the unelevated launcher when re-invoking itself
    # elevated, so the elevated instance can hand its output back.
    [string]$LogFile
)

# How long to wait for the UAC prompt to be answered and the reset to finish
# before giving up on it (and letting the caller continue regardless).
$ElevationTimeoutSeconds = 20

$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    $logPath = [System.IO.Path]::GetTempFileName()
    $elevatedArgs = @(
        "-NoProfile", "-ExecutionPolicy", "Bypass",
        "-File", "`"$PSCommandPath`"",
        "-Name", "`"$Name`"",
        "-LogFile", "`"$logPath`""
    )

    # Start-Process -Verb RunAs blocks the calling thread until the UAC
    # prompt itself is resolved, even without -Wait -- so it's run inside a
    # background job purely to give it a timeout; without one, an ignored
    # UAC prompt would hang this script (and whatever task called it)
    # indefinitely.
    $job = Start-Job -ScriptBlock {
        param($elevatedArgs)
        try {
            $proc = Start-Process -FilePath "powershell.exe" -ArgumentList $elevatedArgs -Verb RunAs -Wait -PassThru -WindowStyle Hidden
            return $proc.ExitCode
        }
        catch {
            # Most commonly: the user dismissed the UAC prompt (Win32Exception 1223).
            return $null
        }
    } -ArgumentList (, $elevatedArgs)

    $exitCode = $null
    Wait-Job -Job $job -Timeout $ElevationTimeoutSeconds | Out-Null
    if ($job.State -eq 'Completed') {
        $exitCode = Receive-Job -Job $job
    }
    else {
        Write-Warning "usbreset: timed out after ${ElevationTimeoutSeconds}s waiting for the elevated USB reset (UAC prompt not answered?); continuing without a USB reset."
    }
    Remove-Job -Job $job -Force -ErrorAction SilentlyContinue

    Get-Content -Path $logPath -ErrorAction SilentlyContinue | ForEach-Object { Write-Host $_ }
    Remove-Item -Path $logPath -ErrorAction SilentlyContinue

    if ($null -eq $exitCode) {
        Write-Warning "usbreset: elevation was cancelled or failed; continuing without a USB reset."
    }
    elseif ($exitCode -ne 0) {
        Write-Warning "usbreset: USB reset failed (exit $exitCode); continuing anyway."
    }
    exit 0
}

# From here on, this is the elevated instance (either the user's shell was
# already elevated, or we're the re-launched copy above).
function Write-Log($message) {
    Write-Host $message
    if ($LogFile) { Add-Content -Path $LogFile -Value $message }
}

# FTDI FT245R: matched by VID/PID too, because Windows lists the device under
# its driver's friendly name (e.g. "USB Serial Converter"), not the product string.
$vidPid = "VID_0403&PID_6001"

$devices = @(Get-PnpDevice -PresentOnly -ErrorAction SilentlyContinue | Where-Object {
    $_.FriendlyName -eq $Name -or $_.InstanceId -like "USB\$vidPid*"
})

if ($devices.Count -eq 0) {
    Write-Log "usbreset: device not found: $Name ($vidPid); continuing without a USB reset."
    exit 0
}

foreach ($device in $devices) {
    Write-Log "Resetting $($device.FriendlyName) [$($device.InstanceId)]"
    $result = & pnputil.exe /restart-device "$($device.InstanceId)" 2>&1
    $result | ForEach-Object { Write-Log $_ }
    if ($LASTEXITCODE -ne 0) {
        # Older Windows without /restart-device: disable/enable does the same job.
        try {
            Disable-PnpDevice -InstanceId $device.InstanceId -Confirm:$false -ErrorAction Stop
            Enable-PnpDevice -InstanceId $device.InstanceId -Confirm:$false -ErrorAction Stop
        }
        catch {
            Write-Log "usbreset: reset failed for $($device.InstanceId): $_; continuing anyway."
        }
    }
}

exit 0
