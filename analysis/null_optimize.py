#!/usr/bin/env python3
"""Actively search knob settings (drive/tone) around a capture's nominal to find the DEEPEST
null — i.e. how close the plugin can get to a real capture if we hand-tune, and WHERE the
optimum lands. If the best null sits away from nominal (e.g. drive 0.55 vs the labelled 0.50),
that's a calibration offset in the knob->param mapping; if it's at nominal, the residual is a
genuine model limit that re-tuning can't remove.

Usage:
  null_optimize.py "G5 T5 OD" [--span 0.1] [--step 0.05] [--seg sweep_drv_-12]
"""
import argparse
import glob
import os
import subprocess
import sys
import numpy as np

import gen_test_signal as gts
import analyze
from null_test import best_null, null_db, BANDS

FS = gts.FS
TIMES = gts.segment_times()
MODE = {"clean": 0, "boost": 0, "od": 1, "dist": 2, "distortion": 2, "overdrive": 1}


def pr_bin():
    for p in ("build/PedalRender_artefacts/Release/PedalRender", "build/PedalRender"):
        if os.path.exists(p):
            return p
    sys.exit("build PedalRender first")


def parse(label):
    import re
    m = re.match(r"G([\d.]+)\s*T([\d.]+)\s*(\w+)", label)
    return float(m.group(1)) / 10, float(m.group(2)) / 10, MODE[m.group(3).lower()]


def seg(x, name, guard=0.15):
    t0, t1 = TIMES[name]
    return x[int((t0 + guard) * FS):int((t1 - guard) * FS)]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("label")
    ap.add_argument("--span", type=float, default=0.10)
    ap.add_argument("--step", type=float, default=0.05)
    ap.add_argument("--seg", default="sweep_drv_-12")
    ap.add_argument("--render-dir", default="/tmp/monarch_opt")
    args = ap.parse_args()

    pr = pr_bin()
    os.makedirs(args.render_dir, exist_ok=True)
    cap = glob.glob(f"analysis/pedal_export2/{args.label}*.wav")
    if not cap:
        sys.exit(f"capture not found: {args.label}")
    real = analyze.load_mono(cap[0])
    d0, t0, clip = parse(args.label)

    offs = np.round(np.arange(-args.span, args.span + 1e-9, args.step), 3)
    drives = np.clip(d0 + offs, 0, 1)
    tones = np.clip(t0 + offs, 0, 1)

    print(f"=== {args.label}  nominal drive={d0:.2f} tone={t0:.2f} clip={clip}  seg={args.seg} ===")
    results = []
    for d in drives:
        row = []
        for t in tones:
            out = os.path.join(args.render_dir, f"d{d:.2f}_t{t:.2f}.wav")
            subprocess.run([pr, "analysis/test_signal_48k.wav", out, f"{d:.3f}", f"{t:.3f}",
                            "0.5", "0.0", str(clip)], check=True, capture_output=True)
            p = analyze.load_mono(out)
            c, pp = seg(real, args.seg), seg(p, args.seg)
            n = min(len(c), len(pp))
            r, g = best_null(c[:n], pp[:n])
            row.append(null_db(c[:n], r))
        results.append(row)

    print("\nnull depth (dB) grid — rows=drive, cols=tone:")
    print("drive\\tone " + "".join(f"{t:7.2f}" for t in tones))
    best = (1e9, None, None)
    for d, row in zip(drives, results):
        print(f"  {d:5.2f}   " + "".join(f"{v:7.1f}" for v in row))
        for t, v in zip(tones, row):
            if v < best[0]:
                best = (v, d, t)
    nom_i = list(drives).index(min(drives, key=lambda x: abs(x - d0)))
    nom_j = list(tones).index(min(tones, key=lambda x: abs(x - t0)))
    print(f"\nnominal (d={d0:.2f},t={t0:.2f}) null = {results[nom_i][nom_j]:+.1f} dB")
    print(f"BEST    (d={best[1]:.2f},t={best[2]:.2f}) null = {best[0]:+.1f} dB"
          f"  -> improvement {results[nom_i][nom_j]-best[0]:.1f} dB,"
          f" offset d{best[1]-d0:+.2f} t{best[2]-t0:+.2f}")


if __name__ == "__main__":
    main()
