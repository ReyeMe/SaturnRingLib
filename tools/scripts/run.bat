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
IF "%1" == "ymir" GOTO ymir
IF "%1" == "USBGamers" GOTO USBGamers

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

:ymir
rem Run ymir
where /q ymir-sdl3.exe

IF ERRORLEVEL 1 (
    echo Using project Ymir installation!
    SET YMIR=../../emulators/ymir/ymir-sdl3.exe
) else (
    echo Using system's Ymir installation!
    SET YMIR=ymir-sdl3.exe
)

FOR %%F IN (./BuildDrop/*.cue) DO (
    start %YMIR% ./BuildDrop/%%F
    exit /b
)

GOTO end
rem ymir block end

:USBGamers
rem Run ftx
where /q ftx.exe

IF ERRORLEVEL 1 (
    echo Using project ftx installation!
    SET FTX=../../tools/bin/win/ftx/ftx.exe
) else (
    echo Using system's ftx installation!
    SET FTX=ftx.exe
)

if not exist cd/data/0.bin (
    echo "0.bin missing, please build first."
    GOTO end
)

echo Starting FTX with %%F
echo Make sure to reset your USB port before running this script
start %FTX% -x ./cd/data/0.bin 0x06004000
timeout /t 2 > nul
start %FTX% -c

GOTO end
rem ftx block end

:end
