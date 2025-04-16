@goto(){
  # Linux code here
  if [ "$1" = "mednafen" ]; then
    export SDL_VIDEODRIVER=dummy
    command="mednafen -sound 0 -ss.cart debug -force_module ss BuildDrop/UTs.cue"
  else
    command="kronos -a -ns -i BuildDrop/UTs.cue"
  fi

  log="uts.log"
  match="***UT_END***"

  $command 2>&1 | tee "$log" &

  while sleep 1
  do
      if fgrep --quiet "$match" "$log"
      then
          pkill -9 kronos
          exit 0
      fi
  done
}

@goto $@
exit

:(){
  @echo off
  rem Windows script here
  echo "Some MS Windows foos required here"

  )
  GOTO end

  :end
