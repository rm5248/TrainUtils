#!/bin/bash

BUILD_DIR="$1"

cd "$BUILD_DIR"
ARDUINO_LIBRARY_ENABLE_UNSAFE_INSTALL=true arduino-cli lib install --zip-path liblcc-arduino-*.zip
cd $(ls -d liblcc*/)

for x in examples/*; do
	echo ""
	echo ""
	echo "Compiling $x..."
	arduino-cli compile -b arduino:avr:uno $x
done
