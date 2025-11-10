#!/usr/bin/env python3
"""
Batch Preset CPU Profiling Script for StoryBored Plugins

Sweeps through a folder of JSON presets and runs CPU benchmarks on each one.
Similar to test_all_plugins.py but for preset-based profiling.

Usage:
    python3 test_preset_folder.py --plugin /path/to/Plugin.vst3 \\
                                   --preset-dir /path/to/presets \\
                                   --output-dir ./results \\
                                   --buffers 64,256,1024,4096 \\
                                   --iterations 200
"""

import argparse
import subprocess
import json
import csv
from pathlib import Path
from typing import List, Dict, Optional
import sys
import time


def find_presets(preset_dir: Path, pattern: str = "*.json") -> List[Path]:
    """Find all preset files in directory."""
    presets = sorted(preset_dir.glob(pattern))
    # Filter out preset_index.json or other non-preset files
    presets = [p for p in presets if p.name != "preset_index.json"]
    return presets


def load_preset_metadata(preset_path: Path) -> Dict:
    """Load preset metadata from JSON file."""
    try:
        with open(preset_path, 'r') as f:
            data = json.load(f)
            if "preset" in data:
                return data["preset"].get("metadata", {})
            return data.get("metadata", {})
    except Exception as e:
        print(f"Warning: Could not load metadata from {preset_path.name}: {e}")
        return {}


def run_benchmark(plugperf_path: Path, 
                  plugin_path: Path,
                  preset_path: Path,
                  output_csv: Path,
                  buffers: List[int],
                  sample_rate: int = 48000,
                  channels: int = 2,
                  warmup: int = 40,
                  iterations: int = 400,
                  bit_depth: str = "32f") -> bool:
    """
    Run plugperf benchmark with a specific preset.
    
    Returns:
        True if benchmark succeeded, False otherwise
    """
    cmd = [
        str(plugperf_path),
        "--plugin", str(plugin_path),
        "--preset-json", str(preset_path),
        "--sr", str(sample_rate),
        "--channels", str(channels),
        "--bits", bit_depth,
        "--buffers", ",".join(map(str, buffers)),
        "--warmup", str(warmup),
        "--iterations", str(iterations),
        "--out", str(output_csv)
    ]
    
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=300  # 5 minute timeout per preset
        )
        
        if result.returncode != 0:
            print(f"  ✗ Benchmark failed: {result.stderr[:200]}")
            return False
        
        # Check if preset was loaded
        if "Loaded preset:" in result.stderr:
            return True
        else:
            print(f"  ⚠️  Preset may not have loaded correctly")
            return True  # Still count as success if benchmark ran
            
    except subprocess.TimeoutExpired:
        print(f"  ✗ Benchmark timed out (>5 minutes)")
        return False
    except Exception as e:
        print(f"  ✗ Error running benchmark: {e}")
        return False


def merge_csv_results(output_files: List[Path], merged_output: Path):
    """Merge multiple CSV files into one, adding preset name column."""
    all_rows = []
    header = None
    
    for csv_file in output_files:
        if not csv_file.exists():
            continue
            
        with open(csv_file, 'r') as f:
            reader = csv.reader(f)
            rows = list(reader)
            
            if not rows:
                continue
            
            if header is None:
                # First file - save header and add preset_name column
                header = rows[0] + ["preset_name", "preset_category"]
                all_rows.append(header)
            
            # Add data rows with preset name
            preset_name = csv_file.stem.replace("_benchmark", "")
            for row in rows[1:]:  # Skip header
                all_rows.append(row + [preset_name, ""])
    
    # Write merged CSV
    with open(merged_output, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerows(all_rows)
    
    print(f"\n✅ Merged results written to: {merged_output}")
    print(f"   Total benchmark runs: {len(all_rows) - 1}")


def generate_summary_report(output_dir: Path, results: Dict):
    """Generate a summary report of all benchmarks."""
    summary_file = output_dir / "benchmark_summary.txt"
    
    with open(summary_file, 'w') as f:
        f.write("=" * 80 + "\n")
        f.write("StoryBored Preset CPU Profiling Summary\n")
        f.write("=" * 80 + "\n\n")
        
        f.write(f"Total Presets Tested: {results['total']}\n")
        f.write(f"Successful: {results['success']}\n")
        f.write(f"Failed: {results['failed']}\n")
        f.write(f"Success Rate: {results['success_rate']:.1f}%\n\n")
        
        if results['failed_presets']:
            f.write("Failed Presets:\n")
            for preset in results['failed_presets']:
                f.write(f"  - {preset}\n")
        
        f.write("\n" + "=" * 80 + "\n")
    
    print(f"✅ Summary report written to: {summary_file}")


def main():
    parser = argparse.ArgumentParser(
        description="Batch CPU profiling for StoryBored presets",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Test all presets with default settings
  python3 test_preset_folder.py --plugin EKKOSPACE.vst3 \\
                                 --preset-dir ./Presets \\
                                 --output-dir ./results

  # Custom buffer sizes and iterations
  python3 test_preset_folder.py --plugin EKKOSPACE.vst3 \\
                                 --preset-dir ./Presets \\
                                 --output-dir ./results \\
                                 --buffers 64,256,1024,4096 \\
                                 --iterations 200 \\
                                 --warmup 40

  # Test only specific presets
  python3 test_preset_folder.py --plugin EKKOSPACE.vst3 \\
                                 --preset-dir ./Presets \\
                                 --output-dir ./results \\
                                 --pattern "*Flanger*.json"
        """
    )
    
    parser.add_argument("--plugin", required=True, help="Path to VST3 plugin")
    parser.add_argument("--preset-dir", required=True, help="Directory containing preset JSON files")
    parser.add_argument("--output-dir", required=True, help="Output directory for results")
    parser.add_argument("--plugperf", default="./build/plugperf", help="Path to plugperf binary")
    parser.add_argument("--buffers", default="64,256,512,1024,2048,4096", help="Comma-separated buffer sizes")
    parser.add_argument("--sr", type=int, default=48000, help="Sample rate (default: 48000)")
    parser.add_argument("--channels", type=int, default=2, help="Channel count (default: 2)")
    parser.add_argument("--bits", default="32f", choices=["32f", "64f"], help="Bit depth (default: 32f)")
    parser.add_argument("--warmup", type=int, default=40, help="Warmup iterations (default: 40)")
    parser.add_argument("--iterations", type=int, default=400, help="Timed iterations (default: 400)")
    parser.add_argument("--pattern", default="*.json", help="Preset file pattern (default: *.json)")
    parser.add_argument("--skip-errors", action="store_true", help="Continue on errors")
    parser.add_argument("--max-presets", type=int, help="Maximum number of presets to test")
    
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
    
    plugperf_path = Path(args.plugperf)
    if not plugperf_path.exists():
        print(f"Error: plugperf binary not found: {plugperf_path}")
        print(f"       Build it first with: cmake --build build")
        return 1
    
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Parse buffer sizes
    buffers = [int(b) for b in args.buffers.split(",")]
    
    # Find presets
    presets = find_presets(preset_dir, args.pattern)
    if not presets:
        print(f"Error: No presets found in {preset_dir} matching {args.pattern}")
        return 1
    
    if args.max_presets:
        presets = presets[:args.max_presets]
    
    print("=" * 80)
    print("StoryBored Preset CPU Profiling")
    print("=" * 80)
    print(f"Plugin:       {plugin_path.name}")
    print(f"Presets:      {len(presets)} found in {preset_dir}")
    print(f"Buffers:      {buffers}")
    print(f"Sample Rate:  {args.sr} Hz")
    print(f"Channels:     {args.channels}")
    print(f"Bit Depth:    {args.bits}")
    print(f"Iterations:   {args.iterations} (warmup: {args.warmup})")
    print(f"Output:       {output_dir}")
    print("=" * 80)
    print()
    
    # Track results
    results = {
        'total': len(presets),
        'success': 0,
        'failed': 0,
        'failed_presets': [],
        'success_rate': 0.0
    }
    
    output_files = []
    start_time = time.time()
    
    # Process each preset
    for idx, preset_path in enumerate(presets, 1):
        print(f"[{idx}/{len(presets)}] Testing: {preset_path.name}")
        
        # Load metadata for context
        metadata = load_preset_metadata(preset_path)
        if metadata:
            print(f"  Name: {metadata.get('name', 'Unknown')}")
            print(f"  Category: {metadata.get('category', 'Unknown')}")
        
        # Create output CSV for this preset
        output_csv = output_dir / f"{preset_path.stem}_benchmark.csv"
        output_files.append(output_csv)
        
        # Run benchmark
        success = run_benchmark(
            plugperf_path=plugperf_path,
            plugin_path=plugin_path,
            preset_path=preset_path,
            output_csv=output_csv,
            buffers=buffers,
            sample_rate=args.sr,
            channels=args.channels,
            warmup=args.warmup,
            iterations=args.iterations,
            bit_depth=args.bits
        )
        
        if success:
            results['success'] += 1
            print(f"  ✅ Benchmark complete")
        else:
            results['failed'] += 1
            results['failed_presets'].append(preset_path.name)
            if not args.skip_errors:
                print(f"\nError: Benchmark failed for {preset_path.name}")
                print(f"Use --skip-errors to continue on failures")
                return 1
        
        print()
    
    # Calculate success rate
    results['success_rate'] = (results['success'] / results['total'] * 100) if results['total'] > 0 else 0.0
    
    # Merge all CSV results
    merged_csv = output_dir / "all_presets_benchmark.csv"
    merge_csv_results(output_files, merged_csv)
    
    # Generate summary report
    generate_summary_report(output_dir, results)
    
    # Print final summary
    elapsed = time.time() - start_time
    print("\n" + "=" * 80)
    print("BENCHMARK COMPLETE")
    print("=" * 80)
    print(f"Total Time: {elapsed:.1f}s ({elapsed/60:.1f} minutes)")
    print(f"Success Rate: {results['success']}/{results['total']} ({results['success_rate']:.1f}%)")
    print(f"Results: {merged_csv}")
    print("=" * 80)
    
    return 0 if results['failed'] == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
