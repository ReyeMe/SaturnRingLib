@ECHO Off
where /q mednafen.exe
IF ERRORLEVEL 1 (
    echo "Using project mednafen installation!"
    SET MEDNAFEN=../../mednafen/mednafen.exe
) else (
    echo "Using system's mednafen installation!"
    SET MEDNAFEN=../../mednafen/mednafen.exe
)

if not exist ./cd/BuildDrop/*.cue (
    echo "CUE/ISO missing, please build first."
) else (
    @REM Finding first cue file and running it on mednafen
    FOR %%F IN (./cd/BuildDrop/*.cue) DO (
        start "" /MIN %MEDNAFEN% ./cd/BuildDrop/%%F
        exit /b
    )
)
