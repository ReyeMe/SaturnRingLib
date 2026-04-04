# Linux code here
if [[ $# -eq 2 ]]; then
  printf "\033[91mUsing custom compiler path\033[0m\r\n"
  export COMPILER_DIR=$2;
else
  export COMPILER_DIR=../../Compiler;
fi

host_platform="$(uname -s)"
if [ "$host_platform" = "Darwin" ]; then
  export PATH=${COMPILER_DIR}/mac/sh2eb-elf/bin:${PATH};
elif [ "$host_platform" = "Linux" ]; then
  export PATH=${COMPILER_DIR}/linux/sh2eb-elf/bin:${PATH};
else
  echo "Unsupported host platform: $host_platform"
  exit 1
fi

if [[ $# -eq 0 ]]; then
  printf "\033[91mNo target specified! Defaulting to debug...\033[0m\r\n"
  printf "\033[30m\033[43mBuilding debug...\033[0m\r\n"
  make all DEBUG=1 || exit
fi

if [[ "$1" = "debug" ]]; then
  printf "\033[30m\033[43mBuilding debug...\033[0m\r\n"
  make all DEBUG=1 || exit
fi

if [[ "$1" = "release" ]]; then
  printf "\033[30m\033[42mBuilding release...\033[0m\r\n"
  make all || exit
fi

if [[ "$1" = "clean" ]]; then
  printf "\033[30m\033[105mCleaning...\033[0m\r\n"
  make clean || exit
fi

exit
