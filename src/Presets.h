#pragma once

#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

namespace monarch
{

/**
 * A factory preset: a named snapshot of parameter values. Only the parameters a preset
 * actually wants to set are listed — anything omitted (bypass, trims, supply voltage,
 * oversampling) is left at whatever the user currently has.
 *
 * Values are already normalised [0, 1] (drive/tone/volume/presence have a native 0..1 range,
 * so normalised == actual; clipping_mode is a 3-way AudioParameterChoice, normalised = index/2).
 */
struct FactoryPreset
{
    juce::String name;
    std::vector<std::pair<juce::String, float>> values;
};

namespace detail
{
// Clipping-mode choice indices (see PluginProcessor::createParameterLayout): 0=Boost, 1=Overdrive,
// 2=Distortion. Stored here normalised (index / 2) since AudioParameterChoice::setValueNotifyingHost
// takes a normalised value.
constexpr float kClipBoost = 0.0f / 2.0f;
constexpr float kClipOverdrive = 1.0f / 2.0f;
constexpr float kClipDistortion = 2.0f / 2.0f;

// Knobs in these presets are specified on the traditional 0-10 pedal-dial scale; convert to the
// plugin's normalised 0..1 parameter range.
constexpr float dial (float zeroToTen) { return zeroToTen / 10.0f; }
} // namespace detail

/**
 * The 5 factory presets. Channel A = Red (first in the real pedal's signal flow), Channel B =
 * Yellow (second) — see circuit.md / architecture.md for the Red-first correction. Drive, Tone,
 * and Presence are shared across both channels in every preset below; Volume and the clipping
 * mode are set per channel.
 */
inline const std::vector<FactoryPreset>& getFactoryPresets()
{
    using namespace detail;

    static const std::vector<FactoryPreset> presets = {
        { "Blues",
          { { "volume_red", dial (6) }, { "volume_yellow", dial (6) },
            { "drive_red", dial (4) }, { "drive_yellow", dial (4) },
            { "tone_red", dial (5) }, { "tone_yellow", dial (5) },
            { "presence_red", dial (0) }, { "presence_yellow", dial (0) },
            { "clipping_mode_red", kClipOverdrive }, { "clipping_mode_yellow", kClipBoost } } },

        { "High Gain",
          { { "volume_red", dial (8) }, { "volume_yellow", dial (9) },
            { "drive_red", dial (8) }, { "drive_yellow", dial (8) },
            { "tone_red", dial (6) }, { "tone_yellow", dial (6) },
            { "presence_red", dial (0) }, { "presence_yellow", dial (0) },
            { "clipping_mode_red", kClipDistortion }, { "clipping_mode_yellow", kClipOverdrive } } },

        { "Rhythm Crunch",
          { { "volume_red", dial (7) }, { "volume_yellow", dial (7) },
            { "drive_red", dial (5) }, { "drive_yellow", dial (5) },
            { "tone_red", dial (7) }, { "tone_yellow", dial (7) },
            { "presence_red", dial (0) }, { "presence_yellow", dial (0) },
            { "clipping_mode_red", kClipBoost }, { "clipping_mode_yellow", kClipOverdrive } } },

        { "Edge-of-Breakup",
          { { "volume_red", dial (5) }, { "volume_yellow", dial (6) },
            { "drive_red", dial (3) }, { "drive_yellow", dial (3) },
            { "tone_red", dial (4) }, { "tone_yellow", dial (4) },
            { "presence_red", dial (0) }, { "presence_yellow", dial (0) },
            { "clipping_mode_red", kClipOverdrive }, { "clipping_mode_yellow", kClipBoost } } },

        { "Fuzz",
          { { "volume_red", dial (9) }, { "volume_yellow", dial (9) },
            { "drive_red", dial (9) }, { "drive_yellow", dial (9) },
            { "tone_red", dial (2) }, { "tone_yellow", dial (2) },
            { "presence_red", dial (2) }, { "presence_yellow", dial (2) },
            { "clipping_mode_red", kClipDistortion }, { "clipping_mode_yellow", kClipDistortion } } },
    };
    return presets;
}

/** Applies every value in `preset` to the matching APVTS parameter, notifying the host. */
inline void applyFactoryPreset (juce::AudioProcessorValueTreeState& apvts, const FactoryPreset& preset)
{
    for (const auto& [paramId, normalisedValue] : preset.values)
        if (auto* param = apvts.getParameter (paramId))
            param->setValueNotifyingHost (normalisedValue);
}

} // namespace monarch
