@ECHO Off
SET COMPILER_DIR=../../Compiler
SET PATH=%COMPILER_DIR%\Other Utilities;%PATH%
SET PATH=%COMPILER_DIR%\msys2\usr\bin;%PATH%
SET PATH=%COMPILER_DIR%\sh2eb-elf\bin;%PATH%
make clean