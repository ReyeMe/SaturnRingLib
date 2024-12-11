# Linux code here
run_medanfen() {
  # We assumed that mednafen is already installed and in the PATH. Check if its true
  if ! command -v mednafen 2>&1 >/dev/null
  then
    echo "mednafen could not be found!"
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

run_kronos() {
  # We assumed that Kronos is already installed and in the PATH. Check if its true
  if ! command -v kronos 2>&1 >/dev/null
  then
    echo "kronos could not be found!"
    exit 1
  fi

}

if [[ ! -d "BuildDrop" ]]; then
  echo "BuildDrop does not exist."
  exit 1
fi

# Mednafen is default, it will start if there is no argument to specify emulator
if [ "$1" == "mednafen" ] || [ "$1" == "" ]; then
  run_medanfen || exit
fi

if [[ "$1" == "kronos" ]]; then
  run_kronos || exit
fi

echo "$1" is not supported!