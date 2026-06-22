#pragma once

#include <cmath>

#include <juce_audio_processors/juce_audio_processors.h>

#include "Assets.h"
#include "MonarchLookAndFeel.h"

/**
 * 3-position clipping switch (Boost / Overdrive / Distortion) bound to a `clipping_mode_*`
 * AudioParameterChoice. A vertical toggle (top = Boost, middle = Overdrive, bottom = Distortion)
 * with gold mode labels beside it; click a third to select. Matches the pedal-face look.
 */
class ClipSwitch : public juce::Component
{
public:
    explicit ClipSwitch (juce::RangedAudioParameter& param)
        : attachment (param, [this] (float v) {
              index = juce::jlimit (0, 2, (int) std::lround (v));
              repaint();
          })
    {
        attachment.sendInitialUpdate();
    }

    void setLabelScale (float sc)
    {
        labelFontPx = juce::jmax (6.0f, 7.0f * sc);
        repaint();
    }

    /** Mirror = switch on the right, labels on the left (so labels point inward on the right channel). */
    void setMirrored (bool m)
    {
        mirrored = m;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        const float W = b.getWidth(), H = b.getHeight();

        // BOOST sits above the switch, DIST below, OD beside it (same side as before — nearest
        // the switch). `gap` is shared between the BOOST/DIST vertical offset and the OD
        // horizontal offset, so all three labels sit the same distance from the switch body.
        const float labelH = juce::jmax (8.0f, labelFontPx * 1.3f);
        const float gap = juce::jmin (W, H) * 0.08f;
        const float switchD = juce::jmax (10.0f, H - 2.0f * (labelH + gap));
        const float switchX = mirrored ? (W - switchD) : 0.0f;
        const float switchY = labelH + gap;
        const auto sw = juce::Rectangle<float> (switchX, switchY, switchD, switchD);

        // Switch art — one image per position (index 0=Boost/up, 1=OD/mid, 2=Dist/down).
        const juce::Image img = index == 0 ? MonarchAssets::switchUpGraded()
                               : index == 1 ? MonarchAssets::switchMid()
                                            : MonarchAssets::switchDown();
        if (img.isValid())
            g.drawImage (img, sw, juce::RectanglePlacement::centred, false);

        g.setFont (juce::Font (juce::FontOptions (labelFontPx, juce::Font::bold)));
        auto colourFor = [this] (int i) {
            return i == index ? juce::Colour (MonarchLookAndFeel::cPedalGoldBright)
                               : juce::Colour (MonarchLookAndFeel::cPedalGold).withAlpha (0.4f);
        };

        g.setColour (colourFor (0));
        g.drawText ("BOOST", juce::Rectangle<float> (switchX, 0.0f, switchD, labelH), juce::Justification::centred);

        g.setColour (colourFor (2));
        g.drawText ("DIST", juce::Rectangle<float> (switchX, switchY + switchD + gap, switchD, labelH),
                    juce::Justification::centred);

        g.setColour (colourFor (1));
        const float odX = mirrored ? 0.0f : (switchD + gap);
        const float odW = mirrored ? (switchX - gap) : (W - switchD - gap);
        const auto odJustify = mirrored ? juce::Justification::centredRight : juce::Justification::centredLeft;
        g.drawText ("OD", juce::Rectangle<float> (odX, switchY + switchD * 0.5f - labelH * 0.5f, odW, labelH),
                    odJustify);
    }

    // Click to select a position, or drag the thumb between positions.
    void mouseDown (const juce::MouseEvent& e) override
    {
        attachment.beginGesture();
        setFromY (e.position.y);
    }
    void mouseDrag (const juce::MouseEvent& e) override { setFromY (e.position.y); }
    void mouseUp (const juce::MouseEvent&) override { attachment.endGesture(); }

private:
    void setFromY (float y)
    {
        const int idx = juce::jlimit (0, 2, (int) (y / (float) juce::jmax (1, getHeight()) * 3.0f));
        if (idx != index)
            attachment.setValueAsPartOfGesture ((float) idx);
    }

    juce::ParameterAttachment attachment;
    int index { 1 };
    float labelFontPx { 7.0f };
    bool mirrored { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipSwitch)
};
