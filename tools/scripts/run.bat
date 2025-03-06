@echo off

if not exist ./BuildDrop/*.cue (
    echo "CUE/ISO missing, please build first."
    GOTO end
)

rem Emulator selection, empty is mednafen
IF "%1" == "" GOTO mednafen
IF "%1" == "mednafen" GOTO mednafen
IF "%1" == "kronos" GOTO kronos
IF "%1" == "yabause" GOTO yabause

rem We do not know what emulator user wants
echo "%1" is not supported
GOTO end

:kronos
rem Run Kronos
where /q kronos.exe

IF ERRORLEVEL 1 (
    echo Using project Kronos installation!
    SET KRONOS=../../emulators/kronos/kronos.exe
) else (
    echo Using system's Kronos installation!
    SET KRONOS=kronos.exe
)

FOR %%F IN (./BuildDrop/*.cue) DO (
    start %KRONOS% -a -i ./BuildDrop/%%F
    exit /b
)

GOTO end
rem Kronos block end

:yabause
rem Run yabause
where /q yabause.exe

IF ERRORLEVEL 1 (
    echo Using project yabause installation!
    SET YABAUSE=../../emulators/yabause/yabause.exe
) else (
    echo Using system's yabause installation!
    SET YABAUSE=yabause.exe
)

FOR %%F IN (./BuildDrop/*.cue) DO (
    start %YABAUSE% -a -i ./BuildDrop/%%F
    exit /b
)

GOTO end
rem yabause block end

:mednafen
rem Run mednafen
where /q mednafen.exe

IF ERRORLEVEL 1 (
    echo Using project mednafen installation!
    SET MEDNAFEN=../../emulators/mednafen/mednafen.exe
) else (
    echo Using system's mednafen installation!
    SET MEDNAFEN=mednafen.exe
)

FOR %%F IN (./BuildDrop/*.cue) DO (
    start %MEDNAFEN% ./BuildDrop/%%F
    exit /b
)

GOTO end
rem mednafen block end

:end
