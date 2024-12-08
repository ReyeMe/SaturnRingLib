@goto(){
  # Linux code here

  export COMPILER_DIR=../../Compiler
  export PATH=${COMPILER_DIR}/linux/sh2eb-elf/bin:${PATH}

  if [[ $# -eq 0 ]]; then
    echo "Not enough arguments"
    exit 1
  fi

  if [[ "$1" = "debug" ]]; then
    echo "BUILD IN DEBUG : $1"
    make all DEBUG=1 || exit
  elif [[ "$1" = "release" ]]; then
    echo "BUILD IN RELEASE"
    make all || exit
  fi
}

@goto $@
exit

:(){

    @ECHO Off

    SET COMPILER_DIR=../../Compiler
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
