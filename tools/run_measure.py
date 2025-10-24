#!/usr/bin/env python3
"""
run_measure.py — convenience wrapper for the plugperf CLI

Examples:
  python tools/run_measure.py \
    --plugin "/Library/Audio/Plug-Ins/VST3/YourPlugin.vst3" \
    --sr 48000 --channels 2 \
    --buffers 32,64,128,256,512,1024,2048,4096,8192,16384 \
    --warmup 40 --iterations 400 \
    --out perf.csv
"""

import argparse
import os
import sys
import subprocess
from pathlib import Path

def default_bin() -> Path:
    if os.name == "nt":
        # Typical MSVC multi-config layout
        p = Path("build/Release/plugperf.exe")
        if not p.exists():
            p = Path("build/plugperf.exe")
    else:
        p = Path("build/plugperf")
    return p

def parse_args():
    ap = argparse.ArgumentParser(description="Run plugperf and print a quick summary table")
    ap.add_argument("--plugin", required=True, help="Path to .vst3 bundle (mac/win/linux)")
    ap.add_argument("--sr", type=float, default=48000.0, help="Sample rate (default 48000)")
    ap.add_argument("--channels", type=int, default=2, help="Channel count (default 2)")
    ap.add_argument("--buffers", default="32,64,128,256,512,1024,2048,4096,8192,16384",
                    help="Comma-separated block sizes (default 32..16384)")
    ap.add_argument("--warmup", type=int, default=40, help="Warmup iterations per block (default 40)")
    ap.add_argument("--iterations", type=int, default=400, help="Timed iterations per block (default 400)")
    ap.add_argument("--out", default=None, help="CSV output path (default stdout)")
    ap.add_argument("--bin", default=str(default_bin()), help="Path to plugperf binary (default ./build/plugperf)")
    return ap.parse_args()

def check_prereqs(args) -> None:
    bin_path = Path(args.bin)
    if not bin_path.exists():
        sys.stderr.write(f"[error] plugperf binary not found at: {bin_path}\n"
                         "        Build it first: cmake -S . -B build && cmake --build build\n")
        sys.exit(2)

    plugin_path = Path(args.plugin)
    if not plugin_path.exists():
        sys.stderr.write(f"[error] Plugin path does not exist: {plugin_path}\n")
        sys.exit(3)

    # Quick sanity: on macOS .vst3 is a bundle dir
    if sys.platform == "darwin" and plugin_path.suffix.lower() != ".vst3":
        sys.stderr.write("[warn] Plugin path on macOS typically ends with .vst3\n")

def run_plugperf(args) -> Path:
    bin_path = Path(args.bin)
    out_path = Path(args.out) if args.out else None

    cmd = [
        str(bin_path),
        "--plugin", args.plugin,
        "--sr", str(args.sr),
        "--channels", str(args.channels),
        "--buffers", args.buffers,
        "--warmup", str(args.warmup),
        "--iterations", str(args.iterations),
    ]
    if out_path is not None:
        cmd += ["--out", str(out_path)]

    print("[info] Running:", " ".join(cmd))
    try:
        if out_path is None:
            # Capture stdout since CSV is output to stdout
            result = subprocess.run(cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            csv_text = result.stdout
            return csv_text
        else:
            subprocess.run(cmd, check=True)
            return out_path
    except subprocess.CalledProcessError as e:
        sys.stderr.write(f"[error] plugperf failed with exit code {e.returncode}\n")
        if e.stderr:
            sys.stderr.write(e.stderr)
        sys.exit(e.returncode or 1)

def print_table_from_text(csv_text: str) -> None:
    try:
        import pandas as pd  # type: ignore
        from io import StringIO
        df = pd.read_csv(StringIO(csv_text))
        cols = ["block_size", "median_us", "p95_us", "approx_rt_cpu_pct"]
        missing = [c for c in cols if c not in df.columns]
        if missing:
            sys.stderr.write(f"[warn] CSV missing expected columns: {missing}\n")
            print(df.head())
            return
        print("\nQuick summary (lower is better):")
        print(df[cols].to_string(index=False))
    except Exception:
        import csv
        from io import StringIO
        print("\nQuick summary (fallback CSV parser):")
        reader = csv.DictReader(StringIO(csv_text))
        rows = list(reader)
        if not rows:
            print("(no rows)")
            return
        cols = ["block_size", "median_us", "p95_us", "approx_rt_cpu_pct"]
        widths = {c: max(len(c), max((len(r.get(c, "")) for r in rows), default=0)) for c in cols}
        header = " ".join(c.ljust(widths[c]) for c in cols)
        print(header)
        print("-" * len(header))
        for r in rows:
            print(" ".join((r.get(c, "").ljust(widths[c]) for c in cols)))

def print_table(csv_path: Path) -> None:
    try:
        import pandas as pd  # type: ignore
        df = pd.read_csv(csv_path)
        cols = ["block_size", "median_us", "p95_us", "approx_rt_cpu_pct"]
        missing = [c for c in cols if c not in df.columns]
        if missing:
            sys.stderr.write(f"[warn] CSV missing expected columns: {missing}\n")
            print(df.head())
            return
        print("\nQuick summary (lower is better):")
        print(df[cols].to_string(index=False))
    except Exception:
        import csv
        print("\nQuick summary (fallback CSV parser):")
        with open(csv_path, newline="") as f:
            reader = csv.DictReader(f)
            rows = list(reader)
        if not rows:
            print("(no rows)")
            return
        cols = ["block_size", "median_us", "p95_us", "approx_rt_cpu_pct"]
        widths = {c: max(len(c), max((len(r.get(c, "")) for r in rows), default=0)) for c in cols}
        header = " ".join(c.ljust(widths[c]) for c in cols)
        print(header)
        print("-" * len(header))
        for r in rows:
            print(" ".join((r.get(c, "").ljust(widths[c]) for c in cols)))

def main():
    args = parse_args()
    check_prereqs(args)
    result = run_plugperf(args)
    if isinstance(result, Path):
        if result.exists():
            print_table(result)
        else:
            sys.stderr.write(f"[error] Expected CSV output file not found: {result}\n")
            sys.exit(4)
    else:
        # CSV text from stdout
        print_table_from_text(result)

if __name__ == "__main__":
    main()
