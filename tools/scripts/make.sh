# Linux code here

# 1. Determine if a custom compiler path was provided ($2)
# If provided (even if empty ""), make arguments start from $3. If not, they start from $2.
if [[ $# -ge 2 ]]; then
  if [[ -n "$2" ]]; then
    printf "\033[91mUsing custom compiler path\033[0m\r\n"
    export COMPILER_DIR="$2"
  else
    export COMPILER_DIR=../../Compiler
  fi
  MAKE_ARGS=("${@:3}") # Captures 3rd, 4th, 5th... arguments
else
  export COMPILER_DIR=../../Compiler
  MAKE_ARGS=()
fi

# 2. Environment Setup

# Absolute path, so builds that run `make -C <subfolder>` (Tests campaigns) still find the compiler
[ -d "$COMPILER_DIR" ] && COMPILER_DIR="$(cd "$COMPILER_DIR" && pwd)"

# Save the original PATH at script startup
OLD_PATH="$PATH"

host_platform="$(uname -s)"
if [ "$host_platform" = "Darwin" ]; then
  export PATH="${COMPILER_DIR}/mac/sh2eb-elf/bin:$PATH"
elif [ "$host_platform" = "Linux" ]; then
  export PATH="${COMPILER_DIR}/linux/sh2eb-elf/bin:$PATH"
else
  echo "Unsupported host platform: $host_platform"
  export PATH="$OLD_PATH"
  exit 1
fi

# 3. Handle default fallback safely
TARGET="${1:-debug}"

# 4. Execute Build Targets with the extra arguments
RC=0
case "$TARGET" in
  debug)
    printf "\033[30m\033[43mBuilding debug...\033[0m\r\n"
    make all DEBUG=1 "${MAKE_ARGS[@]}"
    RC=$?
    ;;
  release)
    printf "\033[30m\033[42mBuilding release...\033[0m\r\n"
    make all "${MAKE_ARGS[@]}"
    RC=$?
    ;;
  clean)
    printf "\033[30m\033[105mCleaning...\033[0m\r\n"
    make clean "${MAKE_ARGS[@]}"
    RC=$?
    ;;
  *)
    echo "Unknown target: $TARGET"
    RC=1
    ;;
esac

# Restore original PATH on exit
export PATH="$OLD_PATH"
exit $RC
