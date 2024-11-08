@ECHO Off

SET COMPILER_DIR=../Compiler
SET PATH=%COMPILER_DIR%\Other Utilities;%PATH%
SET PATH=%COMPILER_DIR%\msys2\usr\bin;%PATH%
SET PATH=%COMPILER_DIR%\sh2eb-elf\bin;%PATH%

IF "%1" == "debug" GOTO debug
IF "%1" == "release" GOTO release

:debug
powershell write-host -back Yellow -fore Black Building debug...
make all DEBUG=1
GOTO end

:release
powershell write-host -back Green -fore White Building release...
make all
GOTO end

:end