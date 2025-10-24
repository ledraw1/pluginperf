#!/usr/bin/env python3
"""
Visualization tool for plugperf benchmark results.
Generates PNG charts showing performance metrics across buffer sizes.
"""

import csv
import sys
import argparse
from pathlib import Path

try:
    import matplotlib
    matplotlib.use('Agg')  # Non-interactive backend
    import matplotlib.pyplot as plt
    import numpy as np
except ImportError:
    print("ERROR: matplotlib is required for visualization")
    print("Install with: pip3 install matplotlib")
    sys.exit(1)


def load_results(csv_path):
    """Load benchmark results from CSV file."""
    data = {
        'buffer_sizes': [],
        'mean_us': [],
        'median_us': [],
        'p95_us': [],
        'min_us': [],
        'max_us': [],
        'std_dev_us': [],
        'cv_pct': [],
        'rt_cpu_pct': [],
        'dsp_load_pct': [],
        'plugin_name': None,
    }
    
    with open(csv_path, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            if data['plugin_name'] is None:
                data['plugin_name'] = row['plugin_name']
            
            data['buffer_sizes'].append(int(float(row['block_size'])))
            data['mean_us'].append(float(row['mean_us']))
            data['median_us'].append(float(row['median_us']))
            data['p95_us'].append(float(row['p95_us']))
            data['min_us'].append(float(row['min_us']))
            data['max_us'].append(float(row['max_us']))
            data['std_dev_us'].append(float(row['std_dev_us']))
            data['cv_pct'].append(float(row['cv_pct']))
            data['rt_cpu_pct'].append(float(row['approx_rt_cpu_pct']))
            data['dsp_load_pct'].append(float(row['dsp_load_pct']))
    
    return data


def create_visualization(data, output_path):
    """Create comprehensive visualization with multiple subplots."""
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle(f'Performance Analysis: {data["plugin_name"]}', fontsize=16, fontweight='bold')
    
    buffer_sizes = data['buffer_sizes']
    
    # Plot 1: Processing Time (with error bars)
    ax1 = axes[0, 0]
    mean_ms = [x / 1000 for x in data['mean_us']]
    std_ms = [x / 1000 for x in data['std_dev_us']]
    
    ax1.errorbar(buffer_sizes, mean_ms, yerr=std_ms, marker='o', capsize=5, 
                 linewidth=2, markersize=8, label='Mean ± StdDev')
    ax1.set_xlabel('Buffer Size (samples)', fontsize=11)
    ax1.set_ylabel('Processing Time (ms)', fontsize=11)
    ax1.set_title('Processing Time vs Buffer Size', fontsize=12, fontweight='bold')
    ax1.grid(True, alpha=0.3)
    ax1.set_xscale('log', base=2)
    ax1.legend()
    
    # Plot 2: CPU Load
    ax2 = axes[0, 1]
    ax2.plot(buffer_sizes, data['rt_cpu_pct'], marker='s', linewidth=2, 
             markersize=8, label='RT CPU %', color='#2ca02c')
    ax2.axhline(y=100, color='r', linestyle='--', linewidth=1, alpha=0.7, label='100% (RT limit)')
    ax2.set_xlabel('Buffer Size (samples)', fontsize=11)
    ax2.set_ylabel('CPU Load (%)', fontsize=11)
    ax2.set_title('Real-Time CPU Load', fontsize=12, fontweight='bold')
    ax2.grid(True, alpha=0.3)
    ax2.set_xscale('log', base=2)
    ax2.legend()
    
    # Plot 3: Measurement Stability (CV)
    ax3 = axes[1, 0]
    colors = ['green' if cv < 10 else 'orange' if cv < 20 else 'red' 
              for cv in data['cv_pct']]
    ax3.bar(range(len(buffer_sizes)), data['cv_pct'], color=colors, alpha=0.7)
    ax3.set_xlabel('Buffer Size (samples)', fontsize=11)
    ax3.set_ylabel('Coefficient of Variation (%)', fontsize=11)
    ax3.set_title('Measurement Stability (lower is better)', fontsize=12, fontweight='bold')
    ax3.set_xticks(range(len(buffer_sizes)))
    ax3.set_xticklabels(buffer_sizes, rotation=45)
    ax3.axhline(y=10, color='orange', linestyle='--', linewidth=1, alpha=0.5, label='Good (<10%)')
    ax3.axhline(y=20, color='red', linestyle='--', linewidth=1, alpha=0.5, label='Fair (<20%)')
    ax3.grid(True, alpha=0.3, axis='y')
    ax3.legend()
    
    # Plot 4: Percentile Distribution
    ax4 = axes[1, 1]
    x_pos = np.arange(len(buffer_sizes))
    width = 0.25
    
    min_ms = [x / 1000 for x in data['min_us']]
    median_ms = [x / 1000 for x in data['median_us']]
    p95_ms = [x / 1000 for x in data['p95_us']]
    
    ax4.bar(x_pos - width, min_ms, width, label='Min', alpha=0.8)
    ax4.bar(x_pos, median_ms, width, label='Median', alpha=0.8)
    ax4.bar(x_pos + width, p95_ms, width, label='P95', alpha=0.8)
    
    ax4.set_xlabel('Buffer Size (samples)', fontsize=11)
    ax4.set_ylabel('Processing Time (ms)', fontsize=11)
    ax4.set_title('Percentile Distribution', fontsize=12, fontweight='bold')
    ax4.set_xticks(x_pos)
    ax4.set_xticklabels(buffer_sizes, rotation=45)
    ax4.legend()
    ax4.grid(True, alpha=0.3, axis='y')
    
    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    print(f"✓ Visualization saved to: {output_path}")


def main():
    parser = argparse.ArgumentParser(
        description='Generate performance visualization from plugperf CSV results'
    )
    parser.add_argument('csv_file', help='Path to CSV results file')
    parser.add_argument('-o', '--output', help='Output PNG file path (default: results.png)',
                       default='results.png')
    
    args = parser.parse_args()
    
    if not Path(args.csv_file).exists():
        print(f"ERROR: File not found: {args.csv_file}")
        sys.exit(1)
    
    print(f"Loading results from: {args.csv_file}")
    data = load_results(args.csv_file)
    
    print(f"Generating visualization for: {data['plugin_name']}")
    print(f"  Buffer sizes: {data['buffer_sizes']}")
    
    create_visualization(data, args.output)


if __name__ == '__main__':
    main()
