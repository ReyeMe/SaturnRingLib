#!/bin/bash

version=$1

if [ -z "$version" ]; then
    echo "Usage: $0 <version>"
    echo "Example: $0 v0.2.2"
    exit 1
fi

# Detect OS and architecture
OS=$(uname -s)
ARCH=$(uname -m)

# Set the appropriate file and platform directory based on OS and architecture
if [ "$OS" = "Darwin" ]; then
    if [ "$ARCH" = "arm64" ]; then
        file="iso2raw-macos-arm64.zip"
    else
        file="iso2raw-macos-amd64.zip"
    fi
    platform="mac"
elif [ "$OS" = "Linux" ]; then
    file="iso2raw-linux-amd64.zip"
    platform="lin"
else
    echo "Unsupported operating system: $OS"
    exit 1
fi

toolDir=./tools/bin/$platform/iso2raw
url="https://github.com/sftwninja/iso2raw/releases/download/${version}/$file"
target="$toolDir/$file"

if [ ! -d "$toolDir" ]; then
  mkdir -p $toolDir
else
  if [ "$(ls -A $toolDir)" ]; then
    echo "iso2raw directory is not empty! Proceeding will clear all of its contents."
    read -r -p "Are you sure? [y/N] " response

    case "$response" in
    [yY][eE][sS]|[yY]) 
        rm -rf $toolDir/*
        ;;
    *)
        exit
        ;;
    esac
  fi
fi

# Ensure parent directories exist
mkdir -p $(dirname $toolDir)
cd $toolDir
wget $url # -q --show-progress

if [ ! -f $file ]; then
  echo "Installation failed!";
  exit
fi

printf "Extracting";
# Create a temp directory for extraction
mkdir -p temp_extract
if ! command -v pv 2>&1 >/dev/null
then
  unzip -q $file -d temp_extract
  printf "."
else
  unzip $file -d temp_extract | pv -l >/dev/null
fi

# Move contents from subdirectory to current directory
mv temp_extract/*/* . 2>/dev/null || mv temp_extract/* .
rmdir temp_extract/* 2>/dev/null
rmdir temp_extract

# Rename the binary to just 'iso2raw' (remove any version suffix)
for f in iso2raw-*; do
    if [ -f "$f" ]; then
        mv "$f" iso2raw
        break
    fi
done

printf "\nCleaning up...\n";
rm -f $file

printf "\nSetting permissions\n";
chmod -R +x .
cd ../../..