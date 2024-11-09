
@goto(){
  # Linux code here
  command="prog -foo -whatever"
  log="prog.log"
  match="this is the what i want to match"

  $command > "$log" 2>&1 &
  pid=$!

  while sleep 60
  do
      if fgrep --quiet "$match" "$log"
      then
          kill $pid
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
