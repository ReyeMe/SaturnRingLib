
@goto(){
  # Linux code here
  # We assumed that mednafen is already installed and in the PATH
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
    mednafen ${cue_files[0]} || exit

  fi

}

@goto $@
exit

:(){
  @echo off
  rem Windows script here
  where /q mednafen.exe
  IF ERRORLEVEL 1 (
      echo "Using project mednafen installation!"
      SET MEDNAFEN=../../emulators/mednafen/mednafen.exe
  ) else (
      echo "Using system's mednafen installation!"
      SET MEDNAFEN=mednafen.exe
  )

  if not exist ./BuildDrop/*.cue (
      echo "CUE/ISO missing, please build first."
  ) else (
      @REM Finding first cue file and running it on mednafen
      FOR %%F IN (./BuildDrop/*.cue) DO (
          start "" /MIN %MEDNAFEN% ./BuildDrop/%%F
          exit /b
      )
  )
  GOTO end

  :end
