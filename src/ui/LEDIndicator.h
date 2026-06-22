#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Assets.h"

/**
 * Small status LED rendered from the bezel art (glow baked into the image). Active = the
 * channel's coloured bezel (yellow or red); inactive = the dim/off bezel. Driven from the
 * editor timer by reading the bypass parameter (LED on = channel active). The images are
 * slightly larger than the original vector part since their glow is baked in — the component
 * is sized accordingly in PedalFace::resized().
 */
class LEDIndicator : public juce::Component
{
public:
    enum class Channel
    {
        Yellow,
        Red
    };

    LEDIndicator() = default;

    void setOn (bool shouldBeOn)
    {
        if (on != shouldBeOn)
        {
            on = shouldBeOn;
            repaint();
        }
    }

    /** Which channel this LED belongs to (selects the yellow/red lit bezel art). */
    void setChannel (Channel c)
    {
        channel = c;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const juce::Image img = on
            ? (channel == Channel::Yellow ? MonarchAssets::ledYellow() : MonarchAssets::ledRed())
            : MonarchAssets::ledOff();
        if (! img.isValid())
            return;

        const auto b = getLocalBounds().toFloat();
        const float d = juce::jmin (b.getWidth(), b.getHeight());
        const juce::Rectangle<float> dest (b.getCentreX() - d * 0.5f, b.getCentreY() - d * 0.5f, d, d);
        g.drawImage (img, dest, juce::RectanglePlacement::centred, false);
    }

private:
    bool on { true };
    Channel channel { Channel::Yellow };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LEDIndicator)
};
