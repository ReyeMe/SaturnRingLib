#!/bin/bash

version=$1

if [ -z "$version" ]; then
    echo "Usage: $0 <version>"
    echo "Example: $0 v0.98"
    exit 1
fi

# Detect OS and architecture
OS=$(uname -s)

# Set the appropriate file and platform directory based on OS
if [ "$OS" = "Darwin" ]; then
    file="ftx-macos"
    platform="mac"
elif [ "$OS" = "Linux" ]; then
    file="ftx-linux"
    platform="lin"
else
    echo "Unsupported operating system: $OS"
    exit 1
fi

toolDir=./tools/bin/$platform/ftx
url="https://github.com/willll/ftx/releases/download/${version}/$file"
target="$toolDir/$file"

if [ ! -d "$toolDir" ]; then
  mkdir -p $toolDir
else
  if [ "$(ls -A $toolDir)" ]; then
    echo "ftx directory is not empty! Proceeding will clear all of its contents."
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

# Rename the binary to just 'ftx'
mv $file ftx

# macOS has no built-in usbreset (it is a Linux usbutils tool), so also install
# dzatona/usbreset-mac next to ftx. It resets by VID:PID (FT245R = 0x0403:0x6001)
# and usually needs sudo.
if [ "$OS" = "Darwin" ]; then
    usbresetVersion="2.0.0"
    case "$(uname -m)" in
        arm64) usbresetArch="arm64" ;;
        x86_64) usbresetArch="x86_64" ;;
        *) usbresetArch="" ;;
    esac

    if [ -z "$usbresetArch" ]; then
        echo "Unsupported architecture for usbreset: $(uname -m)"
    else
        usbresetTar="usbreset-${usbresetVersion}-${usbresetArch}.tar.gz"
        usbresetUrl="https://github.com/dzatona/usbreset-mac/releases/download/v${usbresetVersion}/${usbresetTar}"
        printf "\nInstalling usbreset ${usbresetVersion} (${usbresetArch})\n"
        curl -fsSL -O "$usbresetUrl" && curl -fsSL -O "$usbresetUrl.sha256"
        expected=$(awk '{print $1}' "$usbresetTar.sha256" 2>/dev/null)
        actual=$(shasum -a 256 "$usbresetTar" 2>/dev/null | awk '{print $1}')
        if [ -n "$expected" ] && [ "$expected" = "$actual" ]; then
            usbresetTmp=$(mktemp -d)
            tar xzf "$usbresetTar" -C "$usbresetTmp" && mv "$usbresetTmp/usbreset" ./usbreset
            rm -rf "$usbresetTmp"
        else
            echo "usbreset download or checksum verification failed!"
        fi
        rm -f "$usbresetTar" "$usbresetTar.sha256"
    fi
fi

printf "\nSetting permissions\n";
chmod -R +x .
cd ../../..
