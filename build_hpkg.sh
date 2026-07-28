#!/bin/bash
set -e

# Make sure binary is built
make

STAGE_DIR="_hpkg_stage"
OUTPUT_HPKG="midimonitor-2.0.0-x86_64.hpkg"

echo "Assembling Haiku package directory structure..."
rm -rf "$STAGE_DIR"
mkdir -p "$STAGE_DIR/apps"
mkdir -p "$STAGE_DIR/data/deskbar/menu/Applications"

# Find compiled binary inside objects.* directory or current directory
BINARY_PATH=$(find objects.* -name MidiMonitor -type f 2>/dev/null | head -n 1)
if [ -z "$BINARY_PATH" ]; then
	BINARY_PATH="MidiMonitor"
fi

if [ ! -f "$BINARY_PATH" ]; then
	echo "Error: MidiMonitor binary not found!"
	exit 1
fi

cp "$BINARY_PATH" "$STAGE_DIR/apps/MidiMonitor"

# Create Deskbar menu symlink
ln -s "../../../apps/MidiMonitor" "$STAGE_DIR/data/deskbar/menu/Applications/MidiMonitor"

# Copy package info
cp PackageInfo "$STAGE_DIR/.PackageInfo"

# Create the hpkg file
echo "Creating package $OUTPUT_HPKG..."
package create -C "$STAGE_DIR" "$OUTPUT_HPKG"

rm -rf "$STAGE_DIR"

echo "Package created successfully: $OUTPUT_HPKG"
