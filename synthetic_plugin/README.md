# Synthetic Test Plugin

A minimal JUCE VST3 plugin for validating plugperf measurements.

## Purpose

This plugin provides **controllable, predictable CPU load** for testing measurement accuracy:
- Known processing characteristics
- Adjustable CPU load via parameters
- No buffer size dependencies
- Ideal for validation and calibration

## Parameters

1. **CPU Load** (0-1000): Number of `sin()` calculations per sample
   - 0 = passthrough (minimal CPU)
   - 100 = light load (~100 sin() per sample)
   - 1000 = heavy load (~1000 sin() per sample)

2. **Delay** (0-1000 μs): Artificial delay in microseconds
   - Simulates fixed processing time
   - Independent of buffer size
   - Useful for testing timing accuracy

## Building

```bash
cd synthetic_plugin
mkdir build && cd build
JUCE_SDK_PATH=/Users/ryanwardell/SDKs/JUCE cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

The plugin will be built to: `build/SyntheticTestPlugin_artefacts/Release/VST3/Synthetic Test Plugin.vst3`

## Usage with PlugPerf

```bash
# Test with minimal load (passthrough)
./plugperf --plugin "synthetic_plugin/build/.../Synthetic Test Plugin.vst3"

# Expected: Very low CPU usage, consistent across buffer sizes

# Test with known load
# (Set CPU Load parameter to 100 via preset or parameter control)
# Expected: Proportional increase in processing time
```

## Validation Tests

1. **Passthrough Test**: CPU Load=0, Delay=0
   - Should show minimal processing time
   - Establishes measurement baseline

2. **Fixed Delay Test**: CPU Load=0, Delay=100μs
   - Should show ~100μs processing time
   - Independent of buffer size
   - Validates timing accuracy

3. **Linear Scaling Test**: CPU Load=100
   - Processing time should scale linearly with buffer size
   - Validates per-sample measurement

## Notes

- This plugin is for **testing only**, not for audio processing
- CPU load is intentionally inefficient (sin() calculations)
- Delay uses `sleep_for()` which may have OS-dependent precision
