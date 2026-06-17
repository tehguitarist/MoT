#pragma once

namespace monarch
{

/** Stub for a single King of Tone channel's DSP chain. WDF implementation begins at Step 4.

    @param hiGain  Selects the Stage 1 voicing baked into this channel. The Theseus Hi-Gain
                   mod is a FIXED build option here, not a runtime toggle: the Yellow channel
                   is constructed with hiGain=false (stock), the Red channel with hiGain=true.
                   Stage 1 will use this to pick the corresponding fixed scattering matrix once
                   the Hi-Gain topology is implemented (circuit.md Section 6). */
class MonarchChannel
{
public:
    explicit MonarchChannel (bool hiGain = false) : hiGainStage1 (hiGain) {}

    void prepare (double /*sampleRate*/, int /*samplesPerBlock*/) {}

    void reset() {}

    float process (float input)
    {
        return input;
    }

private:
    bool hiGainStage1 { false };
};

} // namespace monarch
