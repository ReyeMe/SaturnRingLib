@goto(){
  # Linux test runner script for Saturn unit tests
  # Usage: ./run_tests.bat [kronos|mednafen|USBGamers]

  if [ -z "$1" ]; then
    echo "Usage: $0 [kronos|mednafen|USBGamers]"
    exit 1
  fi

  # Set timeout in seconds
  TIMEOUT=600

    cleanup() {
      status=$?
      # Kill watchdog if it's running
      [[ -n $WATCHDOG_PID ]] && kill $WATCHDOG_PID 2>/dev/null
      # Add your cleanup tasks here
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
    #export SDL_VIDEODRIVER=dummy
    command="mednafen -sound 0 -ss.cart debug -force_module ss BuildDrop/UTs.cue"
  elif [ "$1" = "kronos" ]; then
    echo "Using kronos emulator"
    # Run kronos in automation mode with no sound
    command="kronos -a -ns -i BuildDrop/UTs.cue"
  elif [ "$1" = "USBGamers" ]; then
    echo "Using USBGamers cartridge"
    # Optional: $2 is IPv4 for REST API control
    DEVICE_IP="$2"
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
          while [ $latch_left -gt 0 ]; do
            sleep 1
            latch_now=$(curl -s "http://$DEVICE_IP/api/v1/status" | grep -o '"latch":[0-9]*' | cut -d':' -f2)
            [ -z "$latch_now" ] && latch_now=0
            if [ "$latch_now" -eq 0 ]; then
              break
            fi
            latch_left=$latch_now
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
    upload_log=$(ftx -x cd/data/0.bin 0x06004000 2>&1)
    upload_status=$?
    echo "$upload_log"
    if [[ $upload_status -ne 0 ]] || echo "$upload_log" | grep -qi "Upload failed\|Send data error"; then
      echo "Upload failed, aborting"
      # Power OFF if REST API was used
      if [ -n "$DEVICE_IP" ]; then
        echo "Powering off target system at $DEVICE_IP via REST API..."
        # Query current relay status
        relay_status=$(curl -s "http://$DEVICE_IP/api/v1/status" | grep -o '"relay_status":"[A-Z]*"' | cut -d'"' -f4)
        latch_period=$(curl -s "http://$DEVICE_IP/api/v1/latch" | grep -o '"latch":[0-9]*' | cut -d':' -f2)
        [ -z "$latch_period" ] && latch_period=0
        # If already OFF, toggle ON, wait for latch, then toggle OFF
        if [ "$relay_status" = "OFF" ]; then
          echo "Relay already OFF, toggling ON first..."
          curl -s -X POST "http://$DEVICE_IP/api/v1/toggle" > /dev/null
          if [ "$latch_period" -gt 0 ]; then
            echo "Waiting for latch period ($latch_period s) to expire..."
            latch_left=$latch_period
            while [ $latch_left -gt 0 ]; do
              sleep 1
              latch_now=$(curl -s "http://$DEVICE_IP/api/v1/status" | grep -o '"latch":[0-9]*' | cut -d':' -f2)
              [ -z "$latch_now" ] && latch_now=0
              if [ "$latch_now" -eq 0 ]; then
                break
              fi
              latch_left=$latch_now
            done
          fi
          echo "Toggling OFF..."
          curl -s -X POST "http://$DEVICE_IP/api/v1/toggle" > /dev/null
        else
          # Power OFF (POST /api/v1/off)
          curl -s -X POST "http://$DEVICE_IP/api/v1/off" > /dev/null
        fi
      fi
      exit 1
    fi
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
          else
            echo "Emulator process is not running"
          fi
          # Power OFF if REST API was used and USBGamers
          if [ "$1" = "USBGamers" ] && [ -n "$DEVICE_IP" ]; then
            echo "Powering off target system at $DEVICE_IP via REST API..."
            # Query current relay status
            relay_status=$(curl -s "http://$DEVICE_IP/api/v1/status" | grep -o '"relay_status":"[A-Z]*"' | cut -d'"' -f4)
            latch_period=$(curl -s "http://$DEVICE_IP/api/v1/latch" | grep -o '"latch":[0-9]*' | cut -d':' -f2)
            [ -z "$latch_period" ] && latch_period=0
            # If already OFF, toggle ON, wait for latch, then toggle OFF
            if [ "$relay_status" = "OFF" ]; then
              echo "Relay already OFF, toggling ON first..."
              curl -s -X POST "http://$DEVICE_IP/api/v1/toggle" > /dev/null
              if [ "$latch_period" -gt 0 ]; then
                echo "Waiting for latch period ($latch_period s) to expire..."
                latch_left=$latch_period
                while [ $latch_left -gt 0 ]; do
                  sleep 1
                  latch_now=$(curl -s "http://$DEVICE_IP/api/v1/status" | grep -o '"latch":[0-9]*' | cut -d':' -f2)
                  [ -z "$latch_now" ] && latch_now=0
                  if [ "$latch_now" -eq 0 ]; then
                    break
                  fi
                  latch_left=$latch_now
                done
              fi
              echo "Toggling OFF..."
              curl -s -X POST "http://$DEVICE_IP/api/v1/toggle" > /dev/null
            else
              # Power OFF (POST /api/v1/off)
              curl -s -X POST "http://$DEVICE_IP/api/v1/off" > /dev/null
            fi
          fi
          echo "Tests completed successfully"
          exit 0
      fi
      # Check if emulator process is still running
      if ! kill -0 $EMULATOR_PID 2>/dev/null; then
          echo "Emulator process has terminated unexpectedly"
          # Power OFF if REST API was used and USBGamers
          if [ "$1" = "USBGamers" ] && [ -n "$DEVICE_IP" ]; then
            echo "Powering off target system at $DEVICE_IP via REST API..."
            curl -s -X POST "http://$DEVICE_IP/api/v1/off" > /dev/null
          fi
          exit 1
      fi
  done
}

# Windows/DOS compatibility wrapper
@goto $@
exit

:(){
  @echo off
  rem Windows implementation placeholder
  echo "Some MS Windows magics required here"
  )
  GOTO end

  :end