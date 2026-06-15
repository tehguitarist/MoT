# Monarch of Tone

A dual-channel overdrive plugin (AU/VST3) inspired by classic boutique
Bluesbreaker-derived pedal designs. Built with JUCE and modelled using
Wave Digital Filters (WDF) for an analog-feeling response.

## Overview

Monarch of Tone emulates two independent overdrive channels running in
series, each with its own Drive, Tone, Volume, and Presence controls,
multiple clipping modes, and a Hi Gain option — all in the spirit of the
two-in-one boutique pedals that inspired this project.

- **Dual channels (A → B)**, each independently bypassable
- **Eight clipping modes per channel**: Boost, Overdrive, Distortion, and
  Both, each available in standard or Hi Gain variants
- **Circuit-inspired DSP** using chowdsp_wdf wave digital filter modelling
- **Oversampling** (1x/2x/4x/8x) with separate live and render settings
- **Input/Output trim and VU metering** for studio-friendly gain staging

## Status

This project is under active development. See `CLAUDE.md` and
`.claude/rules/` for the current build plan and implementation notes.

## Built With

- [JUCE](https://juce.com) — audio plugin framework
- [chowdsp_wdf](https://github.com/Chowdhury-DSP/chowdsp_wdf) — wave digital
  filter library used for the circuit-inspired DSP modelling

## Building

```bash
# Fetch dependencies
git submodule add https://github.com/juce-framework/JUCE libs/JUCE
git submodule add https://github.com/Chowdhury-DSP/chowdsp_wdf libs/chowdsp_wdf

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target Monarch_AU
cmake --build build --target Monarch_VST3
```

Requires CMake 3.15+, C++17, and macOS 10.13+.

## License

This project is licensed under the [GNU Affero General Public License v3.0](LICENSE)
(AGPLv3), the same license under which the open-source edition of JUCE is
distributed. chowdsp_wdf (BSD-3-Clause) is compatible and included under its
own license terms in `libs/chowdsp_wdf`.

## Author

Leigh Pierce
</content>
