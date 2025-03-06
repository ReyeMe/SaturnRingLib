@echo off
SET COMPILER_DIR=../../Compiler
IF NOT "%2" == "" GOTO customDir
:startBuild

SET PATH=%COMPILER_DIR%\Other Utilities;%PATH%
SET PATH=%COMPILER_DIR%\msys2\usr\bin;%PATH%
SET PATH=%COMPILER_DIR%\sh2eb-elf\bin;%PATH%

IF "%1" == "debug" GOTO debug
IF "%1" == "release" GOTO release
IF "%1" == "clean" GOTO clean

:debug
powershell write-host -back Yellow -fore Black Building debug...
make all DEBUG=1
GOTO end

:release
powershell write-host -back Green -fore White Building release...
make all
GOTO end

:clean
powershell write-host -back Green -fore White Cleaning...
make clean
GOTO end

:customDir
powershell write-host -fore Red Using custom compiler path
SET COMPILER_DIR=%2
GOTO startBuild

:end
