#include "PluginEditor.h"
#include "BinaryData.h"

#include <array>
#include <cmath>

namespace
{
constexpr int kEditorWidth = 1440;
constexpr int kEditorHeight = 760;

constexpr int kPresetInitId = 1;
constexpr int kPresetFirstUserId = 2;
constexpr int kPresetMissingId = 999;

const juce::Colour kPanelCream       = juce::Colour(0xffede5d1);
const juce::Colour kLedRed           = juce::Colour(0xffff6558);
const juce::Colour kButtonFace       = juce::Colour(0xff272727);
const juce::Colour kButtonFaceBright = juce::Colour(0xff3b3b3b);
const juce::Colour kButtonBorder     = juce::Colour(0xff111111);
const juce::Colour kJackGreen        = juce::Colour(0xff7ce381);
const juce::Colour kCableRed         = juce::Colour(0xffb24737);
const juce::Colour kCableDark        = juce::Colour(0xff383838);
const juce::Colour kCableAmber       = juce::Colour(0xffb06a3a);

constexpr float kPanelSourceWidth = 1689.0f;
constexpr float kPanelSourceHeight = 931.0f;
constexpr const char* kPanelControlId = "panel-control";

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

std::array<PatchPoint, 7> inputOrder()
{
    return { PP_VCA_CV, PP_VCA_DECAY, PP_VCF_MOD, PP_VCF_DECAY, PP_NOISE_LVL, PP_VCO_DECAY, PP_FM_AMT };
}

std::array<PatchPoint, 7> outputOrder()
{
    return { PP_VCA_EG, PP_VCF_EG, PP_VCO_EG, PP_VCO1, PP_VCO2, PP_VELOCITY, PP_PITCH };
}

void drawPanelToggle(juce::Graphics& g, juce::Rectangle<float> bounds, int selectedIndex, int numPositions)
{
    bounds = bounds.reduced(0.8f);
    g.setColour(juce::Colour(0x99000000));
    g.fillRoundedRectangle(bounds.translated(1.4f, 2.0f), 2.5f);

    juce::ColourGradient slot(juce::Colour(0xff4a4945), bounds.getX(), bounds.getY(),
                              juce::Colour(0xff151515), bounds.getRight(), bounds.getBottom(), true);
    g.setGradientFill(slot);
    g.fillRoundedRectangle(bounds, 2.5f);
    g.setColour(juce::Colour(0xff050505));
    g.drawRoundedRectangle(bounds, 2.5f, 1.0f);

    const auto segmentW = bounds.getWidth() / (float) numPositions;
    auto handle = bounds.withX(bounds.getX() + segmentW * (float) selectedIndex).withWidth(segmentW).reduced(1.2f);
    juce::ColourGradient handleFill(juce::Colour(0xff343330), handle.getX(), handle.getY(),
                                    juce::Colour(0xff111111), handle.getRight(), handle.getBottom(), true);
    g.setGradientFill(handleFill);
    g.fillRoundedRectangle(handle, 2.0f);
    g.setColour(juce::Colour(0x44000000));
    g.drawRoundedRectangle(handle, 2.0f, 1.0f);

    auto redDot = juce::Rectangle<float>(4.4f, 4.4f).withCentre({ handle.getRight() - 9.0f, handle.getCentreY() - 0.5f });
    g.setColour(juce::Colour(0xff78342d));
    g.fillEllipse(redDot);
}

juce::Path makeCablePath(juce::Point<float> src, juce::Point<float> dst, float slack = 1.0f)
{
    const auto dx = dst.x - src.x;
    const auto dy = dst.y - src.y;
    const auto distance = std::sqrt(dx * dx + dy * dy);
    const auto side = dx >= 0.0f ? 1.0f : -1.0f;
    const auto sag = juce::jlimit(14.0f, 74.0f, distance * 0.15f) * slack;
    const auto bow = juce::jlimit(28.0f, 112.0f, std::abs(dx) * 0.55f + 24.0f);

    juce::Path cable;
    cable.startNewSubPath(src);
    cable.cubicTo(src.x + bow * side, src.y + sag,
                  dst.x - bow * side, dst.y + sag,
                  dst.x, dst.y);
    return cable;
}

juce::Colour cableColourFor(const PatchCable& cable, int index)
{
    if (cable.src == PP_FM_AMT || cable.dst == PP_FM_AMT)
        return kCableDark;

    switch (index % 3)
    {
        case 0:  return kCableRed;
        case 1:  return kCableDark;
        default: return kCableAmber;
    }
}

void drawCable(juce::Graphics& g, juce::Point<float> src, juce::Point<float> dst,
               juce::Colour colour, float alpha, bool preview)
{
    auto cablePath = makeCablePath(src, dst, preview ? 0.82f : 1.0f);
    const auto width = preview ? 4.8f : 7.4f;

    auto shadowPath = cablePath;
    shadowPath.applyTransform(juce::AffineTransform::translation(2.0f, 3.2f));
    g.setColour(juce::Colour(0x8a000000).withAlpha(alpha * 0.62f));
    g.strokePath(shadowPath, juce::PathStrokeType(width + 5.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour(colour.darker(0.72f).withAlpha(alpha));
    g.strokePath(cablePath, juce::PathStrokeType(width + 2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour(colour.darker(0.08f).withAlpha(alpha));
    g.strokePath(cablePath, juce::PathStrokeType(width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    auto lowlightPath = cablePath;
    lowlightPath.applyTransform(juce::AffineTransform::translation(0.0f, 1.1f));
    g.setColour(juce::Colours::black.withAlpha(alpha * 0.24f));
    g.strokePath(lowlightPath, juce::PathStrokeType(width * 0.34f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    auto highlightPath = cablePath;
    highlightPath.applyTransform(juce::AffineTransform::translation(-0.55f, -1.25f));
    g.setColour(colour.brighter(0.72f).withAlpha(alpha * (preview ? 0.28f : 0.38f)));
    g.strokePath(highlightPath, juce::PathStrokeType(width * 0.18f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void drawCablePlug(juce::Graphics& g, juce::Point<float> centre, juce::Colour colour, bool active)
{
    auto collar = juce::Rectangle<float>(18.0f, 18.0f).withCentre(centre);
    g.setColour(juce::Colour(0x75000000));
    g.fillEllipse(collar.translated(1.2f, 1.8f));
    g.setColour(juce::Colour(0xff151515).withAlpha(active ? 0.96f : 0.86f));
    g.fillEllipse(collar);

    auto insert = collar.reduced(5.2f);
    g.setColour(colour.darker(0.35f).withAlpha(active ? 0.92f : 0.76f));
    g.fillEllipse(insert);
    g.setColour(colour.brighter(0.55f).withAlpha(0.30f));
    g.fillEllipse(insert.withTrimmedBottom(insert.getHeight() * 0.55f).reduced(0.8f, 0.0f));
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

bool DFAFLookAndFeel::isPanelControl(const juce::Component& component)
{
    return component.getComponentID().startsWith(kPanelControlId);
}

void DFAFLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                       juce::Slider& slider)
{
    if (slider.getProperties().getWithDefault("hidePanelIndicator", false))
        return;

    const auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height);
    const auto diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
    const auto centre = bounds.getCentre();

    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const float offset = (float) slider.getProperties().getWithDefault("panelIndicatorOffset", 0.0);
    const float indicatorAngle = angle - juce::MathConstants<float>::halfPi + offset;
    const float innerR = diameter * (float) slider.getProperties().getWithDefault("panelIndicatorInner", 0.08);
    const float outerR = diameter * (float) slider.getProperties().getWithDefault("panelIndicatorOuter", 0.28);
    const float lineWidth = (float) slider.getProperties().getWithDefault("panelIndicatorWidth", 3.0);
    const juce::Point<float> p0(centre.x + std::cos(indicatorAngle) * innerR,
                                centre.y + std::sin(indicatorAngle) * innerR);
    const juce::Point<float> p1(centre.x + std::cos(indicatorAngle) * outerR,
                                centre.y + std::sin(indicatorAngle) * outerR);

    g.setColour(juce::Colour(0xffefefef));
    g.drawLine(p0.x, p0.y, p1.x, p1.y, lineWidth);
}

void DFAFLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                           bool isMouseOverButton, bool isButtonDown)
{
    if (isPanelControl(button))
        return;

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
    if (isPanelControl(button))
        return;

    g.setColour(button.findColour(juce::TextButton::textColourOffId));
    g.setFont(makeFont(18.0f, true));
    g.drawFittedText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, 1);
}

void DFAFLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool,
                                   int, int, int, int, juce::ComboBox& box)
{
    if (isPanelControl(box))
        return;

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
    label.setColour(juce::Label::textColourId, isPanelControl(box) ? juce::Colours::transparentBlack
                                                                    : juce::Colour(0xfff3ede1));
    label.setBounds(2, 1, box.getWidth() - 22, box.getHeight() - 2);
}

DFAFEditor::DFAFEditor(DFAFProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(kEditorWidth, kEditorHeight);
    setLookAndFeel(&laf);
    panelImage = juce::ImageCache::getFromMemory(BinaryData::ig_0b9a34b24e54dfb20169ecc11305388191ac4d2979b55f8ab8_png,
                                                 BinaryData::ig_0b9a34b24e54dfb20169ecc11305388191ac4d2979b55f8ab8_pngSize);
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

    presetSaveButton.setComponentID(kPanelControlId);
    presetDeleteButton.setComponentID(kPanelControlId);
    presetInitButton.setComponentID(kPanelControlId);
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
    resetButton.setComponentID(kPanelControlId);
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
    seqPitchModBox.setComponentID(kPanelControlId);
    seqPitchModBox.onChange = [this]() { repaint(); };
    addAndMakeVisible(seqPitchModBox);

    hardSyncBox.addItem("OFF", 1);
    hardSyncBox.addItem("ON", 2);
    hardSyncBox.setComponentID(kPanelControlId);
    hardSyncBox.onChange = [this]() { repaint(); };
    addAndMakeVisible(hardSyncBox);

    vco1WaveBox.addItem("SQUARE", 1);
    vco1WaveBox.addItem("TRIANGLE", 2);
    vco1WaveBox.setComponentID(kPanelControlId);
    vco1WaveBox.onChange = [this]() { repaint(); };
    addAndMakeVisible(vco1WaveBox);

    vco2WaveBox.addItem("SQUARE", 1);
    vco2WaveBox.addItem("TRIANGLE", 2);
    vco2WaveBox.setComponentID(kPanelControlId);
    vco2WaveBox.onChange = [this]() { repaint(); };
    addAndMakeVisible(vco2WaveBox);

    vcfModeBox.addItem("LP", 1);
    vcfModeBox.addItem("HP", 2);
    vcfModeBox.setComponentID(kPanelControlId);
    vcfModeBox.onChange = [this]() { repaint(); };
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
    clockMultBox.setComponentID(kPanelControlId);
    clockMultBox.onChange = [this]() { repaint(); };
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

juce::Rectangle<float> DFAFEditor::getPanelImageBounds() const
{
    const auto scale = juce::jmin((float) getWidth() / kPanelSourceWidth,
                                 (float) getHeight() / kPanelSourceHeight);
    const auto w = kPanelSourceWidth * scale;
    const auto h = kPanelSourceHeight * scale;
    return { ((float) getWidth() - w) * 0.5f, ((float) getHeight() - h) * 0.5f, w, h };
}

juce::Rectangle<int> DFAFEditor::mapPanelRect(float x, float y, float w, float h) const
{
    const auto panel = getPanelImageBounds();
    const auto scale = panel.getWidth() / kPanelSourceWidth;
    return juce::Rectangle<float>(panel.getX() + x * scale,
                                  panel.getY() + y * scale,
                                  w * scale,
                                  h * scale).toNearestInt();
}

juce::Point<int> DFAFEditor::mapPanelPoint(float x, float y) const
{
    const auto panel = getPanelImageBounds();
    const auto scale = panel.getWidth() / kPanelSourceWidth;
    return { juce::roundToInt(panel.getX() + x * scale),
             juce::roundToInt(panel.getY() + y * scale) };
}

juce::Rectangle<int> DFAFEditor::getPatchAreaBounds() const
{
    return mapPanelRect(1388.0f, 110.0f, 211.0f, 708.0f);
}

juce::Point<int> DFAFEditor::getJackCentre(PatchPoint pp) const
{
    const auto inputs = inputOrder();
    const auto outputs = outputOrder();
    constexpr std::array<float, 7> ys { 216.0f, 305.0f, 394.0f, 482.0f, 572.0f, 661.0f, 750.0f };
    constexpr float inX = 1442.0f;
    constexpr float outX = 1543.0f;

    for (int i = 0; i < 7; ++i)
    {
        if (outputs[(size_t) i] == pp)
            return mapPanelPoint(outX, ys[(size_t) i]);
        if (inputs[(size_t) i] == pp)
            return mapPanelPoint(inX, ys[(size_t) i]);
    }

    return { -1, -1 };
}

PatchPoint DFAFEditor::jackHitTest(juce::Point<int> pos) const
{
    constexpr int hitR2 = 18 * 18;

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
    dragPatchPoint = hit;
    dragHoverPoint = hit;
    dragPatchPos = e.getPosition();
    isDraggingPatchCable = false;

    if (hit == PP_NUM_POINTS)
    {
        pendingOut = PP_NUM_POINTS;
        repaint();
        return;
    }
}

void DFAFEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (dragPatchPoint == PP_NUM_POINTS)
        return;

    dragPatchPos = e.getPosition();
    dragHoverPoint = jackHitTest(dragPatchPos);

    if (e.getDistanceFromDragStart() > 3)
    {
        isDraggingPatchCable = true;
        if (kPatchMeta[dragPatchPoint].dir == PD_Out)
            pendingOut = dragPatchPoint;
    }

    repaint();
}

void DFAFEditor::mouseUp(const juce::MouseEvent& e)
{
    if (!isDraggingPatchCable || dragPatchPoint == PP_NUM_POINTS)
    {
        const auto hit = jackHitTest(e.getPosition());

        if (hit != PP_NUM_POINTS && kPatchMeta[hit].dir == PD_Out)
        {
            pendingOut = (pendingOut == hit) ? PP_NUM_POINTS : hit;
        }
        else if (hit != PP_NUM_POINTS && pendingOut != PP_NUM_POINTS)
        {
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
        }

        dragPatchPoint = PP_NUM_POINTS;
        dragHoverPoint = PP_NUM_POINTS;
        isDraggingPatchCable = false;
        repaint();
        return;
    }

    const auto releasePoint = jackHitTest(e.getPosition());
    PatchPoint src = PP_NUM_POINTS;
    PatchPoint dst = PP_NUM_POINTS;

    if (releasePoint != PP_NUM_POINTS && kPatchMeta[dragPatchPoint].dir != kPatchMeta[releasePoint].dir)
    {
        if (kPatchMeta[dragPatchPoint].dir == PD_Out)
        {
            src = dragPatchPoint;
            dst = releasePoint;
        }
        else
        {
            src = releasePoint;
            dst = dragPatchPoint;
        }

        std::vector<PatchCable> cables;
        processor.getCableSnapshot(cables);

        bool alreadyConnected = false;
        for (const auto& c : cables)
            if (c.src == src && c.dst == dst)
                alreadyConnected = true;

        if (alreadyConnected)
            processor.disconnectPatch(src, dst);
        else
            processor.connectPatch(src, dst, 1.0f);
    }

    pendingOut = PP_NUM_POINTS;
    dragPatchPoint = PP_NUM_POINTS;
    dragHoverPoint = PP_NUM_POINTS;
    isDraggingPatchCable = false;
    repaint();
}

void DFAFEditor::drawJackPanel(juce::Graphics& g, juce::Rectangle<int> area,
                               const std::vector<PatchCable>& cables,
                               PatchPoint selectedOut) const
{
    juce::ignoreUnused(area);

    int cableIndex = 0;
    for (const auto& cable : cables)
    {
        if (!cable.enabled)
            continue;

        const auto src = getJackCentre(cable.src).toFloat();
        const auto dst = getJackCentre(cable.dst).toFloat();
        if (src.x < 0.0f || dst.x < 0.0f)
            continue;

        const auto cableColour = cableColourFor(cable, cableIndex++);
        drawCable(g, src, dst, cableColour, 0.96f, false);
    }

    for (const auto& cable : cables)
    {
        if (!cable.enabled)
            continue;

        const auto src = getJackCentre(cable.src).toFloat();
        const auto dst = getJackCentre(cable.dst).toFloat();
        if (src.x < 0.0f || dst.x < 0.0f)
            continue;

        const auto colour = cableColourFor(cable, cableIndex++);
        drawCablePlug(g, src, colour, true);
        drawCablePlug(g, dst, colour, true);
    }

    if (isDraggingPatchCable && dragPatchPoint != PP_NUM_POINTS)
    {
        const auto start = getJackCentre(dragPatchPoint).toFloat();
        auto end = dragPatchPos.toFloat();
        const bool hoverCompatible = dragHoverPoint != PP_NUM_POINTS
            && kPatchMeta[dragHoverPoint].dir != kPatchMeta[dragPatchPoint].dir;
        if (hoverCompatible)
            end = getJackCentre(dragHoverPoint).toFloat();

        const auto previewColour = hoverCompatible ? kCableRed : kCableDark;
        drawCable(g, start, end, previewColour, hoverCompatible ? 0.88f : 0.52f, true);
        drawCablePlug(g, start, previewColour, true);

        if (hoverCompatible)
            drawCablePlug(g, end, previewColour, true);
    }

    auto drawJackState = [&](PatchPoint pp)
    {
        const auto c = getJackCentre(pp).toFloat();
        if (c.x < 0.0f)
            return;

        const bool selected = selectedOut == pp || dragPatchPoint == pp;
        const bool hoverCompatible = isDraggingPatchCable
            && dragHoverPoint == pp
            && dragPatchPoint != PP_NUM_POINTS
            && kPatchMeta[dragPatchPoint].dir != kPatchMeta[pp].dir;

        if (selected || hoverCompatible)
        {
            const auto ring = juce::Rectangle<float>(26.0f, 26.0f).withCentre(c);
            g.setColour((hoverCompatible ? kJackGreen : juce::Colour(0xffffd268)).withAlpha(0.26f));
            g.fillEllipse(ring.expanded(5.0f));
            g.setColour(hoverCompatible ? kJackGreen : juce::Colour(0xffffd268));
            g.drawEllipse(ring.expanded(3.0f), 2.0f);
        }
    };

    const auto inputs = inputOrder();
    const auto outputs = outputOrder();
    for (int i = 0; i < 7; ++i)
    {
        drawJackState(outputs[(size_t) i]);
        drawJackState(inputs[(size_t) i]);
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
    s.setComponentID(kPanelControlId);
    s.setRotaryParameters(juce::MathConstants<float>::pi * 1.18f,
                          juce::MathConstants<float>::pi * 2.82f,
                          true);
    s.getProperties().set("panelIndicatorInner", 0.06);
    s.getProperties().set("panelIndicatorOuter", small ? 0.25 : 0.27);
    s.getProperties().set("panelIndicatorWidth", small ? 2.2 : 2.6);
    s.onValueChange = [this]() { repaint(); };
    if (small)
        s.setScrollWheelEnabled(false);
}

void DFAFEditor::drawPanelSwitches(juce::Graphics& g) const
{
    drawPanelToggle(g, mapPanelRect(231.0f, 182.0f, 96.0f, 33.0f).toFloat(),
                    juce::jlimit(0, 2, seqPitchModBox.getSelectedItemIndex()), 3);
    drawPanelToggle(g, mapPanelRect(631.0f, 171.0f, 54.0f, 30.0f).toFloat(),
                    juce::jlimit(0, 1, vco1WaveBox.getSelectedItemIndex()), 2);
    drawPanelToggle(g, mapPanelRect(958.0f, 191.0f, 43.0f, 31.0f).toFloat(),
                    juce::jlimit(0, 1, vcfModeBox.getSelectedItemIndex()), 2);
    drawPanelToggle(g, mapPanelRect(250.0f, 405.0f, 52.0f, 31.0f).toFloat(),
                    juce::jlimit(0, 1, hardSyncBox.getSelectedItemIndex()), 2);
    drawPanelToggle(g, mapPanelRect(631.0f, 395.0f, 54.0f, 30.0f).toFloat(),
                    juce::jlimit(0, 1, vco2WaveBox.getSelectedItemIndex()), 2);

    if (resetLedActive)
    {
        auto led = mapPanelRect(183.0f, 704.0f, 9.0f, 9.0f).toFloat();
        g.setColour(kLedRed.withAlpha(0.28f));
        g.fillEllipse(led.expanded(5.0f));
        g.setColour(kLedRed);
        g.fillEllipse(led);
    }
}

void DFAFEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xffd9d6d0));
    const auto panel = getPanelImageBounds();
    if (panelImage.isValid())
        g.drawImage(panelImage, panel);
    else
        g.fillAll(kPanelCream);

    drawPanelSwitches(g);

    for (int i = 0; i < 8; ++i)
    {
        const auto led = mapPanelRect(409.0f + 86.0f * (float) i, 571.0f, 9.0f, 9.0f).toFloat();
        if (i == currentLedStep)
        {
            g.setColour(kLedRed.withAlpha(0.28f));
            g.fillEllipse(led.expanded(5.0f));
            g.setColour(kLedRed);
            g.fillEllipse(led);
        }
    }

    std::vector<PatchCable> cables;
    processor.getCableSnapshot(cables);
    drawJackPanel(g, getPatchAreaBounds(), cables, pendingOut);
}

void DFAFEditor::resized()
{
    auto setKnobCentre = [this](juce::Slider& slider, float cx, float cy, float size)
    {
        slider.setBounds(mapPanelRect(cx - size * 0.5f, cy - size * 0.5f, size, size));
    };

    setKnobCentre(vcoDecay, 151.0f, 184.0f, 62.0f);
    setKnobCentre(vco1EgAmount, 400.0f, 184.0f, 62.0f);
    setKnobCentre(vco1Frequency, 529.0f, 184.0f, 82.0f);
    setKnobCentre(vco1Level, 765.0f, 184.0f, 60.0f);
    setKnobCentre(noiseLevel, 884.0f, 184.0f, 60.0f);
    setKnobCentre(cutoff, 1084.0f, 184.0f, 78.0f);
    setKnobCentre(resonance, 1196.0f, 184.0f, 62.0f);
    setKnobCentre(vcaEg, 1303.0f, 184.0f, 62.0f);

    setKnobCentre(fmAmount, 151.0f, 407.0f, 62.0f);
    setKnobCentre(vco2EgAmount, 400.0f, 407.0f, 62.0f);
    setKnobCentre(vco2Frequency, 529.0f, 407.0f, 82.0f);
    setKnobCentre(vco2Level, 765.0f, 407.0f, 60.0f);
    setKnobCentre(vcfDecay, 889.0f, 407.0f, 62.0f);
    setKnobCentre(vcfEgAmount, 1024.0f, 407.0f, 62.0f);
    setKnobCentre(noiseVcfMod, 1156.0f, 407.0f, 62.0f);
    setKnobCentre(vcaDecay, 1284.0f, 407.0f, 62.0f);

    setKnobCentre(volume, 185.0f, 640.0f, 76.0f);
    clockMultBox.setBounds(mapPanelRect(130.0f, 593.0f, 116.0f, 90.0f));
    resetButton.setBounds(mapPanelRect(163.0f, 724.0f, 45.0f, 40.0f));

    seqPitchModBox.setBounds(mapPanelRect(231.0f, 182.0f, 96.0f, 34.0f));
    vco1WaveBox.setBounds(mapPanelRect(631.0f, 171.0f, 54.0f, 51.0f));
    vcfModeBox.setBounds(mapPanelRect(958.0f, 171.0f, 43.0f, 51.0f));
    hardSyncBox.setBounds(mapPanelRect(250.0f, 405.0f, 52.0f, 58.0f));
    vco2WaveBox.setBounds(mapPanelRect(631.0f, 395.0f, 54.0f, 51.0f));

    for (int i = 0; i < 8; ++i)
    {
        const float cx = 411.0f + 86.5f * (float) i;
        setKnobCentre(stepPitch[i], cx, 619.0f, 50.0f);
        setKnobCentre(stepVelocity[i], cx, 698.0f, 50.0f);
    }

    presetBox.setBounds(mapPanelRect(178.0f, 815.0f, 258.0f, 42.0f));
    presetSaveButton.setBounds(mapPanelRect(648.0f, 827.0f, 34.0f, 28.0f));
    presetDeleteButton.setBounds(mapPanelRect(724.0f, 827.0f, 42.0f, 28.0f));
    presetInitButton.setBounds(mapPanelRect(799.0f, 827.0f, 37.0f, 28.0f));
}
