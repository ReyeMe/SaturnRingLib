#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage: Tests/test_campaign.sh [options]

Runs the SaturnRingLib unit-test "campaign": build -> run -> generate reports.

Options:
  --emulator <mednafen|kronos|USBGamers>   Emulator target passed to Tests/run_tests.bat (default: mednafen)
  --skip-build                            Skip "make all" in Tests/
  --skip-run                              Skip running the emulator (requires existing Tests/uts.log)
  --skip-ctrf                             Skip generating Tests/uts.json
  --skip-junit                            Skip generating Tests/uts.xml
  --log <path>                            Path to uts.log (default: <repo>/Tests/uts.log)
  --out-dir <path>                        Directory for uts.json/uts.xml (default: <repo>/Tests)
  --strict                                Exit non-zero if uts.log contains any FATAL failures
  -h, --help                              Show help

Examples:
  bash Tests/test_campaign.sh --emulator mednafen
  bash Tests/test_campaign.sh --skip-build --skip-run   # convert existing uts.log to JSON/XML
EOF
}

# When this script lives in Tests/, the repo root is one directory up.
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

EMULATOR="mednafen"
SKIP_BUILD=0
SKIP_RUN=0
SKIP_CTRF=0
SKIP_JUNIT=0
STRICT=0

LOG_FILE="$ROOT_DIR/Tests/uts.log"
OUT_DIR="$ROOT_DIR/Tests"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --emulator)
      EMULATOR="$2"; shift 2;;
    --skip-build)
      SKIP_BUILD=1; shift;;
    --skip-run)
      SKIP_RUN=1; shift;;
    --skip-ctrf)
      SKIP_CTRF=1; shift;;
    --skip-junit)
      SKIP_JUNIT=1; shift;;
    --log)
      LOG_FILE="$2"; shift 2;;
    --out-dir)
      OUT_DIR="$2"; shift 2;;
    --strict)
      STRICT=1; shift;;
    -h|--help)
      usage; exit 0;;
    *)
      echo "Unknown arg: $1" >&2
      usage
      exit 2;;
  esac
done

mkdir -p "$OUT_DIR"

if [[ $SKIP_BUILD -eq 0 ]]; then
  echo "[campaign] Building tests (make all)"
  make -C "$ROOT_DIR/Tests" all
fi

if [[ $SKIP_RUN -eq 0 ]]; then
  echo "[campaign] Running tests via emulator: $EMULATOR"
  (
    cd "$ROOT_DIR/Tests"
    bash "$ROOT_DIR/Tests/run_tests.bat" "$EMULATOR"
  )
fi

if [[ ! -f "$LOG_FILE" ]]; then
  echo "uts.log not found at: $LOG_FILE" >&2
  echo "Hint: re-run without --skip-run or pass --log <path>." >&2
  exit 3
fi

if [[ $SKIP_CTRF -eq 0 ]]; then
  echo "[campaign] Generating CTRF JSON: $OUT_DIR/uts.json"
  python3 "$ROOT_DIR/tools/scripts/ctrf_converter.py" "$LOG_FILE" "$OUT_DIR/uts.json"
fi

if [[ $SKIP_JUNIT -eq 0 ]]; then
  echo "[campaign] Generating JUnit XML: $OUT_DIR/uts.xml"
  python3 "$ROOT_DIR/tools/scripts/junit_converter.py" "$LOG_FILE" "$OUT_DIR/uts.xml"
fi

if [[ $STRICT -eq 1 ]]; then
  echo "[campaign] Strict mode: failing if any FATAL failures exist"
  python3 - <<PY
import re
log_file = r'''$LOG_FILE'''
failed = 0
with open(log_file, 'r', encoding='utf-8', errors='replace') as f:
    for line in f:
        if re.match(r"FATAL\s*:\s*.*\s+failed:", line):
            failed += 1
if failed:
    raise SystemExit(f"Strict mode: {failed} failures found in {log_file}")
print("Strict mode: 0 failures")
PY
fi

echo "[campaign] Done. Inputs/outputs:"
echo "  log : $LOG_FILE"
[[ $SKIP_CTRF -eq 0 ]] && echo "  ctrf: $OUT_DIR/uts.json"
[[ $SKIP_JUNIT -eq 0 ]] && echo "  junit: $OUT_DIR/uts.xml"
