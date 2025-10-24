# Batch Testing Guide

## Overview

The `test_all_plugins.py` script automates performance testing across all VST3 plugins in your system.

## Quick Start

Test all plugins with default settings:

```bash
python3 tools/test_all_plugins.py
```

This will:
1. Scan `/Library/Audio/Plug-Ins/VST3` for plugins
2. Test each plugin at buffer sizes: 64, 256, 1024, 4096
3. Run 200 iterations per buffer size
4. Save results to `./plugin_results/`
5. Generate a summary report

## Common Usage Patterns

### Fast Test (Fewer Iterations)

```bash
python3 tools/test_all_plugins.py \
  --iterations 50 \
  --buffers 256,1024 \
  --output-dir ./quick_test
```

### Comprehensive Test

```bash
python3 tools/test_all_plugins.py \
  --iterations 400 \
  --buffers 32,64,128,256,512,1024,2048,4096,8192 \
  --output-dir ./comprehensive_test
```

### Test Specific Folder

```bash
python3 tools/test_all_plugins.py \
  --vst3-dir ~/Library/Audio/Plug-Ins/VST3 \
  --output-dir ./user_plugins
```

### Continue on Errors

Some plugins may fail to load or crash. Use `--skip-errors` to continue testing:

```bash
python3 tools/test_all_plugins.py --skip-errors
```

### Double Precision Testing

```bash
python3 tools/test_all_plugins.py \
  --bits 64f \
  --output-dir ./double_precision_test
```

## Output Structure

```
plugin_results/
├── summary.txt              # Human-readable summary
├── summary.json             # Machine-readable summary
├── Plugin_Name_1.csv        # Individual plugin results
├── Plugin_Name_2.csv
└── ...
```

### Summary Report Format

**summary.txt** contains:
- Total plugins tested
- Success/failure counts
- Table of successful tests with average DSP load and CV%
- List of failed tests with error messages

**summary.json** contains:
- All test configuration
- Detailed results for each plugin
- Timestamps
- Error information

## Example Summary Output

```
==========================================================================================
PLUGPERF BATCH TEST SUMMARY
==========================================================================================
Generated: 2025-10-24 15:30:00
Total plugins tested: 43
Successful: 40
Failed: 3

SUCCESSFUL TESTS
------------------------------------------------------------------------------------------
Plugin                                              Avg DSP%     Avg CV%      Buffers   
------------------------------------------------------------------------------------------
UADx Capitol Chambers                               4.79         18.52        4         
Surge XT                                            2.34         12.45        4         
Plugin Doctor                                       1.23         8.91         4         
...

FAILED TESTS
------------------------------------------------------------------------------------------
Broken Plugin: Timeout
Crash Plugin: Exception in thread
...
```

## Performance Considerations

### Time Estimates

With default settings (200 iterations, 4 buffer sizes):
- Fast plugin: ~10-30 seconds
- Average plugin: ~30-60 seconds
- Heavy plugin: ~60-120 seconds

For 43 plugins:
- Estimated total time: 30-90 minutes
- With `--iterations 50`: 10-30 minutes

### Timeout Protection

Each plugin has a 5-minute timeout to prevent hanging. If a plugin times out, it will be marked as failed and the script continues (with `--skip-errors`).

## Troubleshooting

### Plugin Fails to Load

Some plugins may require:
- iLok authorization
- Hardware dongles
- Internet connection
- Specific system configuration

These will fail with an error message. Use `--skip-errors` to continue.

### High Failure Rate

If many plugins fail:
1. Check that plugperf binary exists: `./build/plugperf`
2. Verify VST3 folder path is correct
3. Try testing a single known-good plugin first
4. Check system permissions

### Memory Issues

Testing many heavy plugins may consume significant RAM. Consider:
- Testing in smaller batches
- Closing other applications
- Using `--iterations 50` for faster tests

## Integration with CI/CD

The script returns:
- Exit code 0: All tests passed
- Exit code 1: One or more tests failed

Example GitHub Actions workflow:

```yaml
- name: Test all plugins
  run: |
    python3 tools/test_all_plugins.py \
      --iterations 50 \
      --skip-errors \
      --output-dir ./test_results
    
- name: Upload results
  uses: actions/upload-artifact@v3
  with:
    name: plugin-test-results
    path: ./test_results/
```

## Advanced Usage

### Custom Test Configuration

Create a shell script for repeated testing:

```bash
#!/bin/bash
# test_suite.sh

python3 tools/test_all_plugins.py \
  --vst3-dir /Library/Audio/Plug-Ins/VST3 \
  --output-dir "./results/$(date +%Y%m%d_%H%M%S)" \
  --buffers 64,256,1024,4096 \
  --iterations 200 \
  --warmup 40 \
  --sr 48000 \
  --channels 2 \
  --bits 32f \
  --skip-errors
```

### Parsing Results Programmatically

```python
import json

with open('plugin_results/summary.json', 'r') as f:
    results = json.load(f)

# Find plugins with high DSP load
heavy_plugins = [
    (name, data['mean_dsp_load'])
    for name, data in results['successful'].items()
    if data['mean_dsp_load'] > 10.0
]

print("Heavy plugins (>10% DSP):")
for name, load in sorted(heavy_plugins, key=lambda x: x[1], reverse=True):
    print(f"  {name}: {load:.2f}%")
```

## Best Practices

1. **Start with a quick test** - Use `--iterations 50` first to verify everything works
2. **Use --skip-errors** - Don't let one bad plugin stop the entire test suite
3. **Save results with timestamps** - Use dated output directories for comparison
4. **Test during off-hours** - Batch testing can take time and use CPU
5. **Document failures** - Keep notes on which plugins consistently fail and why
6. **Compare over time** - Run tests after system updates to catch regressions

## See Also

- [README.md](../README.md) - Main documentation
- [run_measure.py](../tools/run_measure.py) - Single plugin testing
- [visualize.py](../tools/visualize.py) - Results visualization
