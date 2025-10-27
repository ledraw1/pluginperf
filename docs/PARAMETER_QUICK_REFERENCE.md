# Parameter Quick Reference

## Common Commands

### List Parameters
```bash
# Compact table
plugparams --plugin plugin.vst3

# Detailed info
plugparams --plugin plugin.vst3 --verbose

# JSON format
plugparams --plugin plugin.vst3 --json
```

### Read Parameters
```bash
# Single parameter
plugparams --plugin plugin.vst3 --get "Gain"

# Multiple parameters
plugparams --plugin plugin.vst3 --get "Gain" --get "Mix" --get "Bypass"
```

### Set Parameters
```bash
# Single parameter
plugparams --plugin plugin.vst3 --set "Gain=0.75"

# Multiple parameters
plugparams --plugin plugin.vst3 --set "Gain=0.75" --set "Mix=0.5"

# By index
plugparams --plugin plugin.vst3 --set 0=0.75
```

### Read → Change → Validate
```bash
# All in one command
plugparams --plugin plugin.vst3 \
  --set "Gain=0.75" \
  --set "Mix=0.5" \
  --get "Gain" \
  --get "Mix"
```

## Parameter Types

### Boolean (On/Off)
- **Values**: 0.0 (Off) or 1.0 (On)
- **Example**: `--set "Bypass=0.0"` (Off)

### Discrete (ComboBox/Stepped)
- **Values**: Normalized 0.0-1.0 mapped to steps
- **Example**: 4 steps → 0.0, 0.33, 0.67, 1.0
- **Use `--verbose` to see all values**

### Continuous (Slider)
- **Values**: Any value 0.0-1.0
- **Example**: `--set "Gain=0.75"`

## Discrete Parameter Mapping

For N steps, normalized values map as:
```
Step 0: 0.0
Step 1: 1/(N-1)
Step 2: 2/(N-1)
...
Step N-1: 1.0
```

### Examples

**4 Steps** (e.g., Ratio: 2:1, 4:1, 8:1, 12:1):
- 0.00 → Step 0 (2:1)
- 0.33 → Step 1 (4:1)
- 0.67 → Step 2 (8:1)
- 1.00 → Step 3 (12:1)

**3 Steps** (e.g., Meter: Input, GR, Output):
- 0.0 → Step 0 (Input)
- 0.5 → Step 1 (GR)
- 1.0 → Step 2 (Output)

**7 Steps** (e.g., Headroom: 4dB, 8dB, 12dB, 16dB, 20dB, 24dB, 28dB):
- 0.00 → Step 0 (4dB)
- 0.17 → Step 1 (8dB)
- 0.33 → Step 2 (12dB)
- 0.50 → Step 3 (16dB)
- 0.67 → Step 4 (20dB)
- 0.83 → Step 5 (24dB)
- 1.00 → Step 6 (28dB)

## Integration with PlugPerf

### Basic Workflow
```bash
# 1. Discover parameters
plugparams --plugin plugin.vst3 --verbose > params.txt

# 2. Set desired state
plugparams --plugin plugin.vst3 --set "Gain=0.75" --set "Mix=0.5"

# 3. Run benchmark
plugperf --plugin plugin.vst3 --buffers 1024 --iterations 200 --out results.csv
```

### Parameter Sweep
```bash
# Test at different settings
for value in 0.0 0.25 0.5 0.75 1.0; do
  plugparams --plugin plugin.vst3 --set "Gain=$value"
  plugperf --plugin plugin.vst3 \
    --buffers 1024 --iterations 200 \
    --out "results_gain_${value}.csv"
done
```

### Discrete Parameter Sweep
```bash
# Test all ratio settings (4 steps)
for ratio in 0.0 0.33 0.67 1.0; do
  plugparams --plugin compressor.vst3 \
    --set "Ratio=$ratio" \
    --get "Ratio"  # Verify setting
  
  plugperf --plugin compressor.vst3 \
    --buffers 1024 --iterations 200 \
    --out "results_ratio_${ratio}.csv"
done
```

## JSON Processing

### Extract Boolean Parameters
```bash
plugparams --plugin plugin.vst3 --json | \
  jq '.parameters[] | select(.type == "Boolean") | {name, current: .currentValue}'
```

### Extract Discrete Parameters with Values
```bash
plugparams --plugin plugin.vst3 --json | \
  jq '.parameters[] | select(.type == "Discrete") | {name, steps: .numSteps, values}'
```

### Extract Continuous Parameters with Ranges
```bash
plugparams --plugin plugin.vst3 --json | \
  jq '.parameters[] | select(.type == "Continuous") | {name, min: .minValue, max: .maxValue}'
```

### Export Current State
```bash
# Save all current values
plugparams --plugin plugin.vst3 --json | \
  jq '.parameters[] | {name, value: .currentValue}' > state.json
```

## Troubleshooting

### Parameter Not Found
```bash
# List all parameter names
plugparams --plugin plugin.vst3 | grep -v "MIDI CC"

# Search for parameter
plugparams --plugin plugin.vst3 --json | jq '.parameters[] | select(.name | contains("Gain"))'
```

### Verify Discrete Values
```bash
# Use verbose mode to see all possible values
plugparams --plugin plugin.vst3 --verbose | grep -A10 "Parameter #0"
```

### Check Parameter Type
```bash
# Get type information
plugparams --plugin plugin.vst3 --json | \
  jq '.parameters[] | select(.name == "Gain") | {name, type, steps: .numSteps}'
```

## See Also

- [PLUGIN_PARAMETERS.md](PLUGIN_PARAMETERS.md) - Complete documentation
- [README.md](../README.md) - Main PlugPerf documentation
- [BATCH_TESTING.md](BATCH_TESTING.md) - Automated testing guide
