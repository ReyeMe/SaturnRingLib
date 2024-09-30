
@goto(){
  # Linux code here
  export COMPILER_DIR=../../Compiler
  export PATH=${COMPILER_DIR}/linux/sh2eb-elf/bin:${PATH}

  make clean | exit
}

@goto $@
exit

:(){
  @echo off
  rem Windows script here
  SET COMPILER_DIR=../../Compiler
  SET PATH=%COMPILER_DIR%\Other Utilities;%PATH%
  SET PATH=%COMPILER_DIR%\msys2\usr\bin;%PATH%
  SET PATH=%COMPILER_DIR%\sh2eb-elf\bin;%PATH%

  powershell write-host -back Yellow -fore Black Building debug...
  make clean DEBUG=1
  GOTO end

  :end
