#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class DFAFLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DFAFLookAndFeel();

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider&) override;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                              bool isMouseOverButton, bool isButtonDown) override;

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool isMouseOverButton, bool isButtonDown) override;

    void drawComboBox(juce::Graphics& g, int width, int height, bool,
                      int, int, int, int, juce::ComboBox&) override;

    juce::Font getComboBoxFont(juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override;

private:
    static bool isPanelControl(const juce::Component& component);
};

class DFAFEditor : public juce::AudioProcessorEditor,
                   private juce::Timer
{
public:
    DFAFEditor(DFAFProcessor&);
    ~DFAFEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    DFAFProcessor& processor;
    DFAFLookAndFeel laf;
    juce::Image panelImage;

    juce::ComboBox presetBox;
    juce::TextButton presetPrevButton { "PREV" };
    juce::TextButton presetNextButton { "NEXT" };
    juce::TextButton presetSaveButton { "SAVE" };
    juce::TextButton presetDeleteButton { "DELETE" };
    juce::TextButton presetInitButton { "INIT" };
    std::unique_ptr<juce::FileChooser> presetSaveChooser;
    std::unique_ptr<juce::AlertWindow> presetSaveChoiceWindow;
    std::unique_ptr<juce::Component> presetSaveChoiceArt;
    juce::String lastPresetName;
    bool isUpdatingPresetBox = false;

    juce::Slider vcoDecay, vco1EgAmount, vco1Frequency;
    juce::ComboBox seqPitchModBox;
    juce::TextButton seqPitchModButton { "SEQ PITCH MOD" };
    juce::ComboBox hardSyncBox;
    juce::TextButton hardSyncButton { "HARD SYNC" };
    juce::Slider vco1Level, noiseLevel, cutoff, resonance, vcaEg, volume;

    juce::Slider fmAmount, vco2EgAmount, vco2Frequency;
    juce::Slider vco2Level, vcfDecay, vcfEgAmount, noiseVcfMod, vcaDecay;

    juce::Slider clockMult;
    juce::Slider stepPitch[8];
    juce::Slider stepVelocity[8];
    juce::ComboBox vco1WaveBox, vco2WaveBox, vcfModeBox;
    juce::TextButton vco1WaveButton { "VCO 1 WAVE" };
    juce::TextButton vco2WaveButton { "VCO 2 WAVE" };
    juce::TextButton vcfModeButton { "VCF MODE" };

    int currentLedStep = -1;
    bool resetLedActive = false;
    juce::TextButton resetButton { "RESET" };

    void timerCallback() override;
    void setupKnob(juce::Slider& slider, bool small = false);
    void refreshPresetControls();
    void promptSavePreset();
    void showSavePresetChooser(const juce::String& suggestedName);
    void stepPreset(int direction);
    void updatePresetButtonState();
    juce::Rectangle<float> getPanelImageBounds() const;
    juce::Rectangle<int> mapPanelRect(float x, float y, float w, float h) const;
    juce::Point<int> mapPanelPoint(float x, float y) const;
    void drawPresetDisplay(juce::Graphics& g) const;
    void drawPanelSwitches(juce::Graphics& g) const;

    juce::Rectangle<int> getPatchAreaBounds() const;

    juce::Point<int> getJackCentre(PatchPoint pp) const;
    PatchPoint jackHitTest(juce::Point<int> pos) const;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void drawJackPanel(juce::Graphics& g, juce::Rectangle<int> area,
                       const std::vector<PatchCable>& cables,
                       PatchPoint selectedOut) const;

    PatchPoint pendingOut = PP_NUM_POINTS;
    PatchPoint dragPatchPoint = PP_NUM_POINTS;
    PatchPoint dragHoverPoint = PP_NUM_POINTS;
    juce::Point<int> dragPatchPos;
    bool isDraggingPatchCable = false;
    std::vector<PatchCable> lastCableSnapshot;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> vcoDecayAtt, vco1FreqAtt, vco1EgAmtAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> seqPitchModBoxAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> hardSyncBoxAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> vco1WaveBoxAtt, vco2WaveBoxAtt, vcfModeBoxAtt;
    std::unique_ptr<SliderAttachment> vco1LevelAtt, vco2LevelAtt;
    std::unique_ptr<SliderAttachment> fmAmountAtt, vco2FreqAtt, vco2EgAmtAtt;
    std::unique_ptr<SliderAttachment> noiseLevelAtt, cutoffAtt, resonanceAtt;
    std::unique_ptr<SliderAttachment> vcfDecayAtt, vcfEgAmtAtt, noiseVcfModAtt;
    std::unique_ptr<SliderAttachment> vcaDecayAtt, vcaEgAtt, volumeAtt;
    std::unique_ptr<SliderAttachment> clockMultAtt;
    std::unique_ptr<SliderAttachment> stepPitchAtt[8], stepVelAtt[8];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DFAFEditor)
};
