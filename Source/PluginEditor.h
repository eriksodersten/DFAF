#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class DFAFEditor : public juce::AudioProcessorEditor
{
public:
    DFAFEditor(DFAFProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    DFAFProcessor& processor;

    // Top row
    juce::Slider vcoDecay, seqPitchMod, vco1EgAmount, vco1Frequency;
    juce::Slider noiseLevel, cutoff, resonance, vcaEg, volume;

    // Bottom row
    juce::Slider fmAmount, vco2EgAmount, vco2Frequency;
    juce::Slider vcfDecay, vcfEgAmount, noiseVcfMod, vcaDecay;

    // Sequencer
    juce::Slider tempo;
    juce::Slider stepPitch[8];
    juce::Slider stepVelocity[8];

    // Labels
    juce::Label vcoDecayLabel, vco1FreqLabel, cutoffLabel, resonanceLabel;
    juce::Label volumeLabel, tempoLabel, fmAmountLabel;

    void setupKnob(juce::Slider& slider);
    void setupLabel(juce::Label& label, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DFAFEditor)
};
