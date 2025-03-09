#!/bin/bash

# Filename for the test file
filename="testfile"

rm $filename
rm tmp

# Size of the file in bytes (512KB)
filesize=$((512 * 1024))


dd if=/dev/zero bs=1 count=$filesize | \
    tr '\0' '\0' | \
    awk 'BEGIN { for (i = 0; i < 256; i++) printf "%c", i }' | \
    dd of=tmp bs=1 seek=0 count=$filesize conv=notrunc

for ((i=0; i<$filesize/256; i++)); do
    cat tmp >> $filename
done

rm tmp

echo "Created $filename with size $filesize bytes."