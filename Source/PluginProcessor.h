#pragma once
#include <JuceHeader.h>
#include "DecayEnvelope.h"
#include "DFAFSequencer.h"
#include "DFAFVoice.h"

class DFAFProcessor : public juce::AudioProcessor
{
public:
    DFAFProcessor();
    ~DFAFProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "DFAF"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

private:
    DFAFSequencer sequencer;
        DFAFVoice     voice;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DFAFProcessor)
};
