# UI Rules

## General

- Custom `LookAndFeel` subclass — no default JUCE styling anywhere
- All drawing code in LookAndFeel overrides only — zero drawing in component or DSP logic
- No foleys_gui_magic or XML-driven UI builders
- Gin library (github.com/FigBug/Gin) may be used for supplementary components
- UI fully decoupled from DSP — visual design replaceable without touching DSP

## Layout — Two-Channel Pedal Face

Reference: `king_of_tone_pedal_picture.jpg` — white enclosure, gold compass rose, black knobs.
The King of Tone has two identical channels side by side, labelled A and B. Layout mirrors this directly.

```
┌──────────────────────────────────────────────────────────┐
│  [INPUT TRIM]                             [OUTPUT TRIM]  │  ← plugin-only, visually distinct
├──────────────────────────────────────────────────────────┤
│           CHANNEL A              CHANNEL B               │
│  [VOLUME A]  [DRIVE A]  [TONE A] [VOLUME B] [DRIVE B] [TONE B]  │  ← main knobs
│                                                          │
│  [CLIP A]  [HI GAIN A] [●LED A] [●LED B] [HI GAIN B] [CLIP B]  │  ← mode controls + LEDs
│  Boost/OD/Dist/Both                      Boost/OD/Dist/Both     │
│                                                          │
│  [PRESENCE A]  ◆ MONARCH OF TONE ◆  [PRESENCE B]        │  ← internal trim knobs + logo
│                                                          │
│    [BYPASS A ⬤]                   [BYPASS B ⬤]          │  ← footswitches, bottom
│  [OVERSAMPLING LIVE]  [OVERSAMPLING RENDER]              │  ← plugin-only controls
│  [IN  METER] ▐▐▐▐▐▐▐▐▐                                 │
│  [OUT METER] ▐▐▐▐▐▐▐▐▐                                 │
└──────────────────────────────────────────────────────────┘
```

## Controls

### Knobs (×6 main, ×2 presence)
- Three main knobs per channel: Volume, Drive, Tone
- Labels: "Volume", "Drive", "Tone" (same on both channels; distinguished by A/B column)
- Drive and Tone: linear taper — applied in DSP, not UI
- Volume: audio taper — applied in DSP, not UI
- Style: large black knurled knobs with white indicator line (reference KOT photo)
- JUCE `Slider` (rotary) + custom LookAndFeel paint
- Presence knobs: smaller, same style, labelled "Presence A" / "Presence B"
- Default Presence = fully CCW (matches hardware default = no treble boost)

### Hi Gain Toggle (×2, one per channel)
- Small toggle button or LED-style indicator per channel, labelled "Hi Gain"
- Two states: off (standard gain range) and on (hi gain range, +4 dB shift)
- Placed adjacent to the clipping mode selector for each channel
- Visual style: small illuminated toggle — lit when Hi Gain is active
- Maps to `hi_gain_a` / `hi_gain_b` APVTS parameters

### Clipping Mode Selector (×2, one per channel)
- 4-position selector per channel: **Boost / Overdrive / Distortion / Both**
- Segmented button or small ComboBox — clearly legible
- Default: "Overdrive" (matches hardware factory default: SW-1 ON, SW-2 OFF)
- Placed above or below the channel LEDs

### Bypass (×2 footswitches)
- Large footswitch-style toggle button per channel, at bottom of each channel column
- LED ON = channel active (processing); LED OFF = bypassed
- Both bypass states are independent

### LEDs (×2)
- Small circular indicator per channel, between knobs and bypass
- ON (red, matching KOT red LED aesthetic) = channel active
- OFF = channel bypassed
- State from `std::atomic<bool> bypassedA / bypassedB`

### Input Trim / Output Trim
- Visually distinct from the main knobs (different size, placement, colour tint)
- Range: -12 to +12 dB
- Placed at the top of the plugin window, outside the pedal face area

### Oversampling Controls
- Two controls, clearly grouped and labelled:
  - **"Oversampling (Live)"** — applied during real-time playback; default 4x
  - **"Oversampling (Render)"** — applied when DAW is bouncing/rendering; default 8x
- Each is a `AudioParameterChoice`: 1x / 2x / 4x / 8x
- ComboBox or segmented button for each — clearly readable
- Placed together at the bottom of the plugin window, outside the pedal face area
- A small info label or tooltip is helpful: e.g. "Render mode activates automatically when bouncing"

## Plugin Window

- Fixed size: **700 × 480 px** (wider than single-channel to accommodate two columns)
- Not resizable in v1
- Set via `setSize(700, 480)` in `PluginEditor` constructor

## Colour Scheme

Inspired by the KOT white/gold aesthetic. All colours as named constants in `MonarchLookAndFeel.h`.

```cpp
static constexpr juce::Colour colourBackground    { 0xFF1A1A1A }; // near-black body/frame
static constexpr juce::Colour colourPanelFace     { 0xFFF0EEE8 }; // off-white pedal face
static constexpr juce::Colour colourKnob          { 0xFF1A1A1A }; // black knob cap
static constexpr juce::Colour colourKnobIndicator { 0xFFFFFFFF }; // white indicator line
static constexpr juce::Colour colourAccent        { 0xFFCCA42A }; // gold (KOT compass rose)
static constexpr juce::Colour colourChannelDivider{ 0xFFCCA42A }; // gold divider between channels
static constexpr juce::Colour colourLEDActive     { 0xFFFF3300 }; // red LED on (KOT-style red)
static constexpr juce::Colour colourLEDInactive   { 0xFF3A1000 }; // dim red LED off
static constexpr juce::Colour colourLabelText     { 0xFF1A1A1A }; // dark text on white face
static constexpr juce::Colour colourMeterLow      { 0xFF44CC44 }; // meter green
static constexpr juce::Colour colourMeterMid      { 0xFFDDCC00 }; // meter yellow
static constexpr juce::Colour colourMeterHigh     { 0xFFDD2222 }; // meter red
static constexpr juce::Colour colourTrimControl   { 0xFF4488CC }; // blue tint for plugin-only controls
```

## VU Meters

- Bar-style, input and output
- Calibrated to -12 dBu nominal (0 VU = -12 dBu)
- VU ballistics: ~300ms release (attack fast)
- Input: post input-trim, pre DSP chain
- Output: post DSP chain (both channels), post output-trim
- Updated via `juce::Timer` on message thread reading `std::atomic<float>` levels
- Mono display acceptable for v1

## Threading in UI

- UI timer reads `std::atomic` meter values and bypass states — no direct DSP access
- Parameter changes go through APVTS — no direct DSP calls from UI
