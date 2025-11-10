#!/usr/bin/env python3
"""
Preset Validation Script for StoryBored Plugins

Validates that all presets can be loaded and applied correctly.
Uses plugparams instead of plugperf for faster validation without benchmarking.

Usage:
    python3 validate_presets.py --plugin /path/to/Plugin.vst3 \\
                                 --preset-dir /path/to/presets \\
                                 --output validation_report.txt
"""

import argparse
import subprocess
import json
from pathlib import Path
from typing import List, Dict, Tuple
import sys
import re


def find_presets(preset_dir: Path, pattern: str = "*.json") -> List[Path]:
    """Find all preset files in directory."""
    presets = sorted(preset_dir.glob(pattern))
    # Filter out preset_index.json or other non-preset files
    presets = [p for p in presets if p.name != "preset_index.json"]
    return presets


def validate_preset(plugparams_path: Path, 
                   plugin_path: Path,
                   preset_path: Path) -> Tuple[bool, int, int, str]:
    """
    Validate that a preset can be loaded and applied.
    
    Returns:
        (success, applied_count, total_count, error_message)
    """
    cmd = [
        str(plugparams_path),
        "--plugin", str(plugin_path),
        "--preset-json", str(preset_path)
    ]
    
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=30  # 30 second timeout per preset
        )
        
        # Parse output for "Applied X parameters"
        output = result.stdout + result.stderr
        
        # Look for success message
        match = re.search(r'Applied (\d+) parameters', output)
        if match:
            applied = int(match.group(1))
            
            # Look for total parameters in preset
            total_match = re.search(r'Total parameters in preset: (\d+)', output)
            total = int(total_match.group(1)) if total_match else applied
            
            return (True, applied, total, "")
        else:
            # Check for error messages
            if "ERROR" in output or "Failed" in output:
                error_lines = [line for line in output.split('\n') if 'ERROR' in line or 'Failed' in line]
                error_msg = "; ".join(error_lines[:3])  # First 3 error lines
                return (False, 0, 0, error_msg)
            else:
                return (False, 0, 0, "No output from plugparams")
            
    except subprocess.TimeoutExpired:
        return (False, 0, 0, "Timeout (>30s)")
    except Exception as e:
        return (False, 0, 0, str(e))


def main():
    parser = argparse.ArgumentParser(
        description="Validate StoryBored presets can be loaded",
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    
    parser.add_argument("--plugin", required=True, help="Path to VST3 plugin")
    parser.add_argument("--preset-dir", required=True, help="Directory containing preset JSON files")
    parser.add_argument("--output", help="Output report file (default: stdout)")
    parser.add_argument("--plugparams", default="./build/plugparams", help="Path to plugparams binary")
    parser.add_argument("--pattern", default="*.json", help="Preset file pattern (default: *.json)")
    parser.add_argument("--max-presets", type=int, help="Maximum number of presets to test")
    parser.add_argument("--verbose", action="store_true", help="Show detailed output")
    
    args = parser.parse_args()
    
    # Validate paths
    plugin_path = Path(args.plugin).expanduser()
    if not plugin_path.exists():
        print(f"Error: Plugin not found: {plugin_path}")
        return 1
    
    preset_dir = Path(args.preset_dir)
    if not preset_dir.exists():
        print(f"Error: Preset directory not found: {preset_dir}")
        return 1
    
    plugparams_path = Path(args.plugparams)
    if not plugparams_path.exists():
        print(f"Error: plugparams binary not found: {plugparams_path}")
        print(f"       Build it first with: cmake --build build")
        return 1
    
    # Find presets
    presets = find_presets(preset_dir, args.pattern)
    if not presets:
        print(f"Error: No presets found in {preset_dir} matching {args.pattern}")
        return 1
    
    if args.max_presets:
        presets = presets[:args.max_presets]
    
    # Open output file if specified
    output_file = open(args.output, 'w') if args.output else sys.stdout
    
    try:
        output_file.write("=" * 80 + "\n")
        output_file.write("StoryBored Preset Validation Report\n")
        output_file.write("=" * 80 + "\n")
        output_file.write(f"Plugin:  {plugin_path.name}\n")
        output_file.write(f"Presets: {len(presets)} found in {preset_dir}\n")
        output_file.write("=" * 80 + "\n\n")
        
        # Track results
        total = len(presets)
        success_count = 0
        failed_presets = []
        param_stats = []
        
        # Process each preset
        for idx, preset_path in enumerate(presets, 1):
            if args.verbose or not args.output:
                print(f"[{idx}/{total}] Validating: {preset_path.name}", end="", flush=True)
            
            success, applied, total_params, error = validate_preset(
                plugparams_path, plugin_path, preset_path
            )
            
            if success:
                success_count += 1
                param_stats.append((applied, total_params))
                if args.verbose or not args.output:
                    print(f" ✅ ({applied}/{total_params} parameters)")
                output_file.write(f"✅ {preset_path.name}: {applied}/{total_params} parameters\n")
            else:
                failed_presets.append((preset_path.name, error))
                if args.verbose or not args.output:
                    print(f" ✗ {error}")
                output_file.write(f"✗ {preset_path.name}: {error}\n")
        
        # Write summary
        success_rate = (success_count / total * 100) if total > 0 else 0.0
        
        output_file.write("\n" + "=" * 80 + "\n")
        output_file.write("SUMMARY\n")
        output_file.write("=" * 80 + "\n")
        output_file.write(f"Total Presets:    {total}\n")
        output_file.write(f"Successful:       {success_count}\n")
        output_file.write(f"Failed:           {len(failed_presets)}\n")
        output_file.write(f"Success Rate:     {success_rate:.1f}%\n")
        
        if param_stats:
            avg_applied = sum(p[0] for p in param_stats) / len(param_stats)
            avg_total = sum(p[1] for p in param_stats) / len(param_stats)
            output_file.write(f"\nAverage Parameters Applied: {avg_applied:.1f}/{avg_total:.1f}\n")
        
        if failed_presets:
            output_file.write("\nFailed Presets:\n")
            for name, error in failed_presets:
                output_file.write(f"  - {name}: {error}\n")
        
        output_file.write("=" * 80 + "\n")
        
        # Print summary to console if writing to file
        if args.output:
            print(f"\n✅ Validation complete: {success_count}/{total} presets ({success_rate:.1f}%)")
            print(f"   Report written to: {args.output}")
        
        return 0 if len(failed_presets) == 0 else 1
        
    finally:
        if args.output:
            output_file.close()


if __name__ == "__main__":
    sys.exit(main())
