:; "./tools/scripts/getcompiler.sh" "14.2.0" && "./tools/scripts/getiso2raw.sh" "v0.2.2"; exit;
@ECHO Off
PowerShell -ExecutionPolicy Bypass -file "./tools/scripts/getcompiler.ps1" "14.2.0"
PowerShell -ExecutionPolicy Bypass -file "./tools/scripts/getiso2raw.ps1" "v0.2.2"