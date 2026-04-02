#pragma once
#include <JuceHeader.h>
#include "DecayEnvelope.h"
#include "DFAFSequencer.h"
#include "DFAFVoice.h"
#include "MoogLadderFilter.h"

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

    void getStateInformation(juce::MemoryBlock& destData) override;
        void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
        static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    int getCurrentStep() const { return sequencer.getCurrentStep(); }
            void resetSequencer() { sequencer.reset(); lastStep = -1; }

    DFAFSequencer   sequencer;
    double currentSampleRate = 44100.0;
        juce::Random    noiseRandom;

    // Sequencer clock state
            int  lastStep           = -1;
            int  pendingStepIndex   = 0;
        DFAFVoice       voice;
        MoogLadderFilter filter;

    // TRIM LOGGER – ta bort efter mätning
        bool measuring       = false;
        bool wasActive       = false;
        bool measurementDone = false;
        std::vector<float> ampGains;
        double sumRaw2   = 0.0;
        double sumOldIn2 = 0.0;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DFAFProcessor)
};
