#include "PluginProcessor.h"
#include "PluginEditor.h"

DFAFProcessor::DFAFProcessor()
    : AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{}

DFAFProcessor::~DFAFProcessor() {}

void DFAFProcessor::prepareToPlay(double sampleRate, int)
{
    envelope.prepare(sampleRate);
    envelope.setDecayTime(0.5f);
}
void DFAFProcessor::releaseResources() {}

void DFAFProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    buffer.clear();
}

juce::AudioProcessorEditor* DFAFProcessor::createEditor()
{
    return new DFAFEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DFAFProcessor();
}
