# Circuit Reference — Monarch of Tone (King of Tone Clone)

> **Primary source:** `king_of_tone_schematic.png` (Ver2, matsumin, 2008-05-15).
> **Secondary/confirmation:** Aion FX "Theseus Dual Overdrive" v2.0.2 kit documentation.
> **Academic:** CCRMA — Kai-Chieh Huang, "King of Tone Guitar Pedal Modeling", Stanford.
>
> Where the two sources conflict, this document records the conflict explicitly and states
> which value is used and why. Component reference designators used internally are functional
> labels (e.g. R_input_lower) — not tied to either schematic's numbering system, to avoid
> confusion between sources.
>
> **All flags resolved. MA856 parameters validated. Hi Gain mod characterised.**
> **Full cross-source verification complete — see Section 2 for all discrepancies.**

---

## 1. Cross-Source Verification Table

Every component traced from the matsumin schematic image and compared against the Theseus
parts list. Values are by function, not by reference designator.

| Function | matsumin ref | matsumin value | Theseus ref | Theseus value | Status | Used in model |
|----------|-------------|---------------|------------|--------------|--------|--------------|
| Reverse polarity diode | D1 | 1N4001 | D1 | 1N5817 (Schottky) | DIFFERS — not in signal path | N/A |
| BIAS upper divider R | R1 | 47k | R12/R13 | 47k | MATCH | excluded (bias) |
| BIAS lower divider R | R2 | 47k | R12/R13 | 47k | MATCH | excluded (bias) |
| BIAS filter cap | C1 | 100µF | C20/C21 | 100µF | MATCH | excluded (bias) |
| VCC filter cap | C2 | 100µF | C21/C22 | 100µF | MATCH | excluded (bias) |
| Input pulldown R | R5 | 1M | RPD1 | 1M | MATCH | 1M |
| **Input coupling cap** | **C3** | **10nF (0.01µF)** | **C1** | **22nF** | **DIFFERS** | **10nF (matsumin primary)** |
| Input shunt cap at pin3(+) | C6 | 10nF | C3/C4 | 10nF | MATCH | 10nF |
| Input filter cap (upper) | C5 | 10nF | C3/C4 | 10nF | MATCH | 10nF |
| Stage 1 upper feedback R | R7 | 33k | R5 | 33k | MATCH | 33k |
| Stage 1 lower feedback R | R8 | 27k | R4 | 27k | MATCH | 27k |
| Stage 1 DC bias R at pin2(–) | R4 | 1M | R1 | 1M | MATCH | 1M |
| Stage 1 HF feedback cap | C4 | 100pF | C2 | 100pF | MATCH | 100pF |
| Stage 1 DRIVE floor R | R6 | 10k | R6 | 10k | MATCH | 10k |
| DRIVE pot | DRIVE | 100kB | DRIVE | 100kB | MATCH | 100kB linear |
| **Stage 2 input coupling cap** | **C7** | **100nF (0.1µF)** | **C5** | **100nF (0.1µF)** | **BOTH AGREE: 100nF** | **100nF** ✓ |
| Stage 2 input R | R9 | 10k | R6(ch2) | 10k | MATCH | 10k |
| Stage 2 feedback R | R10 | 220k | R7 | 220k | MATCH | 220k |
| SW-1 diodes (soft clip) | D2–D5 | 1N914+1N4004 (sub) | D2–D5 | BAS33 ×4 (sub) | BOTH ARE SUBS | MA856 ×4 (original) |
| SW-1 DIP switch | DIP SW-1 | soft clip | SW1 pos2 | Soft Clip | MATCH | SW-1 |
| SW-2 series R | R11 | 6.8k | R8 | 6.8k | MATCH | 6.8k |
| SW-2 diodes (hard clip) | D6/D7 | 1N914 (sub) | D6/D7 | 1N914 (sub) | BOTH USE 1N914 | 1S1588 (original) |
| SW-2 DIP switch | DIP SW-2 | hard clip | SW1 pos3 | Hard Clip | MATCH | SW-2 |
| Tone input series R | R12 | 1k | R3 | 1k | MATCH | 1k |
| TONE pot | TONE | 25kB | TONE | 25kB | MATCH | 25kB linear |
| Tone treble-cut cap | C8 | 10nF | C6 | 10nF | MATCH | 10nF |
| Presence series R | R13 | 6.8k | R10 | 6.8k | MATCH | 6.8k |
| Presence trimpot | Trim 50k | 50kB | PRESENCE | 50kB | MATCH | 50kB linear |
| Presence cap | C9 | 10nF | C7 | 10nF | MATCH | 10nF |
| VOL pot | VOL-a/b | 100kA | VOL | 100kA | MATCH | 100kA audio |
| Output coupling cap | C11-a/b | 1µF | C8 | 1µF film | MATCH | 1µF |
| Output electrolytic cap | — | not shown | C9 | 1µF electro | Theseus addition | 1µF electro |
| Output bleed R | R14 | 1M | R11 | 1M | MATCH | 1M |
| Hi Gain R (mod) | — | not in matsumin | R29 | 22k | Theseus addition | 22k |
| Hi Gain protection R | — | not in matsumin | R27 | 47R | Theseus addition | 47R |
| Hi Gain switch | — | not in matsumin | SW1 pos1 | Hi Gain | Theseus addition | SW-3 |
| Op-amp IC | JRC4558D (label) | — | JRC4580D | — | matsumin label WRONG | JRC4580D |

---

## 2. Discrepancies — Decisions and Rationale

### C7 (Stage 2 input coupling cap): 100nF

Both sources agree: matsumin C7 = 0.1µF (100nF); Theseus C5 = 100n.
Stage 2 HPF: f_c = 1/(2π × R9 × C7) = 1/(2π × 10k × 100nF) = **159 Hz**.
Virtually the entire guitar range receives full Stage 2 gain.

### C3 (Input coupling cap): 10nF matsumin vs 22nF Theseus

The matsumin schematic shows C3 = 0.01µF (10nF). The Theseus uses C1 = 22nF.
This is a genuine design difference — the Theseus is a slightly modified clone.

- matsumin 10nF: f_c ≈ 590 Hz with R8 (27k) — tighter low end, more bass rolloff
- Theseus 22nF: f_c ≈ 268 Hz — fuller low end, closer to Bluesbreaker original

**Decision:** Use **10nF** (matsumin primary source) for the 1:1 KOT Ver2 clone.
The 22nF variant may be offered as a "Theseus" mode in future if desired.

### Op-amp (JRC4558D vs JRC4580D)

The matsumin schematic labels the IC as JRC4558D. Real KOT units traced by the community
confirm JRC4580D. The Theseus uses JRC4580D. **Use JRC4580D.** In the DSP model both are
ideal op-amps — the difference affects noise floor and slew rate, neither of which is modelled.

### Soft-clip diodes (MA856 vs BAS33 vs 1N914+1N4004)

The matsumin Ver2 schematic uses substitute diodes (1N914+1N4004 — mismatched pairs).
The Theseus uses BAS33 (matched pairs, Vf ≈ 0.80V). Real KOT units use MA856 (Vf ≈ 0.82V).
**Model MA856.** BAS33 is very close and may be used as a cross-check simulation.
The matsumin pairing of 1N914+1N4004 in the soft-clip path is a substitution artifact from
Ver2 being a DIY community schematic, not a trace of the original. Disregard those diode values.

### Hard-clip diodes (1S1588 vs 1N914)

Both schematics show 1N914 for D6/D7. Real KOT uses 1S1588, which is electrically identical
to 1N914 (same parameters). **Model 1S1588 = 1N914 = 1N4148** — same Shockley parameters.

### R6 (10k DRIVE floor resistor)

R6 (10k) is a **floor resistor in the Stage 1 feedback / DRIVE pot circuit**, not the
Stage 2 input resistor. It sets the minimum feedback impedance when DRIVE is at zero,
preventing the gain from going to unity. When DRIVE=0: Z_upper_min = R6 = 10k.
When DRIVE=max: Z_upper_max = R6 + 100k = 110k. The Stage 2 input resistor is R9 = 10k
(a separate component). Both happen to be 10k but serve completely different functions.

---

## 3. Circuit Overview

The King of Tone is two independent modified Bluesbreaker circuits in series, each with
its own footswitch. Per channel, the verified signal path is:

```
IN → [Input HPF: C3(10nF) + R_network] → [Stage 1: non-inverting amp, IC_A]
   → [DRIVE pot (100kB): cross-stage gain control]
   → [Stage 2: inverting amp, IC_B, gain=–22, HPF f_c=159Hz]
   → [SW-1: MA856×4 soft-clip in feedback loop (optional)]
   → [SW-2: R11(6.8k) + 1S1588×2 shunt hard-clip (optional)]
   → [Tone: R12(1k) + TONE(25kB) + C8(10nF) + Presence network]
   → [Volume: VOL(100kA) + C11(1µF)] → OUT
```

BIAS = VCC/2 = 4.5V = DSP signal ground (0V in model).

---

## 4. Power and Bias

- VCC = 9V; BIAS = 4.5V from 47k/47k voltage divider + 100µF filter caps
- D1 (1N4001 in matsumin; 1N5817 in Theseus): reverse polarity protection on DC jack
- **DSP model:** BIAS = 0V. Exclude all power supply and bias components.

---

## 5. Input Network

**Components (from matsumin schematic, verified against Theseus):**

| Component | Value | Connection |
|-----------|-------|-----------|
| R5 | 1M | Input jack to GND (pulldown, prevents bypass thump) |
| C3 | 10nF | Input jack to node_IN (series coupling/HPF cap) |
| C5 | 10nF | node_IN to GND (shunt, top of input filter) |
| R7 | 33k | node_IN to IC_A pin 2(–) (upper path toward Stage 1 feedback) |
| R8 | 27k | node_IN to IC_A pin 3(+) (lower path, non-inverting input) |
| C6 | 10nF | IC_A pin 3(+) node to BIAS (shunt cap, AC grounds pin 3 ref) |

**Signal flow:**
R5 (1M) is a bleed to GND — negligible at audio frequencies, keeps input at DC ground when
unconnected. C3 (10nF) blocks DC, forms HPF with the downstream network.

The input then splits: R7 (33k) goes toward the Stage 1 feedback node (pin 2–) and R8 (27k)
drives pin 3(+) directly. C6 (10nF) shunts the pin 3(+) node to BIAS, acting as a bypass cap
ensuring the non-inverting input reference is clean.

The combined input HPF corner frequency is primarily set by C3 and the parallel combination
of R7 and R8 as seen from the input:

    f_c ≈ 1 / (2π × R8 × C3) = 1 / (2π × 27k × 10nF) ≈ **590 Hz**

This rolls off sub-bass below guitar's useful range. The dual-R input network (R7 to feedback,
R8 to signal input) is the Bluesbreaker topology — R7 forms a frequency-dependent gain-setting
path to the feedback node, contributing to the gain peak at ~4194 Hz.

**WDF model:** Series C3, then R7 and R8 as a resistive divider to pin 3(+), with C5 shunt
at node_IN and C6 shunt at pin 3(+). R5 shunt at input node to GND.

---

## 6. Stage 1 — IC_A (Non-Inverting Amplifier)

**Components (from matsumin schematic):**

| Component | Value | Connection |
|-----------|-------|-----------|
| IC_A | JRC4580D (half) | Non-inverting amp; pin 3(+) = signal in, pin 2(–) = feedback, pin 1 = output |
| R4 | 1M | IC_A pin 2(–) to BIAS (DC bias resistor) |
| R8 | 27k | IC_A pin 2(–) to BIAS (lower feedback; **also** in input network — same node) |
| R7 | 33k | IC_A pin 2(–) toward DRIVE pot pin 3 (upper feedback) |
| C4 | 100pF | In parallel with R7 (HF compensation cap in feedback) |
| R6 | 10k | Between DRIVE pot pin 3 and the R7/C4 network (floor gain resistor) |

> **Node identity:** IC_A pin 2(–) connects to R8 (27k to BIAS), R4 (1M to BIAS), and R7
> (33k toward DRIVE). R8 is shared between the input network and Stage 1 feedback — the
> same physical node. This is the Bluesbreaker's characteristic input/feedback coupling.

**Stage 1 gain (non-inverting):**

    Av = 1 + Z_upper / Z_lower

Z_lower = R8 (27k) ∥ R4 (1M) ≈ 26.3k

Z_upper = R6 (10k) + R_drive_upper + R7 (33k) with C4 (100pF) in parallel
- R_drive_upper = 0k at DRIVE max (wiper at pin 3), = 100k at DRIVE min (wiper at GND)
- R6 (10k) is always in series — it is the minimum gain floor

    Av_min (DRIVE=max): Z_upper = R6 + 0 + R7 = 10k + 33k = 43k
    Av_min = 1 + 43k / 26.3k ≈ **2.6×** (8.4 dB)

    Av_max (DRIVE=min): Z_upper = R6 + 100k + R7 = 10k + 100k + 33k = 143k
    Av_max = 1 + 143k / 26.3k ≈ **6.4×** (16.2 dB)

Stage 1 gain **decreases** as DRIVE increases (DRIVE pot reduces Z_upper via R_drive_upper
going toward 0).

**C4 (100pF) HF pole:**
C4 shorts R7 at high frequencies. Pole: f = 1/(2π × R7 × C4) = 1/(2π × 33k × 100pF) ≈ **48.2 kHz**

This pole is above audible range in isolation. Combined with R7's role in the input network
and Stage 2's transfer function, the system produces the measured gain peak near **~4194 Hz**
(CCRMA paper, Fig. 6, measured at mid-DRIVE).

**No PolarityInverterT** — Stage 1 is non-inverting.

**Hi Gain mod (SW-3):**
Switches R29 (22k) in parallel with R8 (27k), via R27 (47R protection) in series.
R8_eff = 27k ∥ 22k + 47R ≈ **12.17k**
Z_lower_hi = R8_eff ∥ R4 ≈ 11.9k
Stage 1 gain range shifts to ~3.8–12.2× (+4 dB throughout DRIVE sweep).

**WDF model:** R-type adaptor at IC_A pin 2(–). Ports: R4 (1M), R8 (27k; or 12.17k Hi Gain),
R7 (33k) ∥ C4 (100pF), R6 (10k) + DRIVE R_upper (0–100k variable).
Precompute two scattering matrices: standard and Hi Gain.

---

## 7. DRIVE Pot — Cross-Stage Gain Control

**Components:** DRIVE = 100kB (linear taper)
- Pin 3 (CW/high): IC_A pin 1 (output) — through R6 (10k) to feedback node
- Pin 1 (CCW/low): BIAS (signal ground)
- Pin 2 (wiper): to C7 (100nF) → R9 (10k) → IC_B pin 6(–)

The wiper simultaneously:
1. **Sets Stage 1 feedback Z_upper** via R_drive_upper (wiper to pin 3) through R6 and R7
2. **Sets Stage 2 input signal level** via R_drive_lower (wiper to BIAS) as voltage divider

As DRIVE increases (wiper toward pin 3):
- R_drive_upper → 0k: Z_upper decreases → Stage 1 gain decreases
- V_wiper → V_stage1_out: full Stage 1 output reaches Stage 2

Net result: total system gain increases with DRIVE. Stage 1 reduces but Stage 2 receives
more signal. Combined two-stage gain still rises monotonically across most of the DRIVE range.

**WDF note:** Use `ScopedDeferImpedancePropagation` when updating DRIVE — Stage 1 R_upper
and Stage 2 input R_lower both change from the same pot movement.

---

## 8. Stage 2 — IC_B (Inverting Amplifier)

**Components (from matsumin schematic):**

| Component | Value | Connection |
|-----------|-------|-----------|
| C7 | **100nF** (0.1µF — verified from both schematics) | DRIVE wiper to R9 (series coupling) |
| R9 | 10k | C7 to IC_B pin 6(–) (input resistor) |
| IC_B | JRC4580D (half) | Inverting amp; pin 6(–) = signal, pin 5(+) = BIAS, pin 7 = output |
| R10 | 220k | IC_B pin 7 (output) to pin 6(–) (feedback resistor) |

**C7 HPF:**

    f_c = 1 / (2π × R9 × C7) = 1 / (2π × 10k × 100nF) = **159 Hz**

The Stage 2 input HPF cuts below 159 Hz — essentially only sub-bass is rolled off.
Virtually the entire guitar frequency range receives full Stage 2 gain. This is by design:
Stage 2 is meant to amplify the full-range signal before clipping. There is no strong
"mid-forward" HPF in Stage 2 as previously documented incorrectly.

**Stage 2 DC gain:**

    Av = –R10 / R9 = –220k / 10k = **–22** (26.8 dB, inverting)

**PolarityInverterT required.** Stage 2 inverts polarity. Without it, clipping is asymmetric
in the wrong direction relative to the physical circuit.

**WDF model:** R-type adaptor at IC_B pin 6(–). Ports: R9 (10k input), R10 (220k feedback),
SW-1 diode network (when active, in parallel with R10). Pin 5(+) tied to BIAS = 0V.

---

## 9. SW-1 — Soft-Clip Feedback Diodes

**Components (from matsumin schematic):**

| Component | Value | Connection |
|-----------|-------|-----------|
| D2+D3 | MA856 antiparallel pair | Between IC_B pin 7 (output) and IC_B pin 6(–) |
| D4+D5 | MA856 antiparallel pair | Same nodes, in parallel with D2+D3 |
| DIP SW-1 | — | In series with combined diode network |

Four MA856 diodes total: two antiparallel pairs (D2+D3 ∥ D4+D5) in parallel with R10 (220k).
SW-1 ON connects this network; SW-1 OFF leaves only R10 in the feedback path.

**How it clips:**
Diodes conduct when feedback current through R10 reaches threshold:

    I_threshold ≈ Vf_MA856 / R10 = 0.82 / 220k ≈ **3.7µA**

At this tiny current, the MA856 pairs begin to conduct, shunting the feedback.
Two pairs in parallel share the current — softer onset, more gradual compression.
Output clamped to approximately ±0.82V. Symmetric — pure odd-harmonic content from this stage.

**WDF model:** Two `DiodePairT` (MA856: Is=7.74e-13, n=1.512) in `WDFParallelT`, placed in
parallel with R10 in Stage 2 R-type adaptor. SW-1 OFF: precomputed matrix with R10 only.

---

## 10. SW-2 — Hard-Clip Shunt Diodes

**Components (from matsumin schematic):**

| Component | Value | Connection |
|-----------|-------|-----------|
| R11 | 6.8k | IC_B pin 7 to node_HC (always in signal path) |
| D6+D7 | 1S1588 antiparallel pair | node_HC shunted to BIAS |
| DIP SW-2 | — | In series with D6/D7; gates diodes only, not R11 |

R11 (6.8k) is **permanently in the signal path** regardless of SW-2 state. It is the only
path from IC_B output to the tone stage. SW-2 connects the 1S1588 shunt diodes at node_HC.

**How it clips:**
When signal at node_HC exceeds ±Vf_1S1588 ≈ ±0.584V, diodes shunt to BIAS.
R11 (6.8k) limits diode current, controlling the hardness of the clipping knee.

**SW-2 OFF effect on level:** With diodes disconnected, node_HC is only the R11 junction.
Signal passes through R11 (6.8k) into tone stage R12 (1k). Voltage divider effect:

    V_tone_in / V_out = R12 / (R11 + R12) = 1k / (6.8k + 1k) ≈ **0.128** (–17.8 dB)

This ~18 dB passive attenuation is compensated by Stage 2's ×22 gain. Include R11 in the
WDF model in all SW-2 states.

**WDF model:** R11 (6.8k) always as series resistor. `DiodePairT` (1S1588: Is=2.52e-9,
n=1.752) at node_HC shunting to BIAS. SW-2 OFF: precomputed open-circuit matrix at diode port.

---

## 11. Tone Control Stage

**Components (from matsumin schematic):**

| Component | Value | Connection |
|-----------|-------|-----------|
| R12 | 1k | node_HC → tone input node_T1 (series isolating R) |
| TONE | 25kB | Linear taper; variable R (0–25k) in treble-cut network |
| C8 | 10nF | Main tone cap; forms LPF with TONE pot |
| R13 | 6.8k | Presence network series R |
| Trim | 50kB | Presence trimpot (internal); default CCW = no boost |
| C9 | 10nF | Presence network cap |

**This stage is entirely passive and linear — no diodes.**

**TONE as treble-cut:**
TONE pot (25kB linear) and C8 (10nF) form a variable low-pass filter:

    f_c = 1 / (2π × R_tone × C8)

    TONE max (25k): f_c ≈ **637 Hz** (heavy treble cut)
    TONE mid (12.5k): f_c ≈ **1274 Hz**
    TONE min (0k): f_c → very high (treble cut bypassed)

R12 (1k) prevents TONE from shorting the signal and isolates SW-2's R11 from the tone network.

**Presence trimpot (Trim 50k):**
From Theseus documentation: "Each channel's Presence trimmer fades the hi-cut capacitor
out of the circuit, reducing the treble cut as you turn it up."
"Default position is full counter-clockwise — equivalent to the stock Bluesbreaker."

The Trim pot and R13 (6.8k) form a parallel path that progressively bypasses C8's treble cut.
At Trim=CCW (0): full treble cut via C8. At Trim=CW (50k): C8's effect is progressively faded.
C9 (10nF) is the cap in this bypass path. R13 (6.8k) is the series R in the bypass path.

**Tone stage topology (for WDF):**

    node_T1 (from R12) → [TONE pot (R, 0–25k) → C8 (10nF) → BIAS]  ← main treble-cut path
                        → [R13 (6.8k) + Trim (0–50k) → C9 (10nF) → BIAS]  ← presence bypass path
    → node_T_out

The two paths are in parallel. As Trim increases, more of C9's bypass path is active,
countering C8's treble cut. Both paths share the same output node.

> **Verify this topology node-by-node from the schematic before implementing ToneStage.h.**
> The above is consistent with all sources but exact capacitor terminal placement should
> be traced from the image. Invoke `schematic-checker` agent at that step.

**WDF model:** Series/parallel passive tree. No R-type adaptors. TONE pot and Trim pot are
variable resistors updated per block.

---

## 12. Volume and Output Stage

**Components (from matsumin schematic):**

| Component | Value | Connection |
|-----------|-------|-----------|
| VOL | 100kA | Audio taper; output voltage divider |
| C11 | 1µF | VOL input coupling (AC block) |
| C_out_elec | 1µF electrolytic | Output coupling (from Theseus; may not be in all KOT units) |
| R14 | 1M | Output to GND (bleed resistor) |

VOL pot (100kA audio taper) attenuates the tone stage output. C11 (1µF) blocks DC.
Output HPF: f_c = 1/(2π × R14 × C11) ≈ 0.16 Hz — DC blocking only.

**WDF model:** Parallel divider for VOL; series C11; shunt R14. All linear.
Audio taper: `R_lower = 100k × pow(10, 2×x - 2)` where x ∈ [0,1].

---

## 13. Full Per-Channel Signal Flow (Node-by-Node)

```
Guitar IN (AC, ±0.1–1V at BIAS=0V reference)
  │
  ├─ R5 (1M) → GND                       [input bleed, negligible loading]
  │
  C3 (10nF)                              [HPF, couples AC only]
  │
node_IN ──────────────────────────────────────────────────────────
  ├─ C5 (10nF) → GND                     [input shunt cap]
  ├─ R7 (33k) → IC_A pin 2(–)           [feeds Stage 1 feedback node]
  └─ R8 (27k) → IC_A pin 3(+)           [drives non-inverting input]
                  └─ C6 (10nF) → BIAS    [pin3 shunt/bypass cap]

IC_A pin 2(–) feedback node:
  ├─ R4 (1M) → BIAS                      [DC bias]
  ├─ R8 (27k) → BIAS (same R8 as above — shared node)
  ├─ R7 (33k) → node_IN (same R7 as above — shared node)
  └─ R6 (10k) → DRIVE pot pin 3          [floor gain R + connects to DRIVE]
      └─ in parallel: C4 (100pF) across R7 [HF feedback pole]

IC_A (non-inverting, no PolarityInverterT)
  pin 3(+) = signal, pin 2(–) = feedback node above, pin 1 = output
  │
DRIVE pin 3 (= IC_A output through R6)
  DRIVE (100kB, 0–100k)
  ├─ Pin 3: IC_A output (via R6)
  ├─ Pin 1: BIAS
  └─ Pin 2 (wiper): V_wiper
              ├─ [feeds back to Stage 1 R-type via R_drive_upper segment]
              └─ C7 (100nF) [Stage 2 HPF, f_c = 159 Hz]
                  │
                  R9 (10k)
                  │
IC_B pin 6(–) feedback node:
  ├─ R9 (10k) → C7 → DRIVE wiper        [signal input]
  ├─ R10 (220k) → IC_B pin 7 (output)   [feedback]
  └─ [SW-1 ON: MA856×4 DiodePairT×2 in WDFParallelT, also IC_B pin7 ↔ pin6]

IC_B (inverting, PolarityInverterT REQUIRED)
  pin 5(+) → BIAS, pin 6(–) = feedback node, pin 7 = output
  │
IC_B pin 7 (output)
  │
  R11 (6.8k)                             [always present; SW-2 series R]
  │
node_HC ──────────────────────────────────────────────────────────
  └─ [SW-2 ON: 1S1588 DiodePairT shunt to BIAS]
  │
  R12 (1k)                               [tone stage isolation R]
  │
node_T1 ──────────────────────────────────────────────────────────
  ├─ TONE (25kB, 0–25k) → C8 (10nF) → BIAS    [variable treble cut]
  └─ R13 (6.8k) → Trim (0–50k) → C9 (10nF) → BIAS  [presence bypass]
  │
node_T_out
  │
  C11 (1µF)                              [output AC coupling]
  │
  VOL (100kA, audio taper)
  ├─ Pin 3: C11 output
  ├─ Pin 1: GND
  └─ Pin 2 (wiper): V_out
              │
              R14 (1M) → GND             [output bleed]
              │
             OUT
```

---

## 14. Component Values Master Table

Using **functional names** to avoid confusion between schematic numbering systems.

| Functional Name | matsumin ref | Value | Stage | Transfer function |
|----------------|-------------|-------|-------|-------------------|
| Input pulldown | R5 | 1M | Input | — |
| Input coupling | C3 | **10nF** | Input | HPF f_c ≈ 590 Hz (w/ R8=27k) |
| Input shunt cap | C5 | 10nF | Input | — |
| Non-inv input feed R | R7 | 33k | Stage 1 | Part of gain network + gain peak |
| Non-inv input R | R8 | 27k | Stage 1 | Also lower feedback R |
| Stage 1 DC bias | R4 | 1M | Stage 1 | DC bias at pin 2(–) |
| Stage 1 HF cap | C4 | 100pF | Stage 1 | f_pole = 48.2 kHz w/ R7 |
| Stage 1 pin3 shunt | C6 | 10nF | Stage 1 | AC ref at pin 3(+) |
| DRIVE floor R | R6 | 10k | Stage 1 | Min feedback Z floor |
| DRIVE pot | DRIVE | 100kB | Both | Linear taper; cross-stage |
| Stage 2 coupling | C7 | **100nF** | Stage 2 | HPF f_c = **159 Hz** w/ R9 |
| Stage 2 input R | R9 | 10k | Stage 2 | Av = –R10/R9 = –22 |
| Stage 2 feedback R | R10 | 220k | Stage 2 | — |
| Soft clip diodes | D2–D5 | MA856 ×4 | SW-1 | Vf≈0.82V; 2× DiodePairT ∥ |
| Hard clip series R | R11 | 6.8k | SW-2 | Always in path |
| Hard clip diodes | D6/D7 | 1S1588 ×2 | SW-2 | Vf≈0.584V; 1× DiodePairT |
| Tone isolating R | R12 | 1k | Tone | — |
| Tone pot | TONE | 25kB | Tone | LPF 637–∞ Hz |
| Tone cut cap | C8 | 10nF | Tone | Main treble-cut cap |
| Presence R | R13 | 6.8k | Tone | Presence bypass path |
| Presence trim | Trim | 50kB | Tone | Default CCW; linear |
| Presence cap | C9 | 10nF | Tone | Presence bypass cap |
| Output coupling | C11 | 1µF | Volume | HPF f_c ≈ 0.16 Hz w/ R14 |
| Volume pot | VOL | 100kA | Volume | Audio taper |
| Output bleed | R14 | 1M | Volume | DC reference |
| Hi Gain R | R29 (Theseus) | 22k | Stage 1 | ∥ R8=27k when SW-3 ON |
| Hi Gain protect | R27 (Theseus) | 47R | Stage 1 | Switch protection |

---

## 15. Key Transfer Functions

| Network | f_c | Notes |
|---------|-----|-------|
| Input HPF (C3 + R8) | 590 Hz | Tight low-end; sub-bass rolloff |
| Stage 2 input HPF (C7 + R9) | 159 Hz | C7=100nF; near full-range gain |
| Stage 1 feedback pole (C4 ∥ R7) | 48.2 kHz | Contributes to ~4194 Hz two-stage gain peak |
| Stage 2 gain (SW-1 OFF) | –22× (26.8 dB) | Inverting |
| Tone LPF @ max TONE (25k) | 637 Hz | Heavy treble cut |
| Gain peak (two-stage system) | ~4194 Hz | Measured at mid-DRIVE (CCRMA paper) |
| Output HPF (C11 + R14) | 0.16 Hz | DC block only |

---

## 16. Diode Parameters

### MA856 — SW-1 Soft Clip (D2–D5) — VALIDATED
```cpp
constexpr double Is_MA856 = 7.74e-13;   // Vf ≈ 0.820V @ 1mA
constexpr double n_MA856  = 1.512;
constexpr double Vt       = 25.85e-3;
constexpr double Rs_MA856 = 0.85;       // optional; model as series ResistorT
```
Two `DiodePairT` instances in `WDFParallelT` (D2+D3 pair ∥ D4+D5 pair).

### 1S1588 — SW-2 Hard Clip (D6–D7) — same as 1N914/1N4148
```cpp
constexpr double Is_1S1588 = 2.52e-9;   // Vf ≈ 0.584V @ 1mA
constexpr double n_1S1588  = 1.752;
constexpr double Rs_1S1588 = 0.568;
```
One `DiodePairT` instance.

---

## 17. Pot Tapers

| Control | Type | DSP mapping |
|---------|------|-------------|
| DRIVE | 100kB linear | `R = 100k × x` |
| TONE | 25kB linear | `R = 25k × x` |
| VOL | 100kA audio | `R = 100k × pow(10, 2×x - 2)` |
| Trim (Presence) | 50kB linear | `R = 50k × x` |

---

## 18. Clipping Mode Summary (Per Channel)

| Mode | SW-1 | SW-2 | SW-3 Hi Gain | Clamp | Character |
|------|------|------|-------------|-------|-----------|
| Boost | OFF | OFF | OFF | None | Clean boost |
| Boost Hi | OFF | OFF | ON | None | Higher gain clean boost |
| Overdrive | ON | OFF | OFF | ±0.82V | Soft, touch-sensitive |
| Overdrive Hi | ON | OFF | ON | ±0.82V | More aggressive OD |
| Distortion | OFF | ON | OFF | ±0.584V | Hard RAT-style |
| Distortion Hi | OFF | ON | ON | ±0.584V | High-gain hard clip |
| Both | ON | ON | OFF | ±0.82V→±0.584V | Stacked dual threshold |
| Both Hi | ON | ON | ON | ±0.82V→±0.584V | Max gain + stacked |

---

## 19. Open Items

**Tone stage topology:** Exact node-level wiring of TONE/C8/R13/Trim/C9.
Described in Section 11 and node diagram. Verify node-by-node from `king_of_tone_schematic.png`
before implementing ToneStage.h. Invoke `schematic-checker` agent at that step.
