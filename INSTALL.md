# Installation Guide

## Pre-built Packages (Recommended)

### macOS

**Option 1: DMG Installer (Easiest)**
1. Download `PlugPerf-1.0.0-Darwin.dmg`
2. Double-click to mount
3. Drag PlugPerf to Applications or desired location
4. Open Terminal and add to PATH:
   ```bash
   echo 'export PATH="/Applications/PlugPerf/bin:$PATH"' >> ~/.zshrc
   source ~/.zshrc
   ```

**Option 2: TGZ Archive**
1. Download `PlugPerf-1.0.0-Darwin.tar.gz`
2. Extract and install:
   ```bash
   tar -xzf PlugPerf-1.0.0-Darwin.tar.gz
   cd PlugPerf-1.0.0-Darwin
   sudo cp -r bin/* /usr/local/bin/
   sudo cp -r share/* /usr/local/share/
   ```

### Verify Installation

```bash
plugperf --help
```

## Building from Source

### Requirements

- CMake 3.21+
- C++17 compiler (Xcode on macOS, GCC/Clang on Linux, MSVC on Windows)
- JUCE 8.0.8+ (optional, will auto-download if not provided)
- Python 3 with matplotlib (for visualization)

### macOS

```bash
# Set JUCE path (optional)
export JUCE_SDK_PATH=/path/to/JUCE

# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Install locally
cd build
sudo make install

# Or create distributable package
cd ..
./package.sh
```

### Linux

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install build-essential cmake libasound2-dev \
  libfreetype6-dev libx11-dev libxrandr-dev libxinerama-dev \
  libxcursor-dev libgl1-mesa-dev

# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Install
cd build
sudo make install

# Or create DEB package
cpack -G DEB
sudo dpkg -i PlugPerf-1.0.0-Linux.deb
```

### Windows

```bash
# Using Visual Studio
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release

# Install
cd build
cmake --install . --config Release

# Or create ZIP package
cpack -G ZIP
```

## Python Dependencies (for visualization)

```bash
pip3 install matplotlib
```

## Uninstall

### macOS/Linux (if installed via make install)

```bash
sudo rm /usr/local/bin/plugperf
sudo rm /usr/local/bin/visualize.py
sudo rm -rf /usr/local/share/doc/plugperf
```

### macOS (if installed via DMG)

Simply delete the PlugPerf folder from Applications.

## Troubleshooting

### "plugperf: command not found"

Make sure the installation directory is in your PATH:
```bash
echo $PATH
```

Add to PATH if needed:
```bash
# macOS/Linux
echo 'export PATH="/usr/local/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

### JUCE not found during build

Either:
1. Set `JUCE_SDK_PATH` environment variable
2. Let CMake auto-download JUCE (slower first build)

### Python visualization not working

Install matplotlib:
```bash
pip3 install matplotlib
```

## Development Build

For development with debug symbols:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/plugperf --help
```
