#include "PluginEditor.h"

#include <array>
#include <cmath>

namespace
{
constexpr int kEditorWidth = 1440;
constexpr int kEditorHeight = 760;
constexpr int kOuterMargin = 16;
constexpr int kUtilityHeight = 74;
constexpr int kPatchWidth = 214;
constexpr int kWoodWidth = 28;

constexpr int kPresetInitId = 1;
constexpr int kPresetFirstUserId = 2;
constexpr int kPresetMissingId = 999;

const juce::Colour kCaseOuter        = juce::Colour(0xff111111);
const juce::Colour kCaseInner        = juce::Colour(0xff2a2a2a);
const juce::Colour kPanelCream       = juce::Colour(0xffede5d1);
const juce::Colour kPanelWarm        = juce::Colour(0xffd8c8a2);
const juce::Colour kPanelShadow      = juce::Colour(0x26000000);
const juce::Colour kInk              = juce::Colour(0xff1e1e1e);
const juce::Colour kDivider          = juce::Colour(0x664b4538);
const juce::Colour kDividerSoft      = juce::Colour(0x334b4538);
const juce::Colour kAccentRed        = juce::Colour(0xffa3382d);
const juce::Colour kLedRed           = juce::Colour(0xffff6558);
const juce::Colour kLedGreen         = juce::Colour(0xffb7ef67);
const juce::Colour kButtonFace       = juce::Colour(0xff272727);
const juce::Colour kButtonFaceBright = juce::Colour(0xff3b3b3b);
const juce::Colour kButtonBorder     = juce::Colour(0xff111111);
const juce::Colour kJackBlue         = juce::Colour(0xff68a7ff);
const juce::Colour kJackGreen        = juce::Colour(0xff7ce381);
const juce::Colour kWoodLight        = juce::Colour(0xff8a6347);
const juce::Colour kWoodDark         = juce::Colour(0xff493222);
const juce::Colour kCableWarm        = juce::Colour(0xff9d564d);
const juce::Colour kCableDark        = juce::Colour(0xff383838);

juce::File getDefaultPresetDirectory()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("DFAF")
        .getChildFile("Presets");
}

bool cableSnapshotsEqual(const std::vector<PatchCable>& a, const std::vector<PatchCable>& b)
{
    if (a.size() != b.size())
        return false;

    for (size_t i = 0; i < a.size(); ++i)
    {
        if (a[i].src != b[i].src) return false;
        if (a[i].dst != b[i].dst) return false;
        if (std::abs(a[i].amount - b[i].amount) > 1.0e-6f) return false;
        if (a[i].enabled != b[i].enabled) return false;
    }

    return true;
}

juce::Font makeFont(float height, bool bold = false)
{
    auto options = juce::FontOptions(height);
    if (bold)
        options = options.withStyle("Bold");
    return juce::Font(options);
}

void drawLabel(juce::Graphics& g, juce::Rectangle<float> area, const juce::String& text,
               float fontHeight, juce::Justification justification = juce::Justification::centred)
{
    g.setColour(kInk);
    g.setFont(makeFont(fontHeight, false));
    g.drawFittedText(text, area.toNearestInt(), justification, 1);
}

void drawSectionHeader(juce::Graphics& g, juce::Rectangle<float> area, const juce::String& text)
{
    g.setColour(kInk);
    g.setFont(makeFont(24.0f, false));
    g.drawFittedText(text, area.toNearestInt(), juce::Justification::centred, 1);
}

void drawScrew(juce::Graphics& g, juce::Point<float> centre)
{
    auto outer = juce::Rectangle<float>(22.0f, 22.0f).withCentre(centre);
    juce::ColourGradient grad(juce::Colour(0xff7c7c7c), outer.getX(), outer.getY(),
                              juce::Colour(0xff2c2c2c), outer.getRight(), outer.getBottom(), true);
    g.setGradientFill(grad);
    g.fillEllipse(outer);

    g.setColour(juce::Colour(0x70000000));
    g.drawEllipse(outer, 1.2f);

    g.setColour(juce::Colour(0xff1a1a1a));
    g.drawLine(centre.x - 4.5f, centre.y - 4.0f, centre.x + 4.5f, centre.y + 4.0f, 1.2f);
    g.drawLine(centre.x - 4.0f, centre.y + 4.5f, centre.x + 4.0f, centre.y - 4.5f, 1.2f);
}

void drawPassiveButton(juce::Graphics& g, juce::Rectangle<float> area, const juce::String& label,
                       juce::Colour ledColour = juce::Colours::transparentBlack, bool ledActive = false)
{
    juce::ColourGradient fill(kButtonFaceBright, area.getX(), area.getY(),
                              kButtonFace, area.getRight(), area.getBottom(), true);
    g.setGradientFill(fill);
    g.fillRoundedRectangle(area, 4.0f);

    g.setColour(kButtonBorder);
    g.drawRoundedRectangle(area, 4.0f, 1.2f);

    auto led = area.withSizeKeepingCentre(10.0f, 10.0f).translated(0.0f, 1.0f);
    if (ledActive)
    {
        g.setColour(ledColour.withAlpha(0.22f));
        g.fillEllipse(led.expanded(8.0f));
        g.setColour(ledColour);
        g.fillEllipse(led);
    }
    else
    {
        g.setColour(juce::Colour(0xff343434));
        g.fillEllipse(led);
    }

    if (label.isNotEmpty())
    {
        g.setColour(kInk);
        g.setFont(makeFont(area.getWidth() < 54.0f ? 11.5f : 16.0f, false));
        g.drawFittedText(label, area.translated(0.0f, area.getHeight() + 6.0f).toNearestInt(),
                         juce::Justification::centred, 2);
    }
}

std::array<PatchPoint, 7> inputOrder()
{
    return { PP_VCA_CV, PP_VCA_DECAY, PP_VCF_MOD, PP_VCF_DECAY, PP_NOISE_LVL, PP_VCO_DECAY, PP_FM_AMT };
}

std::array<PatchPoint, 7> outputOrder()
{
    return { PP_VCA_EG, PP_VCF_EG, PP_VCO_EG, PP_VCO1, PP_VCO2, PP_VELOCITY, PP_PITCH };
}
}

DFAFLookAndFeel::DFAFLookAndFeel()
{
    setColour(juce::ComboBox::backgroundColourId, kButtonFace);
    setColour(juce::ComboBox::outlineColourId, kButtonBorder);
    setColour(juce::ComboBox::textColourId, juce::Colour(0xfff3ede1));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff1d1d1d));
    setColour(juce::PopupMenu::textColourId, juce::Colour(0xfff3ede1));
    setColour(juce::TextButton::buttonColourId, kButtonFace);
    setColour(juce::TextButton::textColourOffId, juce::Colour(0xfff3ede1));
    setColour(juce::TextButton::textColourOnId, juce::Colour(0xfff3ede1));
}

void DFAFLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                       juce::Slider&)
{
    const auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height).reduced(4.0f);
    const auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();

    g.setColour(juce::Colour(0x25000000));
    g.fillEllipse(bounds.translated(0.0f, 10.0f));

    auto body = bounds.reduced(radius * 0.13f);
    const auto bodyRadius = body.getWidth() * 0.5f;

    g.setColour(juce::Colour(0xff141414));
    g.fillEllipse(body.expanded(2.0f));

    juce::ColourGradient outerGrad(juce::Colour(0xff4f4f4f), body.getX(), body.getY(),
                                   juce::Colour(0xff1c1c1c), body.getRight(), body.getBottom(), true);
    g.setGradientFill(outerGrad);
    g.fillEllipse(body);

    for (int i = 0; i < 16; ++i)
    {
        const float angle = juce::MathConstants<float>::twoPi * (float) i / 16.0f;
        const float x0 = centre.x + std::cos(angle) * (bodyRadius + 2.0f);
        const float y0 = centre.y + std::sin(angle) * (bodyRadius + 2.0f);
        const float x1 = centre.x + std::cos(angle) * (bodyRadius + 8.0f);
        const float y1 = centre.y + std::sin(angle) * (bodyRadius + 8.0f);
        g.setColour(juce::Colour(0x9a222222));
        g.drawLine(x0, y0, x1, y1, 1.5f);
    }

    auto cap = body.reduced(body.getWidth() * 0.17f);
    juce::ColourGradient capGrad(juce::Colour(0xff4a4a4a), cap.getX(), cap.getY(),
                                 juce::Colour(0xff222222), cap.getRight(), cap.getBottom(), true);
    g.setGradientFill(capGrad);
    g.fillEllipse(cap);

    g.setColour(juce::Colour(0x55ffffff));
    g.drawEllipse(cap, 1.0f);

    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const float indicatorAngle = angle - juce::MathConstants<float>::halfPi;
    const float innerR = cap.getWidth() * 0.14f;
    const float outerR = cap.getWidth() * 0.40f;
    const juce::Point<float> p0(centre.x + std::cos(indicatorAngle) * innerR,
                                centre.y + std::sin(indicatorAngle) * innerR);
    const juce::Point<float> p1(centre.x + std::cos(indicatorAngle) * outerR,
                                centre.y + std::sin(indicatorAngle) * outerR);

    g.setColour(juce::Colour(0xffefefef));
    g.drawLine(p0.x, p0.y, p1.x, p1.y, 3.0f);
}

void DFAFLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                           bool isMouseOverButton, bool isButtonDown)
{
    auto area = button.getLocalBounds().toFloat().reduced(0.5f);
    auto top = isButtonDown ? juce::Colour(0xff2b2b2b) : kButtonFaceBright;
    auto bottom = isButtonDown ? juce::Colour(0xff161616) : kButtonFace;
    if (isMouseOverButton)
        top = top.brighter(0.06f);

    juce::ColourGradient fill(top, area.getX(), area.getY(),
                              bottom, area.getRight(), area.getBottom(), true);
    g.setGradientFill(fill);
    g.fillRoundedRectangle(area, 4.0f);
    g.setColour(kButtonBorder);
    g.drawRoundedRectangle(area, 4.0f, 1.2f);
}

void DFAFLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                     bool, bool)
{
    g.setColour(button.findColour(juce::TextButton::textColourOffId));
    g.setFont(makeFont(18.0f, true));
    g.drawFittedText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, 1);
}

void DFAFLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool,
                                   int, int, int, int, juce::ComboBox&)
{
    auto area = juce::Rectangle<float>(0.0f, 0.0f, (float) width, (float) height).reduced(0.5f);
    juce::ColourGradient fill(kButtonFaceBright, area.getX(), area.getY(),
                              kButtonFace, area.getRight(), area.getBottom(), true);
    g.setGradientFill(fill);
    g.fillRoundedRectangle(area, 4.0f);

    g.setColour(kButtonBorder);
    g.drawRoundedRectangle(area, 4.0f, 1.2f);

    auto arrow = area.removeFromRight(18.0f).reduced(4.0f, 8.0f);
    juce::Path p;
    p.startNewSubPath(arrow.getX(), arrow.getY());
    p.lineTo(arrow.getCentreX(), arrow.getBottom());
    p.lineTo(arrow.getRight(), arrow.getY());
    g.setColour(juce::Colour(0xffefe6d9).withAlpha(0.8f));
    g.strokePath(p, juce::PathStrokeType(1.6f));
}

juce::Font DFAFLookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return makeFont(16.5f, true);
}

void DFAFLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    label.setFont(getComboBoxFont(box));
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xfff3ede1));
    label.setBounds(2, 1, box.getWidth() - 22, box.getHeight() - 2);
}

DFAFEditor::DFAFEditor(DFAFProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(kEditorWidth, kEditorHeight);
    setLookAndFeel(&laf);
    startTimerHz(30);

    presetBox.setJustificationType(juce::Justification::centred);
    presetBox.setColour(juce::ComboBox::textColourId, kLedRed);
    presetBox.onChange = [this]()
    {
        if (isUpdatingPresetBox)
            return;

        const int selectedId = presetBox.getSelectedId();
        if (selectedId == kPresetMissingId)
            return;

        if (selectedId == kPresetInitId)
            processor.loadInitPreset();
        else
            processor.loadPreset(presetBox.getText());

        refreshPresetControls();
    };
    addAndMakeVisible(presetBox);

    presetSaveButton.onClick = [this]() { promptSavePreset(); };
    presetDeleteButton.onClick = [this]()
    {
        const auto presetName = processor.getCurrentPresetName();
        if (presetName.isEmpty() || presetName == "Init")
            return;

        if (juce::AlertWindow::showOkCancelBox(juce::AlertWindow::WarningIcon,
                                               "Delete Preset",
                                               "Delete preset \"" + presetName + "\"?",
                                               "Delete",
                                               "Cancel",
                                               this,
                                               nullptr))
        {
            processor.deletePreset(presetName);
            refreshPresetControls();
        }
    };
    presetInitButton.onClick = [this]()
    {
        processor.loadInitPreset();
        refreshPresetControls();
    };

    addAndMakeVisible(presetSaveButton);
    addAndMakeVisible(presetDeleteButton);
    addAndMakeVisible(presetInitButton);

    resetButton.onClick = [this]()
    {
        processor.resetSequencer();
        currentLedStep = 0;
        resetLedActive = true;
        repaint();
    };
    addAndMakeVisible(resetButton);

    auto add = [this](juce::Slider& s, bool small = false)
    {
        setupKnob(s, small);
        addAndMakeVisible(s);
    };

    add(vcoDecay);
    add(vco1EgAmount);
    add(vco1Frequency);
    add(vco1Level, true);
    add(noiseLevel, true);
    add(cutoff);
    add(resonance);
    add(vcaEg);
    add(volume);
    add(fmAmount);
    add(vco2EgAmount);
    add(vco2Frequency);
    add(vco2Level, true);
    add(vcfDecay);
    add(vcfEgAmount);
    add(noiseVcfMod);
    add(vcaDecay);

    seqPitchModBox.addItem("VCO 1&2", 1);
    seqPitchModBox.addItem("OFF", 2);
    seqPitchModBox.addItem("VCO 2", 3);
    addAndMakeVisible(seqPitchModBox);

    hardSyncBox.addItem("OFF", 1);
    hardSyncBox.addItem("ON", 2);
    addAndMakeVisible(hardSyncBox);

    vco1WaveBox.addItem("SQUARE", 1);
    vco1WaveBox.addItem("TRIANGLE", 2);
    addAndMakeVisible(vco1WaveBox);

    vco2WaveBox.addItem("SQUARE", 1);
    vco2WaveBox.addItem("TRIANGLE", 2);
    addAndMakeVisible(vco2WaveBox);

    vcfModeBox.addItem("LP", 1);
    vcfModeBox.addItem("HP", 2);
    addAndMakeVisible(vcfModeBox);

    clockMultBox.addItem("1/8", 1);
    clockMultBox.addItem("1/5", 2);
    clockMultBox.addItem("1/4", 3);
    clockMultBox.addItem("1/3", 4);
    clockMultBox.addItem("1/2", 5);
    clockMultBox.addItem("1X", 6);
    clockMultBox.addItem("2X", 7);
    clockMultBox.addItem("3X", 8);
    clockMultBox.addItem("4X", 9);
    clockMultBox.addItem("5X", 10);
    addAndMakeVisible(clockMultBox);

    for (int i = 0; i < 8; ++i)
    {
        add(stepPitch[i], true);
        add(stepVelocity[i], true);
    }

    auto& apvts = p.apvts;
    auto setDoubleClickToDefault = [&apvts](juce::Slider& slider, const juce::String& parameterId)
    {
        if (auto* parameter = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(parameterId)))
            slider.setDoubleClickReturnValue(true, parameter->convertFrom0to1(parameter->getDefaultValue()));
    };

    vcoDecayAtt = std::make_unique<SliderAttachment>(apvts, "vcoDecay", vcoDecay);
    seqPitchModBoxAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "seqPitchMod", seqPitchModBox);
    hardSyncBoxAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "hardSync", hardSyncBox);
    vco1WaveBoxAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "vco1Wave", vco1WaveBox);
    vco2WaveBoxAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "vco2Wave", vco2WaveBox);
    vcfModeBoxAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "vcfMode", vcfModeBox);
    vco1FreqAtt = std::make_unique<SliderAttachment>(apvts, "vco1Freq", vco1Frequency);
    vco1EgAmtAtt = std::make_unique<SliderAttachment>(apvts, "vco1EgAmt", vco1EgAmount);
    fmAmountAtt = std::make_unique<SliderAttachment>(apvts, "fmAmount", fmAmount);
    vco2FreqAtt = std::make_unique<SliderAttachment>(apvts, "vco2Freq", vco2Frequency);
    vco2EgAmtAtt = std::make_unique<SliderAttachment>(apvts, "vco2EgAmt", vco2EgAmount);
    noiseLevelAtt = std::make_unique<SliderAttachment>(apvts, "noiseLevel", noiseLevel);
    vco1LevelAtt = std::make_unique<SliderAttachment>(apvts, "vco1Level", vco1Level);
    vco2LevelAtt = std::make_unique<SliderAttachment>(apvts, "vco2Level", vco2Level);
    cutoffAtt = std::make_unique<SliderAttachment>(apvts, "cutoff", cutoff);
    resonanceAtt = std::make_unique<SliderAttachment>(apvts, "resonance", resonance);
    vcfDecayAtt = std::make_unique<SliderAttachment>(apvts, "vcfDecay", vcfDecay);
    vcfEgAmtAtt = std::make_unique<SliderAttachment>(apvts, "vcfEgAmt", vcfEgAmount);
    noiseVcfModAtt = std::make_unique<SliderAttachment>(apvts, "noiseVcfMod", noiseVcfMod);
    vcaDecayAtt = std::make_unique<SliderAttachment>(apvts, "vcaDecay", vcaDecay);
    vcaEgAtt = std::make_unique<SliderAttachment>(apvts, "vcaEg", vcaEg);
    volumeAtt = std::make_unique<SliderAttachment>(apvts, "volume", volume);
    clockMultBoxAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "clockMult", clockMultBox);

    for (int i = 0; i < 8; ++i)
    {
        const auto stepPitchId = "stepPitch" + juce::String(i);
        const auto stepVelId = "stepVel" + juce::String(i);
        stepPitchAtt[i] = std::make_unique<SliderAttachment>(apvts, stepPitchId, stepPitch[i]);
        stepVelAtt[i] = std::make_unique<SliderAttachment>(apvts, stepVelId, stepVelocity[i]);
        setDoubleClickToDefault(stepPitch[i], stepPitchId);
        setDoubleClickToDefault(stepVelocity[i], stepVelId);
    }

    vco1EgAmount.setDoubleClickReturnValue(true, 0.0);
    vco2EgAmount.setDoubleClickReturnValue(true, 0.0);
    vcfEgAmount.setDoubleClickReturnValue(true, 0.0);

    processor.getCableSnapshot(lastCableSnapshot);
    refreshPresetControls();
}

DFAFEditor::~DFAFEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void DFAFEditor::refreshPresetControls()
{
    const auto presetNames = processor.getAvailablePresetNames();
    const auto currentPreset = processor.getCurrentPresetName();

    isUpdatingPresetBox = true;
    presetBox.clear(juce::dontSendNotification);
    presetBox.addItem("INIT", kPresetInitId);

    for (int i = 0; i < presetNames.size(); ++i)
        presetBox.addItem(presetNames[i], kPresetFirstUserId + i);

    if (currentPreset.isEmpty() || currentPreset == "Init")
    {
        presetBox.setSelectedId(kPresetInitId, juce::dontSendNotification);
    }
    else
    {
        const int presetIndex = presetNames.indexOf(currentPreset);
        if (presetIndex >= 0)
            presetBox.setSelectedId(kPresetFirstUserId + presetIndex, juce::dontSendNotification);
        else
        {
            presetBox.addItem(currentPreset + " *", kPresetMissingId);
            presetBox.setSelectedId(kPresetMissingId, juce::dontSendNotification);
        }
    }
    isUpdatingPresetBox = false;

    lastPresetName = currentPreset;
    updatePresetButtonState();
}

void DFAFEditor::promptSavePreset()
{
    const auto currentPreset = processor.getCurrentPresetName();
    if (currentPreset.isNotEmpty()
        && currentPreset != "Init"
        && processor.getAvailablePresetNames().contains(currentPreset))
    {
        processor.saveCurrentPreset();
        refreshPresetControls();
        return;
    }

    auto presetDirectory = getDefaultPresetDirectory();
    presetDirectory.createDirectory();

    const auto suggestedName = currentPreset == "Init" ? juce::String("New Preset") : currentPreset;
    const auto suggestedFile = presetDirectory.getChildFile(suggestedName + ".dfafpreset");

    presetSaveChooser = std::make_unique<juce::FileChooser>("Save DFAF Preset",
                                                            suggestedFile,
                                                            "*.dfafpreset");

    presetSaveChooser->launchAsync(juce::FileBrowserComponent::saveMode
                                       | juce::FileBrowserComponent::canSelectFiles,
                                   [this](const juce::FileChooser& chooser)
                                   {
                                       const auto result = chooser.getResult();
                                       presetSaveChooser.reset();

                                       if (result == juce::File())
                                           return;

                                       processor.savePreset(result.getFileNameWithoutExtension());
                                       refreshPresetControls();
                                   });
}

void DFAFEditor::updatePresetButtonState()
{
    const auto currentPreset = processor.getCurrentPresetName();
    const bool hasSavedPreset = currentPreset.isNotEmpty()
        && currentPreset != "Init"
        && processor.getAvailablePresetNames().contains(currentPreset);

    presetDeleteButton.setEnabled(hasSavedPreset);
}

juce::Rectangle<int> DFAFEditor::getContentBounds() const
{
    return getLocalBounds().reduced(kOuterMargin);
}

juce::Rectangle<int> DFAFEditor::getMainPanelBounds() const
{
    auto bounds = getContentBounds().withTrimmedLeft(kWoodWidth).withTrimmedRight(kWoodWidth);
    bounds.removeFromBottom(kUtilityHeight);
    return bounds;
}

juce::Rectangle<int> DFAFEditor::getUtilityAreaBounds() const
{
    auto bounds = getContentBounds().withTrimmedLeft(kWoodWidth).withTrimmedRight(kWoodWidth);
    return bounds.removeFromBottom(kUtilityHeight);
}

juce::Rectangle<int> DFAFEditor::getPatchAreaBounds() const
{
    auto main = getMainPanelBounds().reduced(18, 16);
    return main.removeFromRight(kPatchWidth);
}

juce::Point<int> DFAFEditor::getJackCentre(PatchPoint pp) const
{
    const auto area = getPatchAreaBounds();
    const auto inputs = inputOrder();
    const auto outputs = outputOrder();
    const int inX = area.getX() + 56;
    const int outX = area.getRight() - 56;
    const int topY = area.getY() + 94;
    const int rowGap = juce::roundToInt((float) (area.getHeight() - 148) / 6.0f);

    for (int i = 0; i < 7; ++i)
    {
        if (outputs[(size_t) i] == pp)
            return { outX, topY + rowGap * i };
        if (inputs[(size_t) i] == pp)
            return { inX, topY + rowGap * i };
    }

    return { -1, -1 };
}

PatchPoint DFAFEditor::jackHitTest(juce::Point<int> pos) const
{
    constexpr int hitR2 = 14 * 14;

    for (int p = 0; p < PP_NUM_POINTS; ++p)
    {
        auto point = static_cast<PatchPoint>(p);
        auto centre = getJackCentre(point);
        if (centre.x < 0)
            continue;

        const int dx = pos.x - centre.x;
        const int dy = pos.y - centre.y;
        if (dx * dx + dy * dy <= hitR2)
            return point;
    }

    return PP_NUM_POINTS;
}

void DFAFEditor::mouseDown(const juce::MouseEvent& e)
{
    PatchPoint hit = jackHitTest(e.getPosition());

    if (hit == PP_NUM_POINTS)
    {
        pendingOut = PP_NUM_POINTS;
        repaint();
        return;
    }

    if (kPatchMeta[hit].dir == PD_Out)
    {
        pendingOut = (pendingOut == hit) ? PP_NUM_POINTS : hit;
        repaint();
        return;
    }

    if (pendingOut == PP_NUM_POINTS)
        return;

    std::vector<PatchCable> cables;
    processor.getCableSnapshot(cables);

    bool alreadyConnected = false;
    for (const auto& c : cables)
        if (c.src == pendingOut && c.dst == hit)
            alreadyConnected = true;

    if (alreadyConnected)
        processor.disconnectPatch(pendingOut, hit);
    else
        processor.connectPatch(pendingOut, hit, 1.0f);

    pendingOut = PP_NUM_POINTS;
    repaint();
}

void DFAFEditor::drawJackPanel(juce::Graphics& g, juce::Rectangle<int> area,
                               const std::vector<PatchCable>& cables,
                               PatchPoint selectedOut) const
{
    auto panel = area.toFloat().reduced(4.0f, 8.0f);

    g.setColour(kDividerSoft);
    g.drawLine(panel.getX() - 12.0f, panel.getY(), panel.getX() - 12.0f, panel.getBottom(), 1.0f);

    g.setColour(kPanelCream.brighter(0.05f));
    g.fillRoundedRectangle(panel, 8.0f);
    g.setColour(kDivider);
    g.drawRoundedRectangle(panel, 8.0f, 1.0f);

    g.setColour(kDividerSoft);
    g.drawLine(panel.getCentreX(), panel.getY() + 20.0f, panel.getCentreX(), panel.getBottom() - 18.0f, 1.0f);

    g.setColour(kInk);
    g.setFont(makeFont(12.0f, true));
    g.drawFittedText("IN", juce::Rectangle<int>(area.getX() + 30, area.getY() + 16, 56, 18), juce::Justification::centred, 1);
    g.drawFittedText("OUT", juce::Rectangle<int>(area.getRight() - 86, area.getY() + 16, 56, 18), juce::Justification::centred, 1);
    g.setColour(kDivider);
    g.drawLine((float) area.getX() + 14.0f, (float) area.getY() + 25.0f, (float) area.getX() + 30.0f, (float) area.getY() + 25.0f, 1.0f);
    g.drawLine((float) area.getX() + 86.0f, (float) area.getY() + 25.0f, panel.getCentreX() - 10.0f, (float) area.getY() + 25.0f, 1.0f);
    g.drawLine(panel.getCentreX() + 10.0f, (float) area.getY() + 25.0f, (float) area.getRight() - 86.0f, (float) area.getY() + 25.0f, 1.0f);
    g.drawLine((float) area.getRight() - 30.0f, (float) area.getY() + 25.0f, (float) area.getRight() - 14.0f, (float) area.getY() + 25.0f, 1.0f);

    auto drawJack = [&](PatchPoint pp, juce::Point<int> centre, bool isOutput)
    {
        const auto c = juce::Point<float>((float) centre.x, (float) centre.y);
        auto outer = juce::Rectangle<float>(24.0f, 24.0f).withCentre(c);

        g.setColour(juce::Colour(0xff181818));
        g.fillEllipse(outer);
        g.setColour(juce::Colour(0xff5f5f5f));
        g.drawEllipse(outer, 1.3f);
        g.setColour(juce::Colour(0xff8f8f8f));
        g.fillEllipse(outer.reduced(7.0f));

        if (selectedOut == pp)
        {
            g.setColour(juce::Colour(0xffffd268));
            g.drawEllipse(outer.expanded(4.0f), 2.0f);
        }

        auto labelBounds = juce::Rectangle<float>(76.0f, 18.0f);
        labelBounds = labelBounds.withCentre({ isOutput ? c.x : c.x, c.y - 34.0f });
        g.setColour(kInk.withAlpha(0.82f));
        g.setFont(makeFont(10.0f, true));
        g.drawFittedText(kPatchMeta[pp].name, labelBounds.toNearestInt(), juce::Justification::centred, 2);
    };

    const auto inputs = inputOrder();
    const auto outputs = outputOrder();
    for (int i = 0; i < 7; ++i)
    {
        drawJack(outputs[(size_t) i], getJackCentre(outputs[(size_t) i]), true);
        drawJack(inputs[(size_t) i], getJackCentre(inputs[(size_t) i]), false);
    }

    for (const auto& cable : cables)
    {
        if (!cable.enabled)
            continue;

        const auto src = getJackCentre(cable.src).toFloat();
        const auto dst = getJackCentre(cable.dst).toFloat();
        if (src.x < 0.0f || dst.x < 0.0f)
            continue;

        juce::Path cablePath;
        cablePath.startNewSubPath(src);
        const float bend = juce::jmax(26.0f, std::abs(dst.x - src.x) * 0.45f);
        cablePath.cubicTo(src.x + bend, src.y - 10.0f, dst.x - bend, dst.y - 10.0f, dst.x, dst.y);

        const auto cableColour = cable.src == PP_FM_AMT || cable.dst == PP_FM_AMT ? kCableDark : kCableWarm;
        g.setColour(cableColour.withAlpha(0.95f));
        g.strokePath(cablePath, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(juce::Colour(0x25ffffff));
        g.strokePath(cablePath, juce::PathStrokeType(1.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour(kJackGreen);
        g.drawEllipse(juce::Rectangle<float>(22.0f, 22.0f).withCentre(dst).expanded(4.0f), 1.6f);
    }
}

void DFAFEditor::timerCallback()
{
    bool needsRepaint = false;
    const int step = processor.getCurrentStep();
    const auto currentPreset = processor.getCurrentPresetName();

    if (currentPreset != lastPresetName)
        refreshPresetControls();

    if (step >= 0)
        resetLedActive = false;

    const int displayStep = resetLedActive ? 0 : step;
    if (displayStep != currentLedStep)
    {
        currentLedStep = displayStep;
        needsRepaint = true;
    }

    std::vector<PatchCable> cables;
    processor.getCableSnapshot(cables);
    if (!cableSnapshotsEqual(cables, lastCableSnapshot))
    {
        lastCableSnapshot = std::move(cables);
        pendingOut = PP_NUM_POINTS;
        needsRepaint = true;
    }

    if (needsRepaint)
        repaint();
}

void DFAFEditor::setupKnob(juce::Slider& s, bool small)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    s.setRange(0.0, 1.0);
    s.setValue(0.5);
    s.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    if (small)
        s.setScrollWheelEnabled(false);
}

void DFAFEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xffd9d6d0));

    auto bounds = getLocalBounds().toFloat();
    auto content = getContentBounds().toFloat();
    auto face = content.withTrimmedLeft((float) kWoodWidth).withTrimmedRight((float) kWoodWidth);
    auto mainPanel = getMainPanelBounds().toFloat();
    auto utility = getUtilityAreaBounds().toFloat();

    g.setColour(juce::Colour(0x33000000));
    g.fillEllipse(bounds.withTrimmedTop(bounds.getHeight() * 0.78f));

    g.setColour(kCaseOuter);
    g.fillRoundedRectangle(content.expanded(8.0f), 8.0f);

    juce::ColourGradient woodGrad(kWoodLight, content.getX(), content.getY(),
                                  kWoodDark, content.getRight(), content.getBottom(), true);
    g.setGradientFill(woodGrad);
    g.fillRoundedRectangle(content.withWidth((float) kWoodWidth + 10.0f), 10.0f);
    g.fillRoundedRectangle(content.withWidth((float) kWoodWidth + 10.0f).withRightX(content.getRight()), 10.0f);

    juce::ColourGradient faceGrad(kPanelCream.brighter(0.03f), face.getCentreX(), face.getY(),
                                  kPanelWarm, face.getCentreX(), face.getBottom(), true);
    g.setGradientFill(faceGrad);
    g.fillRoundedRectangle(face, 5.0f);
    g.setColour(juce::Colour(0x18ffffff));
    g.fillEllipse(face.withSizeKeepingCentre(face.getWidth() * 0.95f, face.getHeight() * 1.03f));
    g.setColour(kDivider);
    g.drawRoundedRectangle(face, 5.0f, 1.0f);

    drawScrew(g, { face.getX() + 14.0f, face.getY() + 14.0f });
    drawScrew(g, { face.getRight() - 14.0f, face.getY() + 14.0f });
    drawScrew(g, { face.getX() + 14.0f, face.getBottom() - 14.0f });
    drawScrew(g, { face.getRight() - 14.0f, face.getBottom() - 14.0f });

    auto layout = getMainPanelBounds().reduced(18, 16);
    auto patchArea = layout.removeFromRight(kPatchWidth);
    layout.removeFromRight(12);
    auto topArea = layout.removeFromTop(180);
    auto midArea = layout.removeFromTop(180);
    layout.removeFromTop(10);
    auto bottomArea = layout;

    g.setColour(kDivider);
    g.drawLine((float) topArea.getX(), (float) topArea.getBottom(), (float) patchArea.getX() - 10.0f, (float) topArea.getBottom(), 1.0f);
    g.drawLine((float) midArea.getX(), (float) midArea.getBottom(), (float) patchArea.getX() - 10.0f, (float) midArea.getBottom(), 1.0f);
    g.drawLine(face.getX() + 18.0f, utility.getY(), face.getRight() - 18.0f, utility.getY(), 1.0f);

    g.setColour(kInk);
    g.setFont(makeFont(14.0f, false));
    g.drawText("DFA", juce::Rectangle<int>((int) face.getX() + 28, (int) face.getY() + 18, 74, 28), juce::Justification::centredLeft);
    g.setColour(kAccentRed);
    g.drawText("F", juce::Rectangle<int>((int) face.getX() + 96, (int) face.getY() + 18, 24, 28), juce::Justification::centredLeft);

    auto drawCaptionUnder = [&](const juce::Component& c, const juce::String& text, float size = 11.0f)
    {
        auto b = c.getBounds().toFloat();
        drawLabel(g, { b.getX() - 10.0f, b.getBottom() + 4.0f, b.getWidth() + 20.0f, 16.0f }, text, size);
    };

    auto drawCaptionAbove = [&](const juce::Component& c, const juce::String& text, float size = 11.0f)
    {
        auto b = c.getBounds().toFloat();
        drawLabel(g, { b.getX() - 10.0f, b.getY() - 18.0f, b.getWidth() + 20.0f, 14.0f }, text, size);
    };

    drawCaptionAbove(vcoDecay, "VCO DECAY");
    drawCaptionAbove(seqPitchModBox, "SEQ PITCH MOD");
    drawCaptionAbove(vco1EgAmount, "VCO 1 EG AMT");
    drawCaptionAbove(vco1Frequency, "VCO 1 FREQ");
    drawCaptionAbove(vco1WaveBox, "VCO 1 WAVE");
    drawCaptionAbove(vco1Level, "VCO 1 LEVEL");
    drawCaptionAbove(noiseLevel, "NOISE LEVEL");
    drawCaptionAbove(vcfModeBox, "VCF MODE");
    drawCaptionAbove(cutoff, "CUTOFF");
    drawCaptionAbove(resonance, "RESONANCE");
    drawCaptionAbove(vcaEg, "VCA EG");

    drawCaptionAbove(fmAmount, "FM AMT");
    drawCaptionAbove(hardSyncBox, "HARD SYNC");
    drawCaptionAbove(vco2EgAmount, "VCO 2 EG AMT");
    drawCaptionAbove(vco2Frequency, "VCO 2 FREQ");
    drawCaptionAbove(vco2WaveBox, "VCO 2 WAVE");
    drawCaptionAbove(vco2Level, "VCO 2 LEVEL");
    drawCaptionAbove(vcfDecay, "VCF DECAY");
    drawCaptionAbove(vcfEgAmount, "VCF EG AMT");
    drawCaptionAbove(noiseVcfMod, "NOISE VCF MOD");
    drawCaptionAbove(vcaDecay, "VCA DECAY");

    drawCaptionUnder(volume, "VOLUME");
    drawCaptionUnder(resetButton, "RESET", 12.0f);

    g.setColour(kDividerSoft);
    g.drawLine((float) midArea.getX() + midArea.getWidth() * 0.58f, (float) midArea.getY() + 8.0f,
               (float) midArea.getX() + midArea.getWidth() * 0.58f, (float) midArea.getBottom() - 10.0f, 1.0f);

    auto transportArea = bottomArea.removeFromLeft(190);
    bottomArea.removeFromLeft(16);
    auto brandArea = bottomArea.removeFromRight(220);
    bottomArea.removeFromRight(10);
    auto seqArea = bottomArea;

    g.setColour(kInk);
    g.setFont(makeFont(13.0f, false));
    g.drawText("CLOCK / TRANSPORT", juce::Rectangle<int>(transportArea.getX() + 6, transportArea.getY() + 8, 160, 18), juce::Justification::centredLeft);
    drawCaptionAbove(clockMultBox, "CLOCK MULT", 10.5f);

    g.drawLine((float) transportArea.getRight() - 8.0f, (float) transportArea.getY() + 34.0f,
               (float) transportArea.getRight() - 8.0f, (float) transportArea.getBottom() - 10.0f, 1.0f);

    g.drawText("PITCH", juce::Rectangle<int>(seqArea.getX() + 6, seqArea.getY() + 86, 60, 16), juce::Justification::centredLeft);
    g.drawText("VELOCITY", juce::Rectangle<int>(seqArea.getX() - 8, seqArea.getY() + 164, 78, 16), juce::Justification::centredLeft);

    const int seqLabelWidth = 56;
    const int seqStartX = seqArea.getX() + seqLabelWidth + 20;
    const float seqStep = (float) (seqArea.getWidth() - seqLabelWidth - 28) / 8.0f;
    g.setFont(makeFont(12.0f, false));
    for (int i = 0; i < 8; ++i)
    {
        const float cx = seqStartX + seqStep * ((float) i + 0.5f);
        g.drawText(juce::String(i + 1), juce::Rectangle<int>((int) cx - 10, seqArea.getY() + 12, 20, 16), juce::Justification::centred);
        g.setColour(i == currentLedStep ? kLedRed : juce::Colour(0xffb14841));
        if (i == currentLedStep)
            g.fillEllipse(cx - 3.5f, (float) seqArea.getY() + 36.0f, 7.0f, 7.0f);
        else
            g.drawEllipse(cx - 3.5f, (float) seqArea.getY() + 36.0f, 7.0f, 7.0f, 1.2f);
        g.setColour(kDividerSoft);
        if (i < 8)
            g.drawLine(cx + seqStep * 0.5f, (float) seqArea.getY() + 34.0f, cx + seqStep * 0.5f, (float) seqArea.getBottom() - 18.0f, 1.0f);
        g.setColour(kInk);
    }

    g.setColour(kInk);
    g.setFont(makeFont(42.0f, true));
    g.drawText("DFAF", juce::Rectangle<int>(brandArea.getX() + 16, brandArea.getY() + 56, brandArea.getWidth() - 20, 46), juce::Justification::centredLeft);
    g.setColour(kAccentRed);
    g.setFont(makeFont(13.0f, false));
    g.drawText("DRUMMER FROM", juce::Rectangle<int>(brandArea.getX() + 20, brandArea.getY() + 118, 150, 18), juce::Justification::centredLeft);
    g.drawText("ANOTHER FATHER", juce::Rectangle<int>(brandArea.getX() + 20, brandArea.getY() + 136, 160, 18), juce::Justification::centredLeft);
    g.setColour(kDivider);
    g.drawLine((float) brandArea.getX() + 20.0f, (float) brandArea.getY() + 162.0f, (float) brandArea.getRight() - 24.0f, (float) brandArea.getY() + 162.0f, 1.0f);
    g.setColour(kInk);
    g.setFont(makeFont(11.0f, false));
    g.drawText("SEMI-MODULAR", juce::Rectangle<int>(brandArea.getX() + 20, brandArea.getY() + 172, 120, 16), juce::Justification::centredLeft);
    g.drawText("PERCUSSION", juce::Rectangle<int>(brandArea.getX() + 20, brandArea.getY() + 188, 120, 16), juce::Justification::centredLeft);
    g.drawText("VST SYNTHESIZER", juce::Rectangle<int>(brandArea.getX() + 20, brandArea.getY() + 204, 150, 16), juce::Justification::centredLeft);

    g.setColour(kInk.withAlpha(0.82f));
    g.setFont(makeFont(12.0f, false));
    g.drawText("PRESET", juce::Rectangle<int>((int) utility.getX() + 8, (int) utility.getY() + 24, 60, 16), juce::Justification::centredLeft);

    std::vector<PatchCable> cables;
    processor.getCableSnapshot(cables);
    drawJackPanel(g, getPatchAreaBounds(), cables, pendingOut);
}

void DFAFEditor::resized()
{
    auto main = getMainPanelBounds().reduced(18, 16);
    auto patch = main.removeFromRight(kPatchWidth);
    main.removeFromRight(12);
    auto top = main.removeFromTop(180);
    auto mid = main.removeFromTop(180);
    main.removeFromTop(10);
    auto bottom = main;

    auto setKnobCentre = [](juce::Slider& slider, int cx, int cy, int size)
    {
        slider.setBounds(cx - size / 2, cy - size / 2, size, size);
    };

    auto layoutTopRow = [&](juce::Rectangle<int> area)
    {
        auto inner = area.reduced(8, 12);
        const float unit = (float) inner.getWidth() / 12.0f;
        const int knobY = inner.getY() + 94;
        const int comboY = inner.getY() + 88;
        setKnobCentre(vcoDecay, inner.getX() + juce::roundToInt(unit * 0.55f), knobY, 58);
        seqPitchModBox.setBounds(inner.getX() + juce::roundToInt(unit * 1.35f), comboY, 96, 30);
        setKnobCentre(vco1EgAmount, inner.getX() + juce::roundToInt(unit * 2.85f), knobY, 58);
        setKnobCentre(vco1Frequency, inner.getX() + juce::roundToInt(unit * 4.02f), knobY, 72);
        vco1WaveBox.setBounds(inner.getX() + juce::roundToInt(unit * 4.85f), comboY, 68, 30);
        setKnobCentre(vco1Level, inner.getX() + juce::roundToInt(unit * 6.10f), knobY, 56);
        setKnobCentre(noiseLevel, inner.getX() + juce::roundToInt(unit * 7.15f), knobY, 56);
        vcfModeBox.setBounds(inner.getX() + juce::roundToInt(unit * 7.90f), comboY, 60, 30);
        setKnobCentre(cutoff, inner.getX() + juce::roundToInt(unit * 9.10f), knobY, 70);
        setKnobCentre(resonance, inner.getX() + juce::roundToInt(unit * 10.18f), knobY, 58);
        setKnobCentre(vcaEg, inner.getX() + juce::roundToInt(unit * 11.18f), knobY, 58);
    };

    auto layoutMidRow = [&](juce::Rectangle<int> area)
    {
        auto inner = area.reduced(8, 14);
        const float unit = (float) inner.getWidth() / 12.0f;
        const int knobY = inner.getY() + 92;
        const int comboY = inner.getY() + 86;
        setKnobCentre(fmAmount, inner.getX() + juce::roundToInt(unit * 0.55f), knobY, 58);
        hardSyncBox.setBounds(inner.getX() + juce::roundToInt(unit * 1.50f), comboY, 54, 30);
        setKnobCentre(vco2EgAmount, inner.getX() + juce::roundToInt(unit * 2.85f), knobY, 58);
        setKnobCentre(vco2Frequency, inner.getX() + juce::roundToInt(unit * 4.02f), knobY, 72);
        vco2WaveBox.setBounds(inner.getX() + juce::roundToInt(unit * 4.85f), comboY, 68, 30);
        setKnobCentre(vco2Level, inner.getX() + juce::roundToInt(unit * 6.10f), knobY, 56);
        setKnobCentre(vcfDecay, inner.getX() + juce::roundToInt(unit * 7.30f), knobY, 58);
        setKnobCentre(vcfEgAmount, inner.getX() + juce::roundToInt(unit * 8.55f), knobY, 58);
        setKnobCentre(noiseVcfMod, inner.getX() + juce::roundToInt(unit * 9.82f), knobY, 58);
        setKnobCentre(vcaDecay, inner.getX() + juce::roundToInt(unit * 11.05f), knobY, 58);
    };

    layoutTopRow(top);
    layoutMidRow(mid);

    auto transport = bottom.removeFromLeft(190);
    bottom.removeFromLeft(16);
    auto brand = bottom.removeFromRight(220);
    bottom.removeFromRight(10);
    auto seq = bottom;

    clockMultBox.setBounds(transport.getX() + 18, transport.getY() + 54, 116, 32);
    resetButton.setBounds(transport.getX() + 40, transport.getY() + 176, 72, 34);
    setKnobCentre(volume, brand.getX() + 108, brand.getY() + 124, 52);

    const int seqLabelWidth = 56;
    const int seqStartX = seq.getX() + seqLabelWidth + 20;
    const float seqStep = (float) (seq.getWidth() - seqLabelWidth - 28) / 8.0f;
    for (int i = 0; i < 8; ++i)
    {
        const int cx = juce::roundToInt(seqStartX + seqStep * ((float) i + 0.5f));
        setKnobCentre(stepPitch[i], cx, seq.getY() + 108, 50);
        setKnobCentre(stepVelocity[i], cx, seq.getY() + 188, 50);
    }

    auto utility = getUtilityAreaBounds().reduced(18, 12);
    presetBox.setBounds(utility.getX() + 54, utility.getY() + 14, 220, 32);
    presetSaveButton.setBounds(utility.getX() + 420, utility.getY() + 14, 58, 32);
    presetDeleteButton.setBounds(utility.getX() + 486, utility.getY() + 14, 64, 32);
    presetInitButton.setBounds(utility.getX() + 568, utility.getY() + 14, 58, 32);
}
