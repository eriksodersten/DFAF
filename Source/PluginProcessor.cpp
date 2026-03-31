#include "PluginProcessor.h"
#include "PluginEditor.h"

DFAFProcessor::DFAFProcessor()
    : AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{}

DFAFProcessor::~DFAFProcessor() {}

void DFAFProcessor::prepareToPlay(double sampleRate, int)
{
    sequencer.prepare(sampleRate);
        sequencer.setTempo(120.0f);
        voice.prepare(sampleRate);
        filter.prepare(sampleRate);
        filter.setCutoff(800.0f);
        filter.setResonance(0.4f);
}
void DFAFProcessor::releaseResources() {}

void DFAFProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    buffer.clear();
    auto* left  = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        int stepIndex = 0;
        if (sequencer.tick(stepIndex))
        {
            const auto& step = sequencer.getStep(stepIndex);
            voice.trigger(step.pitch, step.velocity);
        }
        float sample = filter.process(voice.process());
                left[i]  = sample;
                right[i] = sample;
    }
}

juce::AudioProcessorEditor* DFAFProcessor::createEditor()
{
    return new DFAFEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DFAFProcessor();
}
