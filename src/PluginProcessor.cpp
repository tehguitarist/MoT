#include "PluginProcessor.h"
#include "PluginEditor.h"

MonarchAudioProcessor::MonarchAudioProcessor()
    : AudioProcessor (BusesProperties()
                           .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                           .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

MonarchAudioProcessor::~MonarchAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout MonarchAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    const juce::StringArray clippingModeChoices { "Boost", "Overdrive", "Distortion", "Both" };
    const juce::StringArray oversamplingChoices { "1x", "2x", "4x", "8x" };

    // The two series channels are identified externally by their LED colour: the first
    // channel is "Yellow", the second is "Red". The Theseus Hi-Gain mod is a FIXED part of
    // the Red channel's Stage 1 (not a runtime parameter) — see circuit.md Section 6.
    struct ChannelSpec
    {
        const char* id;
        const char* label;
    };

    for (const auto& channel : { ChannelSpec { "yellow", "Yellow" }, ChannelSpec { "red", "Red" } })
    {
        const juce::String id = channel.id;
        const juce::String label = channel.label;

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drive_" + id, 1 },
            "Drive " + label,
            juce::NormalisableRange<float> (0.0f, 1.0f),
            0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "tone_" + id, 1 },
            "Tone " + label,
            juce::NormalisableRange<float> (0.0f, 1.0f),
            0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "volume_" + id, 1 },
            "Volume " + label,
            juce::NormalisableRange<float> (0.0f, 1.0f),
            0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "presence_" + id, 1 },
            "Presence " + label,
            juce::NormalisableRange<float> (0.0f, 1.0f),
            0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "clipping_mode_" + id, 1 },
            "Clipping " + label,
            clippingModeChoices,
            1));

        params.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { "bypass_" + id, 1 },
            "Bypass " + label,
            false));
    }

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "input_trim", 1 },
        "Input Trim",
        juce::NormalisableRange<float> (-12.0f, 12.0f),
        0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "output_trim", 1 },
        "Output Trim",
        juce::NormalisableRange<float> (-12.0f, 12.0f),
        0.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "oversampling_realtime", 1 },
        "Oversampling (Live)",
        oversamplingChoices,
        2));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "oversampling_render", 1 },
        "Oversampling (Render)",
        oversamplingChoices,
        3));

    return { params.begin(), params.end() };
}

void MonarchAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    channelYellow.prepare (sampleRate, samplesPerBlock);
    channelRed.prepare (sampleRate, samplesPerBlock);
}

void MonarchAudioProcessor::releaseResources()
{
}

bool MonarchAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainIn = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    // Guitar-pedal layout: mono or stereo only, with the input matching the output
    // (no up-/down-mixing). This lets the plugin load on mono tracks as well as stereo.
    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    return mainIn == mainOut;
}

void MonarchAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* MonarchAudioProcessor::createEditor()
{
    return new MonarchAudioProcessorEditor (*this);
}

bool MonarchAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String MonarchAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MonarchAudioProcessor::acceptsMidi() const
{
    return false;
}

bool MonarchAudioProcessor::producesMidi() const
{
    return false;
}

double MonarchAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MonarchAudioProcessor::getNumPrograms()
{
    return 1;
}

int MonarchAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MonarchAudioProcessor::setCurrentProgram (int)
{
}

const juce::String MonarchAudioProcessor::getProgramName (int)
{
    return {};
}

void MonarchAudioProcessor::changeProgramName (int, const juce::String&)
{
}

void MonarchAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
    }
}

void MonarchAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MonarchAudioProcessor();
}
