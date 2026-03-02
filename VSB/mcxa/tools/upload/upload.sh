#!/bin/bash

BLHOST="$1"
FIRMWARE="$2"
VIDPID="0x1fc9,0x015e"

echo "--- MCXArduino Flashing Script ---"

# 1. Erase All
echo "1/3 Erasing flash..."
"$BLHOST" -u $VIDPID -- flash-erase-all
if [ $? -ne 0 ]; then echo "Error erasing flash"; exit 1; fi

# 2. Write Memory
echo "2/3 Writing firmware..."
"$BLHOST" -u $VIDPID -- write-memory 0x0 "$FIRMWARE"
if [ $? -ne 0 ]; then echo "Error writing memory"; exit 1; fi

# 3. Reset
echo "3/3 Resetting target..."
"$BLHOST" -u $VIDPID -- reset
if [ $? -ne 0 ]; then echo "Error resetting"; exit 1; fi

echo "--- Upload Successful ---"