# DSP Rules

## WDF Implementation

- Use `chowdsp_wdf` (header-only, C++17) for ALL circuit modelling
- Use the **compile-time API** (`chowdsp::wdft` namespace) — not the runtime `chowdsp::wdf` namespace
- **Use `double` precision for all WDF types** — `float` causes audible errors in NR iteration for diode models at audio frequencies
- All passive networks modelled as WDF port trees
- Nonlinear elements use explicit per-component datasheet parameters — never generic defaults
- Op-amp stages use ideal op-amp WDF model (JRC4580D; neither stage clips rails under normal use)
- R-type adaptors required for feedback topologies — derive scattering matrix from nodal equations
- **Never reconstruct the WDF tree at runtime for switch changes** — precomputed scattering matrices, switch via `setSMatrixData()`
- VREF (VB) treated as signal ground throughout — model bipolar
- Stage 1 (IC_A, non-inverting): linear WDF tree, no Newton-Raphson
- Stage 2 (IC_B, inverting): linear WDF tree for op-amp, R-type adaptor at feedback node
- Nonlinear stages (SW-1 soft-clip, SW-2 hard-clip): Newton-Raphson via chowdsp_wdf nonlinear solver

### prepareToPlay Requirements
Every `CapacitorT` must have `.prepare(sampleRate)` called in `prepareToPlay`. Missing this produces silence or wrong behaviour. Call on every cap in every WDF stage in both channels. Also reset both oversamplers.

### Stage 2 Polarity Inversion
Stage 2 (IC_B) is an **inverting** amplifier. `PolarityInverterT` IS required between the R-type adaptor and the ideal voltage source for Stage 2. Omitting it produces inverted polarity from Stage 2. Stage 1 (IC_A, non-inverting) does NOT need a polarity inverter.

## chowdsp_wdf API Reference (key types)

All in `chowdsp::wdft` namespace. Include: `#include <chowdsp_wdf/chowdsp_wdf.h>`

### Passive Elements
```cpp
wdft::ResistorT<double> r { 1.0e3 };
wdft::CapacitorT<double> c { 1.0e-6 };          // call c.prepare(sampleRate)
wdft::ResistorCapacitorSeriesT<double> rc { R, C };
wdft::ResistorCapacitorParallelT<double> rc { R, C };
```

### Adaptors
```cpp
wdft::WDFSeriesT<double, decltype(a), decltype(b)> s { a, b };
wdft::WDFParallelT<double, decltype(a), decltype(b)> p { a, b };
wdft::PolarityInverterT<double, decltype(s)> inv { s };  // required for Stage 2 (inverting)
```

### R-type Adaptor
```cpp
wdft::RtypeAdaptor<double, upPortIndex, ImpedanceCalculator, Port0Type, Port1Type, ...> rtype { port0, port1, ... };
// Switch clipping topologies without tree reconstruction:
rtype.setSMatrixData({{ ... }});
```

### Nonlinear Elements — Symmetric Diode Pairs
Both clipping stages use **symmetric** antiparallel diode pairs. Use `DiodePairT` for both.

```cpp
// MA856 soft-clip parameters — validated from Panasonic datasheet (Vf=0.82V@1mA, Vf≤1.0V@100mA):
constexpr double Is_MA856 = 7.74e-13;  // derived; NOT a placeholder
constexpr double n_MA856  = 1.512;     // derived; lower than 1N4148's 1.752
constexpr double Vt       = 25.85e-3;
// Rs_MA856 = 0.85 Ω (datasheet rf) — model as ResistorT in series if desired; negligible at guitar levels

// 1S1588 hard-clip parameters (= 1N914 = 1N4148 electrically):
constexpr double Is_1S1588 = 2.52e-9;
constexpr double n_1S1588  = 1.752;

// Antiparallel diode pair (identical diodes, symmetric clipping):
wdft::DiodePairT<double, decltype(next), wdft::DiodeQuality::Best> dp { next, Is, Vt, n };
// Use DiodeQuality::Best — Wright Omega approximation, correct for audio
```

**Do NOT use separate `DiodeT` instances for each polarity.** Both stages use symmetric `DiodePairT` pairs.

### SW-1 Soft-Clip Configuration (two pairs in parallel)
The feedback loop has TWO antiparallel MA856 pairs in parallel (D2+D3 ∥ D4+D5). Model as two `DiodePairT` instances in a WDF parallel adaptor. Both pairs have identical MA856 parameters. Having two pairs in parallel lowers effective clipping onset slightly — do not simplify to one pair.

```cpp
wdft::DiodePairT<double, decltype(next), wdft::DiodeQuality::Best> dp1 { next, Is_MA856, Vt, n_MA856 };
wdft::DiodePairT<double, decltype(next), wdft::DiodeQuality::Best> dp2 { next, Is_MA856, Vt, n_MA856 };
wdft::WDFParallelT<double, decltype(dp1), decltype(dp2)> diodePairs { dp1, dp2 };
```

### Voltage Readout
```cpp
double output = wdft::voltage<double>(element);
```

### Deferred Impedance Propagation
```cpp
{
    wdft::ScopedDeferImpedancePropagation deferrer { port0, port1 };
    r_drive.setResistanceValue(newDriveR);
} // propagation fires once
```

## Dual-Channel Implementation

Both channels are identical circuits. Implement a single `MonarchChannel` class and instantiate it twice:

```cpp
class MonarchChannel {
    InputFilter     inputFilter;
    Stage1          stage1;       // IC_A non-inverting
    Stage2          stage2;       // IC_B inverting + clipping
    ToneStage       tone;         // passive RC
    VolumePot       volume;
    juce::dsp::Oversampling<double> oversampler;
};

MonarchChannel channelA;
MonarchChannel channelB;
```

Channel routing:
```
inputSample → channelA.process() → channelA_output → channelB.process() → output
```

Each channel has independent APVTS parameters (e.g., `drive_a`, `drive_b`, `tone_a`, `tone_b` etc.).

## Oversampling

- Apply to SW-1 (soft-clip feedback diodes) and SW-2 (hard-clip shunt diodes) in both channels only — linear stages never oversampled
- Use `juce::dsp::Oversampling` — one oversampler per channel (wraps both clipping stages together)
- Two independent oversampling settings exposed in APVTS:
  - `oversampling_realtime` — applied during live playback; default **4x**
  - `oversampling_render` — applied when DAW is rendering/bouncing (detected via `AudioProcessor::isNonRealtime()`); default **8x**
  - Both are `AudioParameterChoice`: "1x" / "2x" / "4x" / "8x"
- The audio thread checks `isNonRealtime()` each processBlock and selects the appropriate factor. If the active factor differs from the currently initialised oversampler state, reinitialise (same `pendingOversamplingFactor` atomic mechanism, checked at block start).
- **Bypassed channels skip the oversampler entirely** — no upsample/downsample, raw pass-through. Do not run the oversampler on a bypassed channel even if the factor is >1x. This halves the cost when only one channel is in use.
- 4x + first-order ADAA is sufficient for inaudible aliasing on this circuit (gentle feedback clipping, not hard rail clipping). 8x render provides additional headroom for the final bounce with no CPU penalty in a non-realtime context.

## ADAA

- Apply to both soft-clip and hard-clip diode stages
- ADAA in addition to oversampling, not instead
- Reference: DAFx2020 "Antiderivative antialiasing in nonlinear wave digital filters"

## Hi Gain Mod — Stage 1 Scattering Matrix Switch

SW-3 (Hi Gain) switches R29 (22k) in parallel with R8 (27k) in Stage 1's lower feedback
network. Stage 1 is linear — no NR, no oversampling involved. Only the Stage 1 R-type
scattering matrix changes.

```cpp
// Hi Gain OFF: R8 = 27k (standard lower feedback)
constexpr double R8_standard = 27.0e3;

// Hi Gain ON: R8 ∥ R29, plus R27 (47Ω protection) in series in the switch leg
// R8_eff = (27k ∥ 22k) + 47Ω ≈ 12168 Ω
constexpr double R8_hi_gain = (27.0e3 * 22.0e3) / (27.0e3 + 22.0e3) + 47.0; // ≈ 12168 Ω
```

Precompute two Stage 1 scattering matrices in `prepareToPlay`:
1. **Standard** (Hi Gain OFF): Z_lower built with R8 = 27k
2. **Hi Gain** (Hi Gain ON): Z_lower built with R8_eff ≈ 12168 Ω

Switch at block start via `setSMatrixData()` when `pendingHiGainA/B` changes. No tree reconstruction.

**Effect on Stage 1 gain range:**
- Hi Gain OFF: Stage 1 Av sweeps ~2.3–6.1× across DRIVE range
- Hi Gain ON: Stage 1 Av sweeps ~3.8–12.2× across DRIVE range (+4 dB shift)

The Hi Gain and clipping mode changes are independent — both can change simultaneously
within the same block start check. Handle them as separate atomic flags.

## Pot Tapers

- **DRIVE (100kB):** Linear. `R = R_max * x`
- **TONE (25kB):** Linear. `R = R_max * x`
- **VOL (100kA):** Audio. `R = R_max * pow(10.0, 2.0 * x - 2.0)` or equivalent
- **PRESENCE (50kB):** Linear. `R = R_max * x`

Never apply audio taper to DRIVE, TONE, or PRESENCE. Never apply linear taper to VOL.

## Component Values

See `circuit.md` for full table with schematic reference designators. Key values per channel
(using matsumin schematic refs as primary):
- R8 = 27k (Stage 1 lower feedback; Hi Gain: ∥ R29=22k → 12.1k effective)
- R7 = 33k (Stage 1 upper feedback; C4 100pF in parallel)
- R4 = 1M (Stage 1 DC bias at pin 2)
- R9 = 10k (Stage 2 input resistor; Av = –R10/R9 = –22)
- R10 = 220k (Stage 2 feedback resistor)
- R11 = 6.8k (hard-clip shunt series R; always in signal path)
- R12 = 1k (Tone stage series R)
- R13 = 6.8k (Presence network series R)
- R14 = 1M (output bleed)
- C3 = 10nF (input coupling cap, matsumin ref)
- C4 = 100pF (Stage 1 feedback HF cap)
- C7 = **100nF** (Stage 2 input coupling; confirmed from BOTH schematics — previous 10nF was wrong)
- C8, C9 = 10nF (tone stage caps)
- C11 = 1µF (output coupling)
- DRIVE = 100kB, TONE = 25kB, VOL = 100kA, Trim = 50kB

## Signal Calibration

- Internal nominal: **-12 dBu**
- Input trim and output trim on the plugin (not on the original pedal)
- Input trim → VU meter → Channel A → Channel B → VU meter → Output trim

## processBlock Structure (per channel)

```
1. Determine active oversampling factor:
   - isNonRealtime() ? read oversampling_render : read oversampling_realtime
   - If active factor != current factor: set pendingOversamplingFactor, reinit oversampler
2. Check pendingClippingMode (per channel) — update SW-1/SW-2 scattering matrices if changed
3. Check pendingHiGain (per channel) — update Stage 1 scattering matrix if changed
4. Read APVTS parameter values (smoothed) for this channel
5. Apply taper conversion (audio taper to VOL only)
6. Update WDF node values
7. If channel bypassed:
   - Copy input → output directly (no DSP, no oversampler)
   - Return early
8. Else:
   a. Upsample clipping stage block (active factor may be 1x = no-op)
   b. Process full WDF chain sample by sample
   c. Downsample clipping stage block
```

Global processBlock chains: inputTrim → channelA.process() → channelB.process() → outputTrim
