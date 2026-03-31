#include "PluginEditor.h"

DFAFEditor::DFAFEditor(DFAFProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(600, 300);
}

void DFAFEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::white);
    g.setFont(20.0f);
    g.drawText("DFAF", getLocalBounds(), juce::Justification::centred);
}

void DFAFEditor::resized() {}
