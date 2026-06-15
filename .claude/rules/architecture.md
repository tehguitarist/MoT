# Architecture Rules

## Threading

- All DSP runs on the **audio thread**
- All UI runs on the **message thread**
- Cross-thread communication: `std::atomic` only — no locks, mutexes, or blocking calls on the audio thread
- Meter levels: `std::atomic<float>` written by audio thread, read by UI timer
- Bypass state per channel: `std::atomic<bool> bypassedA`, `std::atomic<bool> bypassedB`
- Clipping mode per channel: `std::atomic<int> pendingClippingModeA`, `std::atomic<int> pendingClippingModeB`
- Hi Gain mode per channel: `std::atomic<bool> pendingHiGainA`, `std::atomic<bool> pendingHiGainB`
- Oversampling: `std::atomic<int> pendingOversamplingFactor` (derived each block from `isNonRealtime()` + APVTS; single global setting applies to both channels)
- Parameter changes: JUCE `AudioProcessorValueTreeState` with smoothed parameter values

## Plugin Structure

```
MonarchAudioProcessor          ← AudioProcessor subclass
  AudioProcessorValueTreeState apvts
  InputTrimStage
  MonarchChannel channelA      ← full single-channel DSP
    InputFilter   (linear WDF)
    Stage1        (linear WDF, IC_A non-inverting ideal op-amp)
    Stage2        (linear WDF, IC_B inverting ideal op-amp + R-type at feedback node)
    SW1SoftClip   (nonlinear WDF — MA856×4 in feedback, 2 precomputed topologies)
    SW2HardClip   (nonlinear WDF — 1S1588×2 shunt, 2 precomputed topologies)
    ToneStage     (linear WDF passive)
    VolumePot     (linear)
    juce::dsp::Oversampling oversampler
  MonarchChannel channelB      ← identical, independent parameter set
    (same structure as channelA)
  OutputTrimStage
  std::atomic<float> inputLevelL, inputLevelR
  std::atomic<float> outputLevelL, outputLevelR
  std::atomic<bool>  bypassedA, bypassedB
  std::atomic<int>   pendingClippingModeA, pendingClippingModeB
  std::atomic<bool>  pendingHiGainA, pendingHiGainB
  std::atomic<int>   pendingOversamplingFactor
```

## Parameters (APVTS IDs)

Channel A and B have mirrored parameter sets:

| ID | Label | Range | Default | Notes |
|----|-------|-------|---------|-------|
| `drive_a` | Drive A | 0.0–1.0 | 0.5 | Linear taper (B-pot) applied in DSP |
| `tone_a` | Tone A | 0.0–1.0 | 0.5 | Linear taper (B-pot) applied in DSP |
| `volume_a` | Volume A | 0.0–1.0 | 0.5 | Audio taper (A-pot) applied in DSP |
| `presence_a` | Presence A | 0.0–1.0 | 0.0 | Linear taper; default fully CCW (no boost) |
| `clipping_mode_a` | Clipping A | 0/1/2/3 | 1 | `AudioParameterChoice`: "Boost"/"Overdrive"/"Distortion"/"Both" |
| `hi_gain_a` | Hi Gain A | true/false | false | `AudioParameterBool`; OFF = standard, ON = Hi Gain mod |
| `bypass_a` | Bypass A | true/false | false | `AudioParameterBool` |
| `drive_b` | Drive B | 0.0–1.0 | 0.5 | As above |
| `tone_b` | Tone B | 0.0–1.0 | 0.5 | As above |
| `volume_b` | Volume B | 0.0–1.0 | 0.5 | As above |
| `presence_b` | Presence B | 0.0–1.0 | 0.0 | As above |
| `clipping_mode_b` | Clipping B | 0/1/2/3 | 1 | As above |
| `hi_gain_b` | Hi Gain B | true/false | false | `AudioParameterBool` |
| `bypass_b` | Bypass B | true/false | false | `AudioParameterBool` |
| `input_trim` | Input Trim | -12.0 to +12.0 dB | 0.0 | Plugin-only; `AudioParameterFloat` |
| `output_trim` | Output Trim | -12.0 to +12.0 dB | 0.0 | Plugin-only; `AudioParameterFloat` |
| `oversampling_realtime` | Oversampling (Live) | 0/1/2/3 | 2 (4x) | `AudioParameterChoice`: "1x"/"2x"/"4x"/"8x" — active during live playback |
| `oversampling_render` | Oversampling (Render) | 0/1/2/3 | 3 (8x) | `AudioParameterChoice`: "1x"/"2x"/"4x"/"8x" — active when `isNonRealtime()` |

Default `clipping_mode` = 1 (Overdrive = SW-1 ON, SW-2 OFF). This is the factory default per Analog Man.
Default `presence` = 0.0 (fully CCW = no boost). This is the factory default per Analog Man.

Note: Use `std::make_unique<AudioParameterBool>(...)` for bool params; APVTS does not accept raw bool.

## Clipping Mode Mapping

| Value | Label | SW-1 | SW-2 |
|-------|-------|------|------|
| 0 | "Boost" | OFF | OFF |
| 1 | "Overdrive" | ON | OFF |
| 2 | "Distortion" | OFF | ON |
| 3 | "Both" | ON | ON |

## Channel Routing

```
guitar input → [input trim] → channelA.process() → channelB.process() → [output trim] → amp output
```

When a channel is bypassed, its `process()` returns the input unchanged (no DSP). Both bypass states are independent. When both are bypassed the signal passes through both switches dry, matching the hardware's true-bypass behaviour.

## Bypass

- True bypass per channel: input routed directly to output, zero DSP
- Crossfade on bypass transition (~5ms ramp) to prevent clicks
- `std::atomic<bool> bypassedA / bypassedB` — audio thread polls, applies ramp
- On DAW recall, bypass visual state reflects the restored APVTS parameter value

## Oversampling

- Single `juce::dsp::Oversampling` instance per channel (two total)
- Wraps only the nonlinear clipping stages (SW-1 and SW-2) per channel
- Changing factor: `pendingOversamplingFactor` atomic → audio thread reinits both oversamplers at start of next processBlock. Brief gap (one block) acceptable.
- Call `oversampler.initProcessing(samplesPerBlock)` in `prepareToPlay` for both channels

## prepareToPlay Responsibilities

All of the following in `prepareToPlay(sampleRate, samplesPerBlock)`:
- `.prepare(sampleRate)` on every `CapacitorT` in both channels
- `oversampler.initProcessing(samplesPerBlock)` for both channels
- Reset all smoothed parameter values to current APVTS values
- Reset both bypass crossfade states
- Initialise all precomputed scattering matrices for all clipping modes (both channels)

## processBlock Structure

```
1. Determine active oversampling factor:
   isNonRealtime() ? oversampling_render : oversampling_realtime (read from APVTS)
   If active factor != current → set pendingOversamplingFactor, reinit both oversamplers
2. Check pendingClippingModeA/B — update SW-1/SW-2 scattering matrices for changed channels
3. Check pendingHiGainA/B — update Stage 1 scattering matrix for changed channels
4. Read all APVTS parameter values (smoothed) for both channels
5. Apply taper conversion (audio taper to VOL A/B only; all others linear)
6. Update WDF node values in both channels
7. Apply input trim gain
8. Update input meter levels (std::atomic write)
9. Process channelA:
   - If bypassedA: copy input → output, skip DSP and oversampler
   - Else: upsample → WDF chain → downsample
10. Process channelB (input = channelA output):
    - If bypassedB: copy input → output, skip DSP and oversampler
    - Else: upsample → WDF chain → downsample
11. Apply output trim gain
12. Update output meter levels (std::atomic write)
```

## State Save/Restore

- Full state via `AudioProcessorValueTreeState::state` (JUCE XML serialise)
- All parameters for both channels saved/restored
- Bypass states per channel saved/restored via APVTS
- Oversampling setting saved/restored
