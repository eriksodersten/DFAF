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
};
