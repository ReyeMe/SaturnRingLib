@goto(){
  # Linux test runner script for Saturn unit tests
  # Usage: ./run_tests.bat [kronos|mednafen]
 
  if [ -z "$1" ]; then
    echo "Usage: $0 [kronos|mednafen]"
    exit 1
  fi

  # Set timeout in seconds
  TIMEOUT=600

  cleanup() {
      # Kill watchdog if it's running
      [[ -n $WATCHDOG_PID ]] && kill $WATCHDOG_PID 2>/dev/null
      # Add your cleanup tasks here
  }

  # Set up trap for cleanup
  trap cleanup EXIT

  # Start watchdog in background
  (
      sleep $TIMEOUT
      echo "Script timed out after $TIMEOUT seconds"
      kill -9 -$$ 2>/dev/null
  ) &

  WATCHDOG_PID=$!

  echo "Starting Saturn unit test runner..."

  # Configure emulator based on input parameter
  if [ "$1" = "mednafen" ]; then
    echo "Using mednafen emulator"
    # Disable video output and enable debug cart
    export SDL_VIDEODRIVER=dummy
    command="mednafen -sound 0 -ss.cart debug -force_module ss BuildDrop/UTs.cue"
  elif [ "$1" = "kronos" ]; then
    echo "Using kronos emulator"
    # Run kronos in automation mode with no sound
    command="kronos -a -ns -i BuildDrop/UTs.cue"
  else
    echo "No valid emulator specified"
    exit 1
  fi

  # Setup log file and success marker
  log="uts.log"
  match="***UT_END***"
  
  echo "Test command: $command"
  echo "Watching log file: $log"
  echo "Waiting for completion marker: $match"

  # Run emulator and capture output
  $command 2>&1 | tee "$log" &

  echo "Emulator started, monitoring for completion..."

  # Monitor log file for completion
  while sleep 1
  do
      if fgrep --quiet "$match" "$log"
      then
          echo "Test completion marker found"
          echo "Terminating emulator..."
          pkill -9 kronos
          echo "Tests completed successfully"
          exit 0
      fi
  done
}

# Windows/DOS compatibility wrapper
@goto $@
exit

:(){
  @echo off
  rem Windows implementation placeholder
  echo "Some MS Windows foos required here"
  )
  GOTO end

  :end