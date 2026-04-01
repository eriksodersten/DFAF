#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class DFAFLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DFAFLookAndFeel()
    {
        setColour(juce::Slider::rotarySliderFillColourId,    juce::Colour(0xffcccccc));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff222222));
        setColour(juce::Slider::thumbColourId,               juce::Colour(0xffffffff));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider&) override
    {
        const float radius = juce::jmin((float)width / 2.0f, (float)height / 2.0f) - 4.0f;
        const float cx = (float)x + (float)width  * 0.5f;
        const float cy = (float)y + (float)height * 0.5f;

        g.setColour(juce::Colour(0xff111111));
        g.fillEllipse(cx - radius - 2, cy - radius - 2, (radius + 2) * 2, (radius + 2) * 2);

        juce::ColourGradient grad(
            juce::Colour(0xff383838), cx - radius * 0.3f, cy - radius * 0.3f,
            juce::Colour(0xff111111), cx + radius * 0.5f, cy + radius * 0.5f,
            true);
        g.setGradientFill(grad);
        g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

        const int numTags = 20;
        for (int i = 0; i < numTags; ++i)
        {
            float angle = (float)i / numTags * juce::MathConstants<float>::twoPi;
            float x0 = cx + (radius - 0.5f) * std::cos(angle);
            float y0 = cy + (radius - 0.5f) * std::sin(angle);
            float x1 = cx + (radius - 4.5f) * std::cos(angle);
            float y1 = cy + (radius - 4.5f) * std::sin(angle);
            g.setColour(juce::Colour(0xff555555));
            g.drawLine(x0, y0, x1, y1, 1.2f);
        }

        juce::ColourGradient innerGrad(
            juce::Colour(0xff484848), cx - radius * 0.2f, cy - radius * 0.2f,
            juce::Colour(0xff1a1a1a), cx + radius * 0.3f, cy + radius * 0.3f,
            true);
        g.setGradientFill(innerGrad);
        g.fillEllipse(cx - radius * 0.68f, cy - radius * 0.68f,
                      radius * 1.36f, radius * 1.36f);

        const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        const float lineR = radius * 0.52f;
        const float lx1   = cx + lineR * std::cos(angle - juce::MathConstants<float>::halfPi);
        const float ly1   = cy + lineR * std::sin(angle - juce::MathConstants<float>::halfPi);
        const float lx0   = cx + radius * 0.18f * std::cos(angle - juce::MathConstants<float>::halfPi);
        const float ly0   = cy + radius * 0.18f * std::sin(angle - juce::MathConstants<float>::halfPi);
        g.setColour(juce::Colour(0xffffffff));
        g.drawLine(lx0, ly0, lx1, ly1, 2.0f);

        juce::Path arcTrack;
        arcTrack.addCentredArc(cx, cy, radius + 3.5f, radius + 3.5f,
                               0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(0xff444444));
        g.strokePath(arcTrack, juce::PathStrokeType(1.5f));

        juce::Path arcFill;
        arcFill.addCentredArc(cx, cy, radius + 3.5f, radius + 3.5f,
                              0.0f, rotaryStartAngle, angle, true);
        g.setColour(juce::Colour(0xff888888));
        g.strokePath(arcFill, juce::PathStrokeType(1.5f));
    }
};

//==============================================================================
class DFAFEditor : public juce::AudioProcessorEditor
{
public:
    DFAFEditor(DFAFProcessor&);
    ~DFAFEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    DFAFProcessor& processor;
    DFAFLookAndFeel laf;

    // Row 1
        juce::Slider vcoDecay, vco1EgAmount, vco1Frequency;
    juce::ComboBox seqPitchModBox;
        juce::ComboBox hardSyncBox;
    juce::Slider vco1Level, noiseLevel, cutoff, resonance, vcaEg, volume;

    // Row 2
    juce::Slider fmAmount, vco2EgAmount, vco2Frequency;
    juce::Slider vco2Level, vcfDecay, vcfEgAmount, noiseVcfMod, vcaDecay;

    // Sequencer
    juce::Slider tempo;
    juce::Slider stepPitch[8];
    juce::Slider stepVelocity[8];

    void setupKnob(juce::Slider& slider, bool small = false);
    void drawSwitch(juce::Graphics& g, float x, float y, float w, float h,
                    const juce::String& label, const juce::StringArray& options) const;
    void drawButton(juce::Graphics& g, float x, float y, float r,
                    const juce::String& label, bool red = false) const;
    void drawJackPanel(juce::Graphics& g, int x, int y, int w, int h) const;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> vcoDecayAtt, vco1FreqAtt, vco1EgAmtAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> seqPitchModBoxAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> hardSyncBoxAtt;
        std::unique_ptr<SliderAttachment> vco1LevelAtt, vco2LevelAtt;
    std::unique_ptr<SliderAttachment> fmAmountAtt, vco2FreqAtt, vco2EgAmtAtt;
    std::unique_ptr<SliderAttachment> noiseLevelAtt, cutoffAtt, resonanceAtt;
    std::unique_ptr<SliderAttachment> vcfDecayAtt, vcfEgAmtAtt, noiseVcfModAtt;
    std::unique_ptr<SliderAttachment> vcaDecayAtt, vcaEgAtt, volumeAtt, tempoAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DFAFEditor)
};
