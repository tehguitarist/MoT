# Monarch of Tone — Project Memory

Monarch of Tone is a circuit-level emulation of the Analog Man King of Tone overdrive pedal,
built as an AU/VST3 plugin using JUCE 8+ and chowdsp_wdf Wave Digital Filter modelling.
**Author/Company:** Leigh Pierce

The King of Tone is a **dual-channel** Bluesbreaker-derived overdrive. Both channels are
modelled — 1-to-1 digital clone. Channels run in series and are independently bypassable.
The two channels are named externally by their LED colour: **Yellow** (first) → **Red**
(second). The Theseus Hi-Gain mod is a **fixed** part of the **Red** channel only (not a
runtime toggle); Yellow is always stock.

---

## Rule Files (read before touching any code)

@.claude/rules/circuit.md      ← circuit topology, all component values, signal flow
@.claude/rules/dsp.md          ← WDF implementation rules, API reference, diode parameters
@.claude/rules/architecture.md ← plugin structure, APVTS parameters, threading model
@.claude/rules/ui.md           ← layout, controls, colour scheme
@.claude/rules/build.md        ← CMake setup, project structure, validation gates

**Agents:**
- `dsp-validator` — invoke after implementing each DSP stage. Do not proceed to the next
  stage without a PASS verdict.
- `schematic-checker` — invoke for any circuit topology question. Never guess a component
  value or connection from memory.

---

## Quick Reference

```bash
# First-time setup
git submodule add https://github.com/juce-framework/JUCE libs/JUCE          # JUCE 8+
git submodule add https://github.com/Chowdhury-DSP/chowdsp_wdf libs/chowdsp_wdf
git submodule add https://github.com/xtensor-stack/xsimd libs/xsimd         # optional SIMD

# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build

# Individual targets
cmake --build build --target Monarch_AU     # AU (primary)
cmake --build build --target Monarch_VST3   # VST3

# Code quality
clang-tidy src/**/*.cpp
clang-format -i src/**/*.{cpp,h}
```

**CMake minimum:** 3.15 | **C++ standard:** 17 | **macOS target:** 10.13+

---

## Repository State

> **CURRENT: Step 4 — Stage-by-stage DSP**

The `.claude/` rules and agents are complete. The Step 2 JUCE CMake scaffold and Step 3
chowdsp_wdf smoke test are both in place and verified.

---

## Build Sequence — Do Not Skip Ahead

1. ✅ **Schematic analysis** — complete. All circuit values verified, MA856 validated, Hi Gain mod characterised.
2. ✅ **JUCE CMake scaffold** — complete. CMakeLists.txt, PluginProcessor, PluginEditor, full
   APVTS parameter layout, MonarchChannel stub, and TaperUtils all in place. AU validated
   with `auval`. (Parameter set later trimmed: the two `hi_gain_*` params were removed when
   Hi Gain became a fixed Red-channel mod — see Step 5 / circuit.md Section 6.)
3. ✅ **chowdsp_wdf smoke test** — complete. `tests/SmokeTest_RC.cpp` implements a 1kHz RC
   lowpass via `chowdsp::wdft` (double precision); measured -3.018 dB at the theoretical
   -3dB corner, confirmed PASS.
4. **Stage-by-stage DSP** ← CURRENT STEP — implement and validate each before moving on:
   - ✅ `Stage1` (IC_A, non-inverting) — **DONE & validated (dsp-validator PASS).** Input
     network (C3/R4/R5 high-pass sub-filter) + root R-type op-amp gain stage (Z_lower =
     (R7+C5)∥(R8+C6), Z_upper = C4∥(R6+DRIVE)). Scattering matrix from `tools/r_solver_sympy.py`
     (validated sympy port of R-Solver) with ideal-op-amp limit. `tests/Stage1_FreqResponse.cpp`:
     peak +13.93 dB @ 3780 Hz (96k; analog 3803 Hz, −23 Hz bilinear warp), DC shelf −0.08 dB,
     DRIVE +4.45→+18.22 dB monotonic. Accurate at base rate — no oversampling/prewarp needed
     (an output-reconstruction bug, since fixed, had caused a ~−880 Hz error). See `src/dsp/Stage1.h`.
   - `Stage1` Hi Gain — now a **fixed mod on the Red channel only** (decision 2026-06-17), not
     a runtime toggle. Topology still ⚠️ **under revision** (Theseus trace 2026-06-16): actual
     element is SW1B switching R3=1k in the Stage-1 DRIVE feedback, NOT the old R29∥R8 Z_lower
     model (R29/R27 are power-supply, not gain stage). Until pinned, **Red falls back to the
     stock Stage-1 voicing** so the build proceeds. Confirm wiring before swapping in the
     Hi-Gain matrix — see circuit.md Section 6.
   - ✅ `Stage2` (IC_B, inverting) — **DONE & validated (dsp-validator PASS).** Root R-type
     (op-amp VCVS), input C7(100nF)+R9(10k), feedback R10(220k). `tests/Stage2_Gain.cpp`:
     passband 21.90× (−22 target), −3 dB corner 159 Hz exactly, signed gain −21 (inverting).
     Inversion via VCVS terminals (in+=BIAS, in−=pin6−), no PolarityInverterT; output read off
     passive R10 port. SW-1 diodes (R10 ∥ [R11+diodes]) added next. See `src/dsp/Stage2.h`.
   - ✅ `SW1SoftClip` — **DONE & validated (dsp-validator PASS).** ONE `DiodePairT`
     (n_eff=2×n_MA856≈3.024 via the `nDiodes` arg), R11(6.8k) series, R10 parallel.
     Implemented as current-source/diode-root (op-amp virtual ground → known i_in drives
     R10 ∥ [R11+diode], diode = nonlinear root). `tests/SW1SoftClip_Sine.cpp`: small-signal
     −21.5 (inverting), perfectly symmetric, soft knee ≈1.64V (confirms n_eff). See `src/dsp/SW1SoftClip.h`.
   - ✅ `SW2HardClip` — **DONE & validated (dsp-validator PASS).** 1S1588×2 true antiparallel
     (ONE `DiodePairT`, n=1.752), shunt at node_HC via always-present R12=1k. Current-source...
     no — series-R + shunt-diode-root. `tests/SW2HardClip_Sine.cpp`: gain ≈+1, symmetric, HARD
     clamp ±0.55V (rises only to 0.66V @ 10× drive = diode log). See `src/dsp/SW2HardClip.h`.
   - **Run `dsp-validator` after each stage. Do not proceed on FAIL.**
5. **4 clipping modes per channel** — Boost/OD/Dist/Both (`clipping_mode_*`). Hi Gain is no
   longer a runtime axis: Yellow is fixed-standard, Red is fixed-Hi-Gain, so the old
   "× Standard/HiGain" 8-mode matrix collapses to 4 modes each (circuit.md Section 18).
6. **Tone stage** — passive RC; TONE is a 3-terminal pot tap (R-type adaptor at the
   wiper: R_a toward node_HC, R_b+C8 toward BIAS, R13 toward node_T_out/Presence/VOL) —
   topology fully resolved, see circuit.md Section 11
7. **Oversampling + ADAA** on both clipping stages — verify aliasing reduction
8. **Dual-channel integration** — Yellow→Red in series, independent bypass; Hi Gain fixed on Red
9. **UI implementation** — both channel panels (Yellow/Red, no Hi Gain toggle), oversampling controls
10. **Final sweep** — all controls full range, no instability, clicks, or NaN output

---

## Key Circuit Facts

| Fact | Value |
|------|-------|
| Op-amp | JRC4580D per channel (matsumin schematic label JRC4558D is wrong) |
| Stage 1 (IC_A) | Non-inverting — no `PolarityInverterT` |
| Stage 2 (IC_B) | **Inverting** — `PolarityInverterT` required |
| Stage 2 DC gain | –22 (R10=220k feedback / R9=10k input) |
| Stage 2 HPF | 159 Hz (C7=100nF, R9=10k) |
| Soft-clip diodes SW-1 | MA856 ×4 as `[D4+D5]∥[D2+D3]` back-to-back series strings ≡ ONE `DiodePairT` with n_eff=2×1.512≈3.024, Is=7.74e-13; in series with R11=6.8k, branch ∥ R10=220k, gated by SW-1 |
| Hard-clip diodes SW-2 | 1S1588 ×2 true antiparallel pair; one `DiodePairT` shunt at node_HC, fed via R12=1k (always-present Stage 2 output R); Is=2.52e-9, n=1.752 |
| Diode topology | **Symmetric pairs** — `DiodePairT` only, never `DiodeT` |
| Hi Gain | **Fixed mod on the Red channel only** (not a runtime toggle; no `hi_gain_*` param). Yellow always stock. Topology ⚠️ **UNDER REVISION** — Theseus trace shows R29/R27 are power-supply (LED/VCC-filter), NOT gain stage; actual element = SW1B switching R3=1k in Stage-1 DRIVE feedback, exact wiring TBC (Red falls back to stock until pinned). See circuit.md Section 6. Do not implement the old R29∥R8 model. |
| Stage 1 feedback | Z_lower=(R7+C5)∥(R8+C6); Z_upper=C4∥(R6+DRIVE 0-100k); Av(s)=1+Zupper/Zlower |
| DRIVE taper | 100kB **linear**; 2-terminal rheostat inside Stage 1 Z_upper only |
| TONE taper | 25kB **linear**; 3-terminal pot tap (R-type adaptor at wiper) — see circuit.md Section 11 |
| VOL taper | 100kA **audio** (`pow(10, 2x-2)`) |
| Presence taper | 50kB **linear**; default fully CCW; 2-terminal rheostat (like DRIVE) from node_T_out |
| Tone stage | Passive RC only — no diodes; 3-terminal TONE pot tap, not two parallel branches |
| Channel routing | Yellow → Red in series; independently bypassable |
| Channel names | Yellow (first, stock) → Red (second, fixed Hi-Gain), after the LED colours |
| Default mode | Overdrive (SW-1 ON, SW-2 OFF) per channel |
| Gain peak | **+13.93 dB @ 3780 Hz (96k)**, analog 3803 Hz (−23 Hz; −74 Hz @ 48k). Accurate at base rate — linear stages need no oversampling/prewarp (earlier large error was an output-recon bug, fixed; see dsp.md). |
| Oversampling live | 1x/2x/4x/8x; default **4x**; bypassed channels skip oversampler |
| Oversampling render | 1x/2x/4x/8x; default **8x**; auto via `isNonRealtime()` |

## Three Most Likely Implementation Mistakes

1. **`DiodeT` instead of `DiodePairT`** — both clipping stages use symmetric matched pairs.
2. **Audio taper on DRIVE or TONE** — both are linear (B-taper). Only VOL is audio taper.
3. **Missing `PolarityInverterT` on Stage 2** — it is inverting; omitting it produces wrong polarity.

## References

- CCRMA paper: https://ccrma.stanford.edu/~kaichieh/KingOfTone.pdf
- Theseus kit documentation: https://aionfx.com/app/files/docs/theseus_kit_documentation.pdf
- Primary schematic: `king_of_tone_schematic.png` (in project root)
