
@goto(){
  # Linux code here
  # We assumed that yabause is already installed and in the PATH
  if [[ ! -d "BuildDrop" ]]; then
    echo "BuildDrop does not exist."
    exit 1
  fi
  cue_files=( ./BuildDrop/*.cue )
  if [[ ${#cue_files[@]} -eq 0 ]]; then
    echo "Stop it VBT !"
    exit 1
  else
    echo "STARTING ${cue_files[0]} !"
    yabause -a -i ${cue_files[0]} || exit

  fi

}

@goto $@
exit

:(){
  @echo off
  rem Windows script here
  where /q yabause.exe
  IF ERRORLEVEL 1 (
      echo "yabause is missing !!"
      GOTO end
  ) else (
      echo "Using system's yabause installation!"
      SET YABAUSE=yabause.exe
  )

  if not exist ./BuildDrop/*.cue (
      echo "CUE/ISO missing, please build first."
  ) else (
      @REM Finding first cue file and running it on yabause
      FOR %%F IN (./BuildDrop/*.cue) DO (
          start "" /MIN %YABAUSE% -a -i ./BuildDrop/%%F
          exit /b
      )
  )
  GOTO end

  :end
