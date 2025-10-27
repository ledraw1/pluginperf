# Plugin Parameter Inspector (`plugparams`)

## Overview

`plugparams` is a command-line tool for inspecting and manipulating VST3 plugin parameters. It provides detailed information about parameter types, ranges, possible values, and allows setting parameters programmatically.

## Features

- **Parameter Discovery** - List all parameters with their properties
- **Type Classification** - Identifies Boolean, Discrete (ComboBox), and Continuous (Slider) parameters
- **Range Information** - Shows min/max values for continuous parameters
- **Discrete Values** - Lists all possible values for discrete/choice parameters
- **Parameter Setting** - Set parameters by index, name, or ID
- **JSON Output** - Machine-readable format for automation
- **Verbose Mode** - Detailed parameter information

## Usage

### Basic Syntax

```bash
plugparams --plugin <path> [options]
```

### List All Parameters

```bash
# Compact table view
plugparams --plugin plugin.vst3

# Detailed verbose view
plugparams --plugin plugin.vst3 --verbose
```

### Get Specific Parameter

```bash
# By index
plugparams --plugin plugin.vst3 --get 0

# By name
plugparams --plugin plugin.vst3 --get "Gain"
plugparams --plugin plugin.vst3 --get "Mix"

# Get multiple parameters
plugparams --plugin plugin.vst3 --get "Gain" --get "Mix" --get "Bypass"
```

### Set Parameters

```bash
# Set single parameter (normalized 0.0-1.0)
plugparams --plugin plugin.vst3 --set "Gain=0.5"
plugparams --plugin plugin.vst3 --set 0=0.75

# Set multiple parameters
plugparams --plugin plugin.vst3 \
  --set "Gain=0.5" \
  --set "Mix=1.0" \
  --set "Bypass=0.0"
```

### Read → Change → Validate Workflow

Combine `--set` and `--get` in a single command to validate changes:

```bash
# Read current settings
plugparams --plugin plugin.vst3 --get "Gain" --get "Mix"
# Output: Gain = 0.5 (0.500)
#         Mix = 1.0 (1.000)

# Change and validate in one command
plugparams --plugin plugin.vst3 \
  --set "Gain=0.75" \
  --set "Mix=0.5" \
  --get "Gain" \
  --get "Mix"
# Output: ✓ Set 'Gain' = 0.75
#         ✓ Set 'Mix' = 0.5
#         Gain = 0.75 (0.750)  ← Validated
#         Mix = 0.5 (0.500)    ← Validated
```

### JSON Output

```bash
# Get all parameters in JSON format
plugparams --plugin plugin.vst3 --json

# Pipe to jq for processing
plugparams --plugin plugin.vst3 --json | jq '.parameters[] | select(.type == "Boolean")'
```

## Parameter Types

### Boolean
On/Off switches, bypass controls, etc.
- **Steps**: 2
- **Values**: Off (0.0) / On (1.0)
- **Example**: "Bypass", "Power", "Wet Solo"

### Discrete
ComboBox selections, stepped controls
- **Steps**: 2-999
- **Values**: Named options (e.g., "Chamber A", "Chamber B", "Chamber C")
- **Example**: "Reverb Type", "Filter Mode", "Waveform"

### Continuous
Sliders, knobs with smooth ranges
- **Steps**: 0 or ≥1000
- **Range**: Min to Max (often 0.0-1.0 normalized)
- **Example**: "Gain", "Frequency", "Mix"

## Output Formats

### Compact Table

```
#    Name                           Type         Current    Range/Values         Label
---------------------------------------------------------------------------------------------
0    Chambers                       Discrete     0.333      4 steps              
1    Microphones                    Discrete     0.667      4 steps              
2    Position                       Continuous   1.000      0.00 - 1.00          
3    Predelay                       Continuous   0.000      0.00 - 1.00          
4    Decay                          Continuous   1.000      0.00 - 1.00          
5    Mix                            Continuous   0.500      0.00 - 1.00          
10   Wet Solo                       Boolean      1.000      Off / On             
12   Power                          Boolean      1.000      Off / On             
```

### Verbose Mode

```
Parameter #0
---------------------------------------------------------------------------------------------
  Name:         Chambers
  ID:           Chambers
  Type:         Discrete
  Current:      0.333333 (4)
  Default:      0.333333
  Steps:        4
  Values:       2, 4, 6, 7
  Automatable:  Yes

Parameter #2
---------------------------------------------------------------------------------------------
  Name:         Position
  ID:           Position
  Type:         Continuous
  Current:      1.000 (1.000)
  Default:      1.000
  Range:        0.00 - 1.00
  Automatable:  Yes
```

### JSON Format

```json
{
  "parameters": [
    {
      "index": 0,
      "id": "Chambers",
      "name": "Chambers",
      "type": "Discrete",
      "currentValue": 0.333333,
      "currentValueText": "4",
      "defaultValue": 0.333333,
      "label": "",
      "numSteps": 4,
      "values": ["2", "4", "6", "7"],
      "automatable": true,
      "metaParameter": false
    },
    {
      "index": 2,
      "id": "Position",
      "name": "Position",
      "type": "Continuous",
      "currentValue": 1.0,
      "currentValueText": "1.000",
      "defaultValue": 1.0,
      "label": "",
      "minValue": 0.0,
      "maxValue": 1.0,
      "automatable": true,
      "metaParameter": false
    }
  ]
}
```

## Examples

### Real-World Example: UADx 176 Compressor

```bash
# Discover discrete parameter values
plugparams --plugin /Library/Audio/Plug-Ins/VST3/uaudio_176.vst3 --verbose | grep -A10 "Parameter #0"
# Output shows: Ratio has 4 steps: 2:1, 4:1, 8:1, 12:1

# Read current settings
plugparams --plugin /Library/Audio/Plug-Ins/VST3/uaudio_176.vst3 \
  --get "Ratio" --get "Input" --get "Mix"
# Output: Ratio = 0 (2:1)
#         Input = 0.4 (0.400)
#         Mix = 1 (1.000)

# Change to aggressive compression settings and validate
plugparams --plugin /Library/Audio/Plug-Ins/VST3/uaudio_176.vst3 \
  --set "Ratio=1.0" \
  --set "Input=0.75" \
  --set "Mix=0.5" \
  --get "Ratio" --get "Input" --get "Mix"
# Output: ✓ Set 'Ratio' = 1
#         ✓ Set 'Input' = 0.75
#         ✓ Set 'Mix' = 0.5
#         Ratio = 1 (12:1)      ← Changed from 2:1 to 12:1
#         Input = 0.75 (0.750)  ← Changed from 0.4 to 0.75
#         Mix = 0.5 (0.500)     ← Changed from 1.0 to 0.5

# Now benchmark with these settings
plugperf --plugin /Library/Audio/Plug-Ins/VST3/uaudio_176.vst3 \
  --buffers 1024 --iterations 200 --out results_ratio_12to1.csv
```

### Setting Discrete Parameters

For discrete parameters, use normalized values (0.0-1.0) that map to steps:

```bash
# UADx 176 Ratio parameter (4 steps):
# 0.00 → 2:1
# 0.33 → 4:1
# 0.67 → 8:1
# 1.00 → 12:1

plugparams --plugin plugin.vst3 --set "Ratio=0.67"  # Sets to 8:1
plugparams --plugin plugin.vst3 --get "Ratio"       # Verify: Ratio = 0.67 (8:1)

# UADx 176 Meter parameter (3 steps):
# 0.0 → Input
# 0.5 → GR (Gain Reduction)
# 1.0 → Output

plugparams --plugin plugin.vst3 --set "Meter=0.0"   # Sets to Input
plugparams --plugin plugin.vst3 --get "Meter"       # Verify: Meter = 0 (Input)
```

### Find All Boolean Parameters

```bash
plugparams --plugin plugin.vst3 --json | \
  jq '.parameters[] | select(.type == "Boolean") | {index, name, current: .currentValue}'
```

### Set Multiple Parameters and Verify

```bash
# Set parameters
plugparams --plugin plugin.vst3 \
  --set "Gain=0.75" \
  --set "Mix=0.5" \
  --set "Bypass=0.0"

# Verify settings
plugparams --plugin plugin.vst3 \
  --get "Gain" \
  --get "Mix" \
  --get "Bypass"
```

### Export Parameter State

```bash
# Export all current values to JSON
plugparams --plugin plugin.vst3 --json > plugin_state.json

# Extract just the parameter values
jq '.parameters[] | {name, value: .currentValue}' plugin_state.json
```

### Find Parameters by Type

```bash
# List all discrete parameters
plugparams --plugin plugin.vst3 --json | \
  jq '.parameters[] | select(.type == "Discrete") | .name'

# List all continuous parameters with ranges
plugparams --plugin plugin.vst3 --json | \
  jq '.parameters[] | select(.type == "Continuous") | {name, min: .minValue, max: .maxValue}'
```

### Automation Script Example

```bash
#!/bin/bash
# Test plugin at different mix levels

PLUGIN="plugin.vst3"
OUTPUT_DIR="./mix_sweep"

for mix in 0.0 0.25 0.5 0.75 1.0; do
  echo "Testing Mix=$mix"
  
  # Set mix parameter
  plugparams --plugin "$PLUGIN" --set "Mix=$mix"
  
  # Run benchmark
  plugperf --plugin "$PLUGIN" \
    --buffers 1024 \
    --iterations 200 \
    --out "$OUTPUT_DIR/mix_${mix}.csv"
done
```

## Integration with PlugPerf

The `plugparams` tool is designed to work alongside `plugperf` for comprehensive plugin testing:

1. **Inspect Parameters** - Use `plugparams` to discover available parameters
2. **Set Test Conditions** - Configure plugin state before benchmarking
3. **Run Benchmarks** - Use `plugperf` to measure performance
4. **Repeat** - Test different parameter configurations

### Example Workflow

```bash
# 1. Discover parameters
plugparams --plugin reverb.vst3 --verbose > reverb_params.txt

# 2. Test with different decay settings
for decay in 0.0 0.5 1.0; do
  plugparams --plugin reverb.vst3 --set "Decay=$decay"
  plugperf --plugin reverb.vst3 \
    --buffers 1024 \
    --iterations 200 \
    --out "results_decay_${decay}.csv"
done

# 3. Analyze results
python3 tools/visualize.py results_decay_*.csv
```

## Programmatic Use

### Python Example

```python
import subprocess
import json

def get_plugin_parameters(plugin_path):
    """Get all plugin parameters as JSON."""
    result = subprocess.run(
        ['plugparams', '--plugin', plugin_path, '--json'],
        capture_output=True,
        text=True
    )
    return json.loads(result.stdout)

def set_parameter(plugin_path, param_name, value):
    """Set a plugin parameter."""
    subprocess.run([
        'plugparams',
        '--plugin', plugin_path,
        '--set', f'{param_name}={value}'
    ])

# Usage
params = get_plugin_parameters('plugin.vst3')
for p in params['parameters']:
    if p['type'] == 'Boolean':
        print(f"{p['name']}: {p['currentValue']}")

set_parameter('plugin.vst3', 'Gain', 0.75)
```

### Shell Script Example

```bash
#!/bin/bash
# Sweep through all discrete parameter values

PLUGIN="$1"
PARAM_INDEX="$2"

# Get parameter info
INFO=$(plugparams --plugin "$PLUGIN" --json | \
       jq ".parameters[$PARAM_INDEX]")

TYPE=$(echo "$INFO" | jq -r '.type')
STEPS=$(echo "$INFO" | jq -r '.numSteps')

if [ "$TYPE" != "Discrete" ]; then
  echo "Parameter is not discrete"
  exit 1
fi

# Test each value
for i in $(seq 0 $((STEPS-1))); do
  VALUE=$(echo "scale=6; $i / ($STEPS - 1)" | bc)
  echo "Testing value $VALUE"
  
  plugparams --plugin "$PLUGIN" --set "$PARAM_INDEX=$VALUE"
  plugperf --plugin "$PLUGIN" \
    --buffers 1024 \
    --iterations 100 \
    --out "results_step_${i}.csv"
done
```

## Limitations

- **VST3 Only** - Currently supports VST3 plugins only
- **Normalized Values** - All parameter values are normalized 0.0-1.0
- **No Preset Loading** - Cannot load .vstpreset files (planned feature)
- **No State Persistence** - Parameter changes are not saved to plugin

## See Also

- [README.md](../README.md) - Main PlugPerf documentation
- [BATCH_TESTING.md](BATCH_TESTING.md) - Batch testing guide
- [Plugin Parameter API](../src/plugin_params.hpp) - C++ implementation
