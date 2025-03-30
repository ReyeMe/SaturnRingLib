version=$1
compilerDir=./Compiler
tag="$version"

if [ "$version" = "14.2.0" ]; then
  tag="gcc_${version}_2"
fi

url="https://github.com/willll/Saturn-SDK-GCC-SH2/releases/download/${tag}/sh-gcc-${version}.zip"
file="sh-gcc-$version.zip"
target="$compilerDir/sh-gcc-$version.zip"

if [ ! -d "$compilerDir" ]; then
  mkdir $compilerDir
else
  if [ ! -d "ls -A $compilerDir" ]; then
    echo "Compiler directory is not empty! Proceeding will clear all of its contents."
    read -r -p "Are you sure? [y/N] " response

    case "$response" in
    [yY][eE][sS]|[yY]) 
        rm -rf $compilerDir
        ;;
    *)
        exit
        ;;
    esac
    
    mkdir $compilerDir
  fi
fi

cd $compilerDir
wget $url # -q --show-progress

if [ ! -f $file ]; then
  echo "Installation failed!";
  exit
fi

printf "Extracting";
if ! command -v pv 2>&1 >/dev/null
then
  unzip $file | awk 'BEGIN {ORS=""} {if(NR%10==0)print "."}'
else
  unzip $file | pv -l >/dev/null
fi

printf "\nCleaning up...\n";
rm -f $file
cd ..

printf "\nSetting permissions\n";
chmod -R +x $compilerDir
