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
    exit 0
  fi
}

run_kronos() {
  # We assumed that Kronos is already installed and in the PATH. Check if its true
  if ! command -v kronos 2>&1 >/dev/null
  then
    echo "kronos could not be found!"
    exit 1
  fi

  cue_files=( ./BuildDrop/*.cue )

  if [[ ${#cue_files[@]} -eq 0 ]]; then
    echo "VBT, you shoud build before testing !"
    exit 1
  else
    echo "STARTING ${cue_files[0]} !"
    kronos -ns -a -i ${cue_files[0]} || exit
    exit 0
  fi

}

run_yabause() {
  # We assumed that Yabause is already installed and in the PATH. Check if its true
  if ! command -v yabause 2>&1 >/dev/null
  then
    echo "yabause could not be found!"
    exit 1
  fi

  cue_files=( ./BuildDrop/*.cue )

  if [[ ${#cue_files[@]} -eq 0 ]]; then
    echo "VBT, you shoud build before testing !"
    exit 1
  else
    echo "STARTING ${cue_files[0]} !"
    yabause -a -i ${cue_files[0]} || exit
    exit 0
  fi

}

if [[ ! -d "BuildDrop" ]]; then
  echo "BuildDrop does not exist."
  exit 1
fi

# Mednafen is default, it will start if there is no argument to specify emulator
if [ "$1" == "mednafen" ] || [ "$1" == "" ]; then
  run_medanfen || exit
  exit 0
fi

if [[ "$1" == "kronos" ]]; then
  run_kronos || exit
  exit 0
fi

if [[ "$1" == "yabause" ]]; then
  run_yabause || exit
  exit 0
fi

echo "$1" is not supported!
