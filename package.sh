#!/bin/bash
# Package PlugPerf for distribution

set -e

echo "==================================="
echo "PlugPerf Packaging Script"
echo "==================================="
echo ""

# Clean previous builds
echo "Cleaning previous builds..."
rm -rf build package

# Configure
echo "Configuring..."
JUCE_SDK_PATH="${JUCE_SDK_PATH:-$HOME/SDKs/JUCE}"
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local

# Build
echo "Building..."
JOBS=${JOBS:-}
if [[ -z "$JOBS" ]]; then
  if command -v sysctl >/dev/null 2>&1; then
    JOBS=$(sysctl -n hw.ncpu 2>/dev/null || echo 1)
  elif command -v nproc >/dev/null 2>&1; then
    JOBS=$(nproc 2>/dev/null || echo 1)
  elif command -v getconf >/dev/null 2>&1; then
    JOBS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)
  else
    JOBS=1
  fi
fi
if ! [[ "$JOBS" =~ ^[0-9]+$ ]] || [[ "$JOBS" -lt 1 ]]; then
  JOBS=1
fi
cmake --build build --config Release -j"$JOBS"

# Create package
echo "Creating package..."
cd build
cpack -G DragNDrop
cpack -G TGZ
cd ..

# List created packages
echo ""
echo "==================================="
echo "Packages created:"
echo "==================================="
ls -lh build/PlugPerf-*.dmg build/PlugPerf-*.tar.gz 2>/dev/null || true

echo ""
echo "✓ Packaging complete!"
echo ""
echo "To install locally:"
echo "  cd build && sudo make install"
echo ""
echo "To distribute:"
echo "  - DMG: build/PlugPerf-1.0.0-Darwin.dmg (drag & drop installer)"
echo "  - TGZ: build/PlugPerf-1.0.0-Darwin.tar.gz (command-line install)"
