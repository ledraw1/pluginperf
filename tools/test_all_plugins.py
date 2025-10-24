#!/usr/bin/env python3
"""
test_all_plugins.py — Batch test all VST3 plugins in system folder

Scans the system VST3 plugins folder and runs performance tests on each plugin.
Results are saved to individual CSV files and a summary report is generated.

Usage:
  python tools/test_all_plugins.py [options]

Options:
  --vst3-dir PATH          Path to VST3 plugins folder (default: /Library/Audio/Plug-Ins/VST3)
  --output-dir PATH        Output directory for results (default: ./plugin_results)
  --buffers SIZES          Comma-separated buffer sizes (default: 64,256,1024,4096)
  --iterations N           Number of iterations per buffer (default: 200)
  --warmup N               Number of warmup iterations (default: 40)
  --sr RATE                Sample rate (default: 48000)
  --channels N             Channel count (default: 2)
  --bits DEPTH             Bit depth: 32f or 64f (default: 32f)
  --skip-errors            Continue testing even if a plugin fails
  --parallel N             Number of plugins to test in parallel (default: 1)
"""

import argparse
import os
import sys
import subprocess
import csv
from pathlib import Path
from datetime import datetime
import json

def find_plugperf_binary():
    """Find the plugperf binary."""
    script_dir = Path(__file__).parent
    candidates = [
        script_dir.parent / "build" / "plugperf",
        script_dir.parent / "build" / "Release" / "plugperf",
        Path("./build/plugperf"),
        Path("./plugperf"),
    ]
    
    for candidate in candidates:
        if candidate.exists():
            return candidate.resolve()
    
    return None

def find_vst3_plugins(vst3_dir):
    """Find all VST3 plugins in the given directory."""
    vst3_path = Path(vst3_dir)
    if not vst3_path.exists():
        print(f"ERROR: VST3 directory not found: {vst3_dir}")
        return []
    
    plugins = []
    for item in vst3_path.iterdir():
        if item.suffix.lower() == '.vst3':
            plugins.append(item)
    
    return sorted(plugins)

def test_plugin(plugperf_bin, plugin_path, output_file, args):
    """Test a single plugin and return success status."""
    cmd = [
        str(plugperf_bin),
        "--plugin", str(plugin_path),
        "--sr", str(args.sr),
        "--channels", str(args.channels),
        "--bits", args.bits,
        "--buffers", args.buffers,
        "--warmup", str(args.warmup),
        "--iterations", str(args.iterations),
        "--out", str(output_file)
    ]
    
    print(f"  Testing: {plugin_path.name}")
    print(f"  Command: {' '.join(cmd)}")
    
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=300  # 5 minute timeout per plugin
        )
        
        if result.returncode == 0:
            print(f"  ✓ Success")
            return True, None
        else:
            error_msg = result.stderr if result.stderr else "Unknown error"
            print(f"  ✗ Failed: {error_msg[:200]}")
            return False, error_msg
            
    except subprocess.TimeoutExpired:
        print(f"  ✗ Timeout (>5 minutes)")
        return False, "Timeout"
    except Exception as e:
        print(f"  ✗ Exception: {e}")
        return False, str(e)

def load_results(csv_file):
    """Load results from a CSV file."""
    if not csv_file.exists():
        return None
    
    try:
        with open(csv_file, 'r') as f:
            reader = csv.DictReader(f)
            rows = list(reader)
            if not rows:
                return None
            
            # Calculate summary statistics
            mean_dsp_load = sum(float(r['dsp_load_pct']) for r in rows) / len(rows)
            mean_cv = sum(float(r['cv_pct']) for r in rows) / len(rows)
            
            return {
                'plugin_name': rows[0]['plugin_name'],
                'format': rows[0]['format'],
                'channels': rows[0]['channels'],
                'bit_depth': rows[0]['bit_depth'],
                'buffer_count': len(rows),
                'mean_dsp_load': mean_dsp_load,
                'mean_cv': mean_cv,
                'rows': rows
            }
    except Exception as e:
        print(f"  Warning: Could not parse results: {e}")
        return None

def generate_summary_report(output_dir, results):
    """Generate a summary report of all tests."""
    summary_file = output_dir / "summary.txt"
    summary_json = output_dir / "summary.json"
    
    # Text summary
    with open(summary_file, 'w') as f:
        f.write("=" * 90 + "\n")
        f.write("PLUGPERF BATCH TEST SUMMARY\n")
        f.write("=" * 90 + "\n")
        f.write(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        f.write(f"Total plugins tested: {len(results['tested'])}\n")
        f.write(f"Successful: {len(results['successful'])}\n")
        f.write(f"Failed: {len(results['failed'])}\n")
        f.write("\n")
        
        if results['successful']:
            f.write("SUCCESSFUL TESTS\n")
            f.write("-" * 90 + "\n")
            f.write(f"{'Plugin':<50} {'Avg DSP%':<12} {'Avg CV%':<12} {'Buffers':<10}\n")
            f.write("-" * 90 + "\n")
            
            for plugin_name, data in sorted(results['successful'].items()):
                f.write(f"{plugin_name:<50} {data['mean_dsp_load']:<12.2f} "
                       f"{data['mean_cv']:<12.2f} {data['buffer_count']:<10}\n")
            f.write("\n")
        
        if results['failed']:
            f.write("FAILED TESTS\n")
            f.write("-" * 90 + "\n")
            for plugin_name, error in sorted(results['failed'].items()):
                f.write(f"{plugin_name}: {error}\n")
            f.write("\n")
    
    # JSON summary
    with open(summary_json, 'w') as f:
        json.dump(results, f, indent=2)
    
    print(f"\n✓ Summary report saved to: {summary_file}")
    print(f"✓ JSON summary saved to: {summary_json}")

def main():
    parser = argparse.ArgumentParser(description="Batch test all VST3 plugins")
    parser.add_argument("--vst3-dir", default="/Library/Audio/Plug-Ins/VST3",
                       help="Path to VST3 plugins folder")
    parser.add_argument("--output-dir", default="./plugin_results",
                       help="Output directory for results")
    parser.add_argument("--buffers", default="64,256,1024,4096",
                       help="Comma-separated buffer sizes")
    parser.add_argument("--iterations", type=int, default=200,
                       help="Number of iterations per buffer")
    parser.add_argument("--warmup", type=int, default=40,
                       help="Number of warmup iterations")
    parser.add_argument("--sr", type=int, default=48000,
                       help="Sample rate")
    parser.add_argument("--channels", type=int, default=2,
                       help="Channel count")
    parser.add_argument("--bits", default="32f",
                       help="Bit depth (32f or 64f)")
    parser.add_argument("--skip-errors", action="store_true",
                       help="Continue testing even if a plugin fails")
    
    args = parser.parse_args()
    
    # Find plugperf binary
    plugperf_bin = find_plugperf_binary()
    if not plugperf_bin:
        print("ERROR: Could not find plugperf binary")
        print("Please build it first: cmake -B build && cmake --build build")
        return 1
    
    print(f"Using plugperf binary: {plugperf_bin}")
    
    # Find plugins
    plugins = find_vst3_plugins(args.vst3_dir)
    if not plugins:
        print(f"ERROR: No VST3 plugins found in {args.vst3_dir}")
        return 1
    
    print(f"\nFound {len(plugins)} VST3 plugins in {args.vst3_dir}")
    
    # Create output directory
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    print(f"Results will be saved to: {output_dir}")
    
    # Test each plugin
    results = {
        'tested': [],
        'successful': {},
        'failed': {},
        'timestamp': datetime.now().isoformat(),
        'config': {
            'vst3_dir': args.vst3_dir,
            'buffers': args.buffers,
            'iterations': args.iterations,
            'warmup': args.warmup,
            'sample_rate': args.sr,
            'channels': args.channels,
            'bit_depth': args.bits
        }
    }
    
    print(f"\n{'=' * 90}")
    print("STARTING BATCH TEST")
    print(f"{'=' * 90}\n")
    
    for i, plugin in enumerate(plugins, 1):
        print(f"[{i}/{len(plugins)}] {plugin.name}")
        
        # Create safe filename
        safe_name = "".join(c if c.isalnum() or c in ('-', '_') else '_' 
                           for c in plugin.stem)
        output_file = output_dir / f"{safe_name}.csv"
        
        results['tested'].append(plugin.name)
        
        # Test the plugin
        success, error = test_plugin(plugperf_bin, plugin, output_file, args)
        
        if success:
            # Load and parse results
            data = load_results(output_file)
            if data:
                results['successful'][plugin.name] = data
            else:
                results['failed'][plugin.name] = "Could not parse results"
        else:
            results['failed'][plugin.name] = error
            if not args.skip_errors:
                print(f"\nStopping due to error. Use --skip-errors to continue.")
                break
        
        print()
    
    # Generate summary
    print(f"{'=' * 90}")
    print("BATCH TEST COMPLETE")
    print(f"{'=' * 90}\n")
    print(f"Total tested: {len(results['tested'])}")
    print(f"Successful: {len(results['successful'])}")
    print(f"Failed: {len(results['failed'])}")
    
    generate_summary_report(output_dir, results)
    
    return 0 if not results['failed'] else 1

if __name__ == "__main__":
    sys.exit(main())
