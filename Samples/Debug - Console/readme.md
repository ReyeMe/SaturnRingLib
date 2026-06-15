# Console Debug Sample (DevCart + ftx)

This sample demonstrates bidirectional console communication between host and Sega Saturn over the DevCart USB FIFO.

## Behavior

- Saturn sends `HELLO\n` once per second.
- Saturn accumulates input until line end (`\r` or `\n`).
- When a full line is received, Saturn responds with `OK : <received string>\n`.
- After the first successful `OK : ...` response, periodic HELLO stops.

## Build

```bash
# From inside Samples/Debug - Console/
../../tools/scripts/make.sh
```

Artifacts:

- `./BuildDrop/Debug_Console.elf`
- `./BuildDrop/Debug_Console.bin`

## Run On Real Saturn (USBGamers)

```bash
usbreset "FT245R USB FIFO"
sleep 2
ftx -c
```

In another terminal:

```bash
cd /saturn/SaturnRingLib/Samples/Debug\ -\ Console
../../tools/scripts/run.sh USBGamers
```

## Bidirectional Validation With ftx -c

1. Keep `ftx -c` running.
2. Verify repeated `HELLO` output every second.
3. Type a line in the `ftx -c` terminal, for example:
   - `PING` then Enter
4. Verify Saturn replies with `OK : PING`.
5. Verify periodic `HELLO` no longer appears after the first successful response.

Expected transcript pattern:

```text
HELLO
HELLO
HELLO
PING
OK : PING
```

If `ftx -c` shows Saturn output but typed input does not reach Saturn, build and use the local `/saturn/ftx` version with bidirectional `-c` support.
