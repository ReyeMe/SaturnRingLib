@goto(){
  # Linux test runner script for Saturn unit tests
  # Usage: ./run_tests.bat <kronos|mednafen|USBGamers> <campaign> [device-ip]
  # <campaign> is a folder of campaigns/ (math, memory_simple, memory_tlsf, memory_cpus_simple, memory_cpus_tlsf, video, system, timer, event, datetime, cd, media, slave, scene, string, endian, vdp1, vdp2, input, sound)

  if [ -z "$1" ] || [ -z "$2" ]; then
    echo "Usage: $0 <kronos|mednafen|USBGamers> <campaign> [device-ip]"
    exit 1
  fi

  MODE="$1"
  CAMPAIGN="$2"
  DIR="campaigns/$CAMPAIGN"
  if [ ! -d "$DIR" ]; then
    echo "Unknown campaign '$CAMPAIGN'. Available: $(ls campaigns | tr '\n' ' ')"
    exit 1
  fi

  # Per-campaign settings (modes to skip, known failures) live in campaigns.json
  if ! skip_reason=$(python3 -c 'import json, sys; print(json.load(open("campaigns.json"))["campaigns"].get(sys.argv[1], {}).get("skip_modes", {}).get(sys.argv[2], ""))' "$CAMPAIGN" "$MODE"); then
    echo "Could not read campaigns.json"
    exit 1
  fi
  if [ -n "$skip_reason" ]; then
    echo "Skipping the $CAMPAIGN campaign in $MODE mode: $skip_reason"
    exit 0
  fi

  # Set timeout in seconds
  TIMEOUT=600

  # Runs on every exit (done, failed, timed out): stop the watchdog and the emulator, power the Saturn off
  cleanup() {
    status=$?
    [[ -n $WATCHDOG_PID ]] && kill $WATCHDOG_PID 2>/dev/null
    [[ -n $EMULATOR_PID ]] && kill $EMULATOR_PID 2>/dev/null
    if [ "$MODE" = "USBGamers" ] && [ -n "$DEVICE_IP" ]; then
      echo "Powering off target system at $DEVICE_IP via REST API..."
      curl -s --max-time 5 -X POST "http://$DEVICE_IP/api/v1/off" > /dev/null
    fi
    exit $status
  }

  reset_usb_device() {
    echo "Resetting USB device..."
    if ! usbreset "FT245R USB FIFO"; then
      echo "USB reset failed"
      exit 1
    fi
  }

  # Set up trap for cleanup
  trap cleanup EXIT
  trap 'exit 1' TERM INT

  # Start watchdog in background (TERM, not KILL, so cleanup still runs)
  (
      sleep $TIMEOUT
      echo "Script timed out after $TIMEOUT seconds"
      kill -TERM $$ 2>/dev/null
  ) &

  WATCHDOG_PID=$!

  echo "Starting Saturn unit test runner..."

  # Configure emulator based on input parameter
  if [ "$1" = "mednafen" ]; then
    echo "Using mednafen emulator"
    # Disable video output and enable debug cart
    export SDL_VIDEODRIVER=dummy
    command="mednafen -sound 0 -ss.cart debug -force_module ss $DIR/BuildDrop/$CAMPAIGN.cue"
  elif [ "$1" = "kronos" ]; then
    echo "Using kronos emulator"
    # Run kronos in automation mode with no sound
    command="kronos -a -ns -i $DIR/BuildDrop/$CAMPAIGN.cue"
  elif [ "$1" = "USBGamers" ]; then
        # Precondition: check if cd/data/0.bin exists
        if [ ! -f "$DIR/cd/data/0.bin" ]; then
          echo "ERROR: $DIR/cd/data/0.bin not found. Please build the test binary before running."
          exit 1
        fi
    echo "Using USBGamers cartridge"
    # Optional: $3 is IPv4 for REST API control
    DEVICE_IP="$3"
    if [ -n "$DEVICE_IP" ]; then
      # Validate IPv4 address format
      if [[ ! $DEVICE_IP =~ ^([0-9]{1,3}\.){3}[0-9]{1,3}$ ]]; then
        echo "Invalid IPv4 address: $DEVICE_IP"
        exit 1
      fi
      # Check if HTTP server is running (expecting JSON with relay_status)
      echo "Checking if REST API server is up at $DEVICE_IP..."
      if ! curl -s --connect-timeout 3 "http://$DEVICE_IP/api/v1/status" | grep -q 'relay_status'; then
        echo "REST API server not reachable or invalid response at $DEVICE_IP"
        exit 1
      fi
      echo "Controlling target system at $DEVICE_IP via REST API..."
      # Query current relay status
      relay_status=$(curl -s "http://$DEVICE_IP/api/v1/status" | grep -o '"relay_status":"[A-Z]*"' | cut -d'"' -f4)
      echo "Current relay status: $relay_status"
      # Query latch period (seconds)
      latch_period=$(curl -s "http://$DEVICE_IP/api/v1/latch" | grep -o '"latch":[0-9]*' | cut -d':' -f2)
      [ -z "$latch_period" ] && latch_period=0
      # If already ON, toggle OFF, wait for latch, then toggle ON
      if [ "$relay_status" = "ON" ]; then
        echo "Relay already ON, toggling OFF first..."
        curl -s -X POST "http://$DEVICE_IP/api/v1/toggle" > /dev/null
        # Wait for latch period to expire (poll status)
        if [ "$latch_period" -gt 0 ]; then
          echo "Waiting for latch period ($latch_period s) to expire..."
          latch_left=$latch_period
          latch_timeout=$((latch_period + 10)) # 10s grace period
          latch_elapsed=0
          while [ $latch_left -gt 0 ]; do
            sleep 1
            latch_elapsed=$((latch_elapsed + 1))
            latch_now=$(curl -s "http://$DEVICE_IP/api/v1/status" | grep -o '"latch":[0-9]*' | cut -d':' -f2)
            [ -z "$latch_now" ] && latch_now=0
            if [ "$latch_now" -eq 0 ]; then
              break
            fi
            latch_left=$latch_now
            if [ $latch_elapsed -ge $latch_timeout ]; then
              echo "[ERROR] Latch wait timed out after $latch_timeout seconds."
              break
            fi
          done
        fi
        echo "Toggling ON..."
        curl -s -X POST "http://$DEVICE_IP/api/v1/toggle" > /dev/null
      else
        # Power ON (POST /api/v1/on)
        curl -s -X POST "http://$DEVICE_IP/api/v1/on" > /dev/null
      fi
      echo "Waiting 20 seconds for system to power on..."
      sleep 20
      # Check relay status (GET /api/v1/status)
      echo -n "Relay status: "
      curl -s "http://$DEVICE_IP/api/v1/status" | grep -o '"relay_status":"[A-Z]*"' | cut -d'"' -f4
    fi
    # Push the test binary to the cartridge and run it
    command="ftx -c"
    # Makes sure the USB device is reset before programming
    reset_usb_device
    sleep 2
    upload_log=$(ftx -x "$DIR/cd/data/0.bin" 0x06004000 2>&1)
    upload_status=$?
    echo "$upload_log"
    if [[ $upload_status -ne 0 ]] || echo "$upload_log" | grep -qi "Upload failed\|Send data error"; then
      echo "Upload failed, aborting"
      exit 1
    fi
  else
    echo "No valid emulator specified"
    exit 1
  fi

  # Setup log file and success marker
  log="uts_$CAMPAIGN.log"
  match="***UT_END***"
  
  echo "Test command: $command"
  echo "Watching log file: $log"
  echo "Waiting for completion marker: $match"

  # Run emulator and capture output
  $command > >(tee "$log") 2>&1 &
  
  EMULATOR_PID=$!

  # Start timer for 5 minutes (300 seconds)
  TIMER_START=$(date +%s)
  TIMER_LIMIT=300

  echo "Emulator started, monitoring for completion..."

  # Monitor log file for completion
  while sleep 1
  do
      # Check timer
      TIMER_NOW=$(date +%s)
      TIMER_ELAPSED=$((TIMER_NOW - TIMER_START))
      if [ $TIMER_ELAPSED -ge $TIMER_LIMIT ]; then
          echo "Test timed out after $TIMER_LIMIT seconds"
          echo "Terminating emulator due to timeout..."
          if kill -0 $EMULATOR_PID 2>/dev/null; then
            kill -15 $EMULATOR_PID
            # Wait for exit, so the next run is not blocked by the emulator's lock file
            wait $EMULATOR_PID 2>/dev/null
          else
            echo "Emulator process is not running"
          fi
          echo "Timeout occurred, exiting"
          exit 1
      fi

      if fgrep --quiet "$match" "$log"
      then
          echo "Test completion marker found"
          echo "Terminating emulator..."
          if kill -0 $EMULATOR_PID 2>/dev/null; then
            kill -15 $EMULATOR_PID
            # Wait for exit, so the next run is not blocked by the emulator's lock file
            wait $EMULATOR_PID 2>/dev/null
          else
            echo "Emulator process is not running"
          fi
          if ! grep -q 'tests, .* failures' "$log"; then
            echo "No test summary in the log"
            exit 1
          fi
          # Failures listed under known_failures in campaigns.json (known library bugs) do not fail the run
          known=$(python3 -c 'import json, sys; print("\n".join(json.load(open("campaigns.json"))["campaigns"].get(sys.argv[1], {}).get("known_failures", {})))' "$CAMPAIGN" | sort -u)
          failed=$(tr -d '\r' < "$log" | grep -o 'FATAL : [A-Za-z0-9_]* failed' | cut -d' ' -f3 | sort -u)
          passed=$(tr -d '\r' < "$log" | grep -o 'Passed :[A-Za-z0-9_]*' | cut -d: -f2 | sort -u)
          new_failures=$(comm -23 <(echo "$failed") <(echo "$known") | grep -v '^$')
          now_passing=$(comm -12 <(echo "$passed") <(echo "$known") | grep -v '^$')
          echo "Known failures: $(comm -12 <(echo "$failed") <(echo "$known") | grep -vc '^$')"
          [ -n "$now_passing" ] && echo "Known failures that now pass (remove them from campaigns.json):" && echo "$now_passing"
          if [ -n "$new_failures" ]; then
            echo "New failures:"
            echo "$new_failures"
            exit 1
          fi
          echo "Tests completed successfully"
          exit 0
      fi
      # Check if emulator process is still running
      if ! kill -0 $EMULATOR_PID 2>/dev/null; then
          echo "Emulator process has terminated unexpectedly"
          exit 1
      fi
  done
}

# Windows/DOS compatibility wrapper
@goto $@
exit

:(){
  @echo off
  ..\tools\bin\win\ftx\ftx -x .\campaigns\%2\cd\data\0.bin 0x06004000 
  ..\tools\bin\win\ftx\ftx -c
  )
  GOTO end

  :end
