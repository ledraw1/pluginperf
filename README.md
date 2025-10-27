# PlugPerf

A high-precision performance benchmarking tool for audio plugins, with focus on VST3 format. Designed for plugin developers and audio engineers who need accurate, reproducible performance measurements.

## Features

- **Real-time performance measurement** - Matches Plugin Doctor methodology
- **Statistical validation** - Standard deviation, coefficient of variation, percentile analysis
- **Multiple bit depths** - 32-bit float, 64-bit double
- **Configurable sample rates** - Any rate (44.1kHz, 48kHz, 96kHz, 192kHz, etc.)
- **Measurement consistency checks** - Automatic warnings for outliers and instability
- **Parameter inspection** - Query and manipulate plugin parameters
- **CSV export** - Detailed results for analysis and visualization
- **Visualization tools** - Generate performance charts (PNG)
- **Batch testing** - Automated testing of multiple plugins
- **Cross-validated** - 95% match with Plugin Doctor measurements

## Quick Start

### Building

```bash
# Configure (uses JUCE from JUCE_SDK_PATH environment variable)
JUCE_SDK_PATH=/path/to/JUCE cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release
```

### Basic Usage

```bash
# Benchmark a plugin with default settings
./build/plugperf --plugin /path/to/plugin.vst3

# Save results to CSV
./build/plugperf --plugin /path/to/plugin.vst3 --out results.csv

# Custom buffer sizes and iterations
./build/plugperf --plugin /path/to/plugin.vst3 \
  --buffers 64,128,256,512,1024 \
  --iterations 500
```

## Command Line Options

```
Usage: plugperf --plugin /path/Your.vst3 [options]

Required:
  --plugin PATH            Path to .vst3 bundle to measure

Options:
  --sr HZ                  Sample rate (default: 48000)
                           Examples: 44100, 48000, 96000, 192000
  
  --channels N             Channel count (default: 2)
  
  --bits DEPTH             Bit depth (default: 32f)
                           32f  = 32-bit float
                           64f  = 64-bit double
  
  --buffers CSV            Buffer sizes to test (default: 32,64,128,256,512,1024,2048,4096,8192,16384)
                           Example: --buffers 64,128,256,512
  
  --warmup N               Warmup iterations per buffer size (default: 40)
  
  --iterations N           Measurement iterations per buffer size (default: 400)
  
  --out PATH               Output CSV file path (default: stdout)
  
  -h, --help               Show help message
```

## Real-Time Priority

PlugPerf runs measurements with elevated process priority (`Process::RealtimePriority`) to minimize interference from other system processes. This provides:

- **Reduced measurement variance** - Less jitter from OS scheduling
- **More consistent results** - Better repeatability across runs
- **DAW-like conditions** - Matches how audio applications run
- **Continuous priority** - Set once at startup, maintained throughout all measurements

The implementation uses process-level priority on the main thread, which is simpler and more reliable for a command-line tool than creating separate real-time threads. Priority is set once after plugin loading and maintained for the entire test session, avoiding repeated priority changes that could affect measurements.

See `docs/REALTIME_THREAD_RESEARCH.md` for detailed research on real-time audio thread implementation.

## Output Metrics

Each test produces the following metrics per buffer size:

| Metric | Description |
|--------|-------------|
| `mean_us` | Mean processing time in microseconds |
| `median_us` | Median processing time (50th percentile) |
| `p95_us` | 95th percentile processing time |
| `min_us` | Minimum processing time |
| `max_us` | Maximum processing time |
| `std_dev_us` | Standard deviation of processing time |
| `cv_pct` | Coefficient of variation (relative std dev %) |
| `approx_rt_cpu_pct` | Real-time CPU usage (% of buffer time window) |
| `dsp_load_pct` | DSP load (per-sample processing cost %) |
| `latency_samples` | Plugin-reported latency in samples |

### Interpreting Results

**Coefficient of Variation (CV)**
- < 5%: Excellent measurement stability
- < 10%: Good stability
- < 20%: Fair stability
- ≥ 20%: High variation (consider more iterations)

**Real-time CPU %**
- Should stay relatively constant across buffer sizes
- < 100%: Plugin can run in real-time
- ≥ 100%: Plugin cannot keep up (will cause dropouts)

## Tools

### Visualization

Generate performance charts from CSV results:

```bash
python3 tools/visualize.py results.csv -o performance_chart.png
```

Creates a 4-panel visualization showing:
1. Processing time vs buffer size (with error bars)
2. Real-time CPU load
3. Measurement stability (CV %)
4. Percentile distribution (min/median/p95)

### Python Wrapper

Use `run_measure.py` for a more convenient interface with summary output:

```bash
python3 tools/run_measure.py \
  --plugin /path/to/plugin.vst3 \
  --sr 48000 \
  --channels 2 \
  --buffers 64,128,256,512,1024 \
  --iterations 200 \
  --out results.csv
```

This wrapper:
- Validates plugin path before running
- Provides progress feedback
- Displays a summary table after completion
- Automatically finds the plugperf binary

### Batch Testing

Test all plugins in your system VST3 folder:

```bash
python3 tools/test_all_plugins.py \
  --vst3-dir /Library/Audio/Plug-Ins/VST3 \
  --output-dir ./plugin_results \
  --buffers 64,256,1024,4096 \
  --iterations 200 \
  --skip-errors
```

Features:
- Automatically discovers all VST3 plugins
- Tests each plugin with configurable settings
- Saves individual CSV results per plugin
- Generates summary report with statistics
- Optional `--skip-errors` to continue on failures
- 5-minute timeout per plugin

### Parameter Inspection

Inspect and manipulate plugin parameters with `plugparams`:

```bash
# List all parameters
./build/plugparams --plugin plugin.vst3

# Detailed view with ranges and values
./build/plugparams --plugin plugin.vst3 --verbose

# Read → Change → Validate workflow
./build/plugparams --plugin plugin.vst3 \
  --get "Gain" --get "Mix"                    # Read current
  
./build/plugparams --plugin plugin.vst3 \
  --set "Gain=0.75" --set "Mix=0.5" \         # Change
  --get "Gain" --get "Mix"                    # Validate

# JSON output for automation
./build/plugparams --plugin plugin.vst3 --json
```

Features:
- Classifies parameters as Boolean, Discrete, or Continuous
- Shows ranges, possible values, and current settings
- Set parameters by index, name, or ID
- Combine `--set` and `--get` to validate changes
- JSON output for programmatic use
- See [docs/PLUGIN_PARAMETERS.md](docs/PLUGIN_PARAMETERS.md) for details

## Examples

### Compare bit depths

```bash
# Test 32-bit float
./build/plugperf --plugin plugin.vst3 --bits 32f --out results_32f.csv

# Test 64-bit double
./build/plugperf --plugin plugin.vst3 --bits 64f --out results_64f.csv
```

### Test at different sample rates

```bash
# 44.1 kHz
./build/plugperf --plugin plugin.vst3 --sr 44100 --out results_44k.csv

# 96 kHz
./build/plugperf --plugin plugin.vst3 --sr 96000 --out results_96k.csv
```

### Quick test with fewer iterations

```bash
./build/plugperf --plugin plugin.vst3 \
  --buffers 256,512,1024 \
  --iterations 100 \
  --warmup 20
```

### Test with different parameter settings

```bash
# Test compressor at different ratios
for ratio in 0.0 0.33 0.67 1.0; do
  ./build/plugparams --plugin compressor.vst3 --set "Ratio=$ratio"
  ./build/plugperf --plugin compressor.vst3 \
    --buffers 1024 --iterations 200 \
    --out "results_ratio_${ratio}.csv"
done

# Test reverb with different decay times
for decay in 0.0 0.5 1.0; do
  ./build/plugparams --plugin reverb.vst3 --set "Decay=$decay"
  ./build/plugperf --plugin reverb.vst3 \
    --buffers 1024 --iterations 200 \
    --out "results_decay_${decay}.csv"
done
```

## Validation

### Synthetic Test Plugin

A synthetic test plugin is included for validation:

```bash
# Build synthetic plugin
cd synthetic_plugin
mkdir build && cd build
JUCE_SDK_PATH=/path/to/JUCE cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .

# Test passthrough baseline (measurement overhead only)
cd ../..
./build/plugperf --plugin "synthetic_plugin/build/SyntheticTestPlugin_artefacts/Release/VST3/Synthetic Test Plugin.vst3"
```

Expected baseline: ~0.16-1.1 μs (negligible overhead)

### Cross-validation with Plugin Doctor

PlugPerf measurements have been validated against Plugin Doctor:
- Average match: 95% (0.871x to 1.041x ratio)
- Nearly identical at buffer sizes ≥ 4096
- Both tools use real-time processing mode

## Measurement Consistency Warnings

PlugPerf automatically detects measurement issues:

```
WARNING [buffer=64]: High coefficient of variation - CV=31.99% (consider more iterations or warmup)
WARNING [buffer=256]: High outlier ratio - p95/median=3.2 (suggests measurement instability)
WARNING [buffer=512]: Sanity check failed - min=10.5 median=8.2 mean=9.1
```

These warnings help identify:
- Insufficient warmup/iterations
- System interference
- Plugin instability
- Measurement errors

## Project Structure

```
plugperf/
├── src/
│   ├── main.cpp           # Main benchmark engine
│   ├── argparse.hpp       # Command-line argument parsing
│   └── csv.hpp            # CSV output writer
├── tools/
│   └── visualize.py       # Visualization script
├── synthetic_plugin/      # Validation test plugin
│   ├── Source/
│   │   ├── PluginProcessor.h
│   │   └── PluginProcessor.cpp
│   └── CMakeLists.txt
├── CMakeLists.txt
└── README.md
```

## Requirements

- **CMake** 3.21 or later
- **JUCE** 8.0.8 or later (via JUCE_SDK_PATH or FetchContent)
- **C++17** compiler
- **Python 3** (for visualization, requires matplotlib)

### Installing Python dependencies

```bash
pip3 install matplotlib
```

## Troubleshooting

### Plugin fails to load

```
CreatePluginInstance failed for /path/to/plugin.vst3
Reason: Unable to load VST-3 plug-in file
```

**Solutions:**
- Verify the .vst3 bundle path is correct
- Check plugin is compatible with your system architecture
- Ensure plugin doesn't require additional dependencies

### High CV warnings

```
WARNING: High coefficient of variation - CV=25%
```

**Solutions:**
- Increase `--iterations` (try 500-1000)
- Increase `--warmup` (try 100-200)
- Close other applications to reduce system noise
- Use larger buffer sizes (more stable measurements)

### Measurements don't match Plugin Doctor

**Check:**
- Are you using the same sample rate?
- Are you using the same buffer sizes?
- Is the plugin in the same state/preset?
- Both tools should show similar trends, even if absolute values differ slightly

## Distribution

### Creating Packages

To create distributable installers:

```bash
# Quick package script (creates DMG and TGZ)
./package.sh

# Or manually:
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
cd build
cpack -G DragNDrop  # Creates .dmg for macOS
cpack -G TGZ        # Creates .tar.gz archive
```

Packages will be created in `build/`:
- `PlugPerf-1.0.0-Darwin.dmg` - macOS drag & drop installer (~14MB)
- `PlugPerf-1.0.0-Darwin.tar.gz` - Compressed archive (~17MB)

### Installation

See [INSTALL.md](INSTALL.md) for detailed installation instructions.

## Contributing

See `backlog.md` for planned features and development roadmap.

## License

MIT License - See [LICENSE](LICENSE) for details.

## Acknowledgments

- Built with [JUCE](https://juce.com/) framework
- Validated against [Plugin Doctor](https://ddmf.eu/plugindoctor/)
- Inspired by the need for accurate, reproducible plugin performance measurements
