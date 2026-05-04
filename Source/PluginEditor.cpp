#include "PluginEditor.h"
#include "BinaryData.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>

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

constexpr float kPanelSourceWidth = 1688.0f;
constexpr float kPanelSourceHeight = 932.0f;
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

class RoyalCatSaveArt final : public juce::Component
{
public:
    RoyalCatSaveArt()
    {
        setSize(340, 150);
    }

    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat().reduced(10.0f);

        g.setColour(juce::Colour(0xfff7efd9));
        g.fillRoundedRectangle(area, 8.0f);
        g.setColour(juce::Colour(0xffc7a85e));
        g.drawRoundedRectangle(area, 8.0f, 1.5f);

        drawCat(g, area.removeFromLeft(area.getWidth() * 0.48f).reduced(8.0f));
        drawKing(g, area.reduced(8.0f));
    }

private:
    static void drawCat(juce::Graphics& g, juce::Rectangle<float> area)
    {
        const auto head = juce::Rectangle<float>(84.0f, 70.0f).withCentre({ area.getCentreX(), area.getCentreY() + 8.0f });
        const auto leftEar = juce::Point<float>(head.getX() + 15.0f, head.getY() + 8.0f);
        const auto rightEar = juce::Point<float>(head.getRight() - 15.0f, head.getY() + 8.0f);

        juce::Path face;
        face.startNewSubPath(leftEar);
        face.lineTo(head.getX() + 24.0f, head.getY() - 20.0f);
        face.lineTo(head.getX() + 38.0f, head.getY() + 2.0f);
        face.lineTo(head.getRight() - 38.0f, head.getY() + 2.0f);
        face.lineTo(head.getRight() - 24.0f, head.getY() - 20.0f);
        face.lineTo(rightEar);
        face.cubicTo(head.getRight() + 10.0f, head.getY() + 34.0f,
                     head.getRight() - 10.0f, head.getBottom() + 8.0f,
                     head.getCentreX(), head.getBottom() + 8.0f);
        face.cubicTo(head.getX() + 10.0f, head.getBottom() + 8.0f,
                     head.getX() - 10.0f, head.getY() + 34.0f,
                     leftEar.x, leftEar.y);
        face.closeSubPath();

        g.setColour(juce::Colour(0xffffc16e));
        g.fillPath(face);
        g.setColour(juce::Colour(0xff6b4a27));
        g.strokePath(face, juce::PathStrokeType(2.0f));

        g.setColour(juce::Colour(0xff2b221c));
        g.fillEllipse(head.getCentreX() - 24.0f, head.getCentreY() - 8.0f, 8.0f, 12.0f);
        g.fillEllipse(head.getCentreX() + 16.0f, head.getCentreY() - 8.0f, 8.0f, 12.0f);

        g.setColour(juce::Colour(0xffff7d89));
        g.fillEllipse(head.getCentreX() - 5.0f, head.getCentreY() + 9.0f, 10.0f, 7.0f);

        g.setColour(juce::Colour(0xff6b4a27));
        g.drawLine(head.getCentreX() - 30.0f, head.getCentreY() + 12.0f, head.getCentreX() - 58.0f, head.getCentreY() + 5.0f, 1.3f);
        g.drawLine(head.getCentreX() - 30.0f, head.getCentreY() + 20.0f, head.getCentreX() - 58.0f, head.getCentreY() + 23.0f, 1.3f);
        g.drawLine(head.getCentreX() + 30.0f, head.getCentreY() + 12.0f, head.getCentreX() + 58.0f, head.getCentreY() + 5.0f, 1.3f);
        g.drawLine(head.getCentreX() + 30.0f, head.getCentreY() + 20.0f, head.getCentreX() + 58.0f, head.getCentreY() + 23.0f, 1.3f);
    }

    static void drawKing(juce::Graphics& g, juce::Rectangle<float> area)
    {
        const auto centre = area.getCentre();
        const auto portrait = juce::Rectangle<float>(82.0f, 74.0f).withCentre({ centre.x, centre.y + 12.0f });

        g.setColour(juce::Colour(0xff2f5fb8));
        g.fillRoundedRectangle(portrait.withY(portrait.getY() + 34.0f).expanded(10.0f, 4.0f), 12.0f);
        g.setColour(juce::Colour(0xffffd84a));
        g.drawLine(portrait.getX() - 6.0f, portrait.getBottom() - 12.0f,
                   portrait.getRight() + 6.0f, portrait.getBottom() - 42.0f, 5.0f);

        g.setColour(juce::Colour(0xffffd2a1));
        g.fillEllipse(portrait.withHeight(66.0f).reduced(8.0f, 0.0f));
        g.setColour(juce::Colour(0xff6b4a27));
        g.drawEllipse(portrait.withHeight(66.0f).reduced(8.0f, 0.0f), 1.7f);

        juce::Path crown;
        const auto y = portrait.getY() - 9.0f;
        crown.startNewSubPath(portrait.getX() + 12.0f, y + 25.0f);
        crown.lineTo(portrait.getX() + 21.0f, y);
        crown.lineTo(portrait.getX() + 36.0f, y + 21.0f);
        crown.lineTo(portrait.getCentreX(), y - 8.0f);
        crown.lineTo(portrait.getRight() - 36.0f, y + 21.0f);
        crown.lineTo(portrait.getRight() - 21.0f, y);
        crown.lineTo(portrait.getRight() - 12.0f, y + 25.0f);
        crown.closeSubPath();
        g.setColour(juce::Colour(0xffffd447));
        g.fillPath(crown);
        g.setColour(juce::Colour(0xff8b6f16));
        g.strokePath(crown, juce::PathStrokeType(1.6f));

        g.setColour(juce::Colour(0xff2b221c));
        g.fillEllipse(portrait.getCentreX() - 22.0f, portrait.getY() + 28.0f, 6.0f, 6.0f);
        g.fillEllipse(portrait.getCentreX() + 16.0f, portrait.getY() + 28.0f, 6.0f, 6.0f);
        g.drawLine(portrait.getCentreX() - 12.0f, portrait.getY() + 48.0f,
                   portrait.getCentreX() + 12.0f, portrait.getY() + 48.0f, 1.4f);
    }
};

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

    auto redDot = juce::Rectangle<float>(5.6f, 5.6f).withCentre({ handle.getRight() - 9.0f, handle.getCentreY() - 0.5f });
    g.setColour(kLedRed.withAlpha(0.20f));
    g.fillEllipse(redDot.expanded(3.0f));
    g.setColour(juce::Colour(0xff5a1614));
    g.fillEllipse(redDot.expanded(0.8f));
    g.setColour(kLedRed);
    g.fillEllipse(redDot);
    g.setColour(juce::Colour(0xffffd0bd).withAlpha(0.58f));
    g.fillEllipse(redDot.reduced(1.9f).translated(-0.7f, -0.7f));
}

int seqPitchModPanelIndex(int parameterIndex)
{
    switch (parameterIndex)
    {
        case 2:  return 1; // VCO 2 is the centre position on the panel.
        case 1:  return 2; // OFF is the right position on the panel.
        default: return 0; // VCO 1&2 is the left position.
    }
}

int wavePanelIndex(int parameterIndex)
{
    return parameterIndex == 1 ? 0 : 1; // TRI is left on the panel; square is right.
}

juce::Path makeCablePath(juce::Point<float> src, juce::Point<float> dst, float slack = 1.0f, float lift = 0.0f)
{
    const auto dx = dst.x - src.x;
    const auto dy = dst.y - src.y;
    const auto distance = std::sqrt(dx * dx + dy * dy);
    const auto side = dx >= 0.0f ? 1.0f : -1.0f;
    const auto sag = juce::jlimit(18.0f, 92.0f, distance * 0.19f) * slack + lift;
    const auto bow = juce::jlimit(34.0f, 136.0f, std::abs(dx) * 0.48f + 32.0f);
    const auto verticalBias = juce::jlimit(-28.0f, 28.0f, dy * 0.16f);

    juce::Path cable;
    cable.startNewSubPath(src);
    cable.cubicTo(src.x + bow * side, src.y + sag + verticalBias,
                  dst.x - bow * side, dst.y + sag - verticalBias,
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
               juce::Colour colour, float alpha, bool preview, int visualIndex = 0)
{
    const float lift = preview ? 0.0f : (float)((visualIndex % 5) - 2) * 3.0f;
    auto cablePath = makeCablePath(src, dst, preview ? 0.72f : 1.0f, lift);
    const auto width = preview ? 4.2f : 6.2f;

    auto shadowPath = cablePath;
    shadowPath.applyTransform(juce::AffineTransform::translation(2.2f, 3.6f));
    g.setColour(juce::Colour(0x8a000000).withAlpha(alpha * 0.50f));
    g.strokePath(shadowPath, juce::PathStrokeType(width + 5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour(colour.darker(0.82f).withAlpha(alpha));
    g.strokePath(cablePath, juce::PathStrokeType(width + 2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour(colour.darker(0.12f).withAlpha(alpha));
    g.strokePath(cablePath, juce::PathStrokeType(width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    auto lowlightPath = cablePath;
    lowlightPath.applyTransform(juce::AffineTransform::translation(0.0f, 1.35f));
    g.setColour(juce::Colours::black.withAlpha(alpha * 0.18f));
    g.strokePath(lowlightPath, juce::PathStrokeType(width * 0.28f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    auto highlightPath = cablePath;
    highlightPath.applyTransform(juce::AffineTransform::translation(-0.65f, -1.45f));
    g.setColour(colour.brighter(0.86f).withAlpha(alpha * (preview ? 0.30f : 0.42f)));
    g.strokePath(highlightPath, juce::PathStrokeType(width * 0.16f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void drawCablePlug(juce::Graphics& g, juce::Point<float> centre, juce::Colour colour, bool active)
{
    auto collar = juce::Rectangle<float>(19.5f, 19.5f).withCentre(centre);
    g.setColour(juce::Colour(0x72000000));
    g.fillEllipse(collar.translated(1.4f, 2.1f));

    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff454545), collar.getX(), collar.getY(),
                                           juce::Colour(0xff101010), collar.getRight(), collar.getBottom(), false));
    g.fillEllipse(collar);
    g.setColour(juce::Colour(0xff050505).withAlpha(active ? 0.86f : 0.68f));
    g.drawEllipse(collar.reduced(0.55f), 1.1f);

    auto insert = collar.reduced(4.7f);
    g.setColour(colour.darker(0.48f).withAlpha(active ? 0.96f : 0.78f));
    g.fillEllipse(insert);
    g.setColour(colour.brighter(0.70f).withAlpha(active ? 0.30f : 0.20f));
    g.fillEllipse(insert.withTrimmedBottom(insert.getHeight() * 0.58f).reduced(0.9f, 0.0f));
    g.setColour(juce::Colours::white.withAlpha(active ? 0.16f : 0.09f));
    g.fillEllipse(insert.reduced(3.0f).translated(-1.4f, -1.6f));
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
    const auto centre = bounds.getCentre()
        + juce::Point<float>((float) slider.getProperties().getWithDefault("panelIndicatorCentreX", 0.0),
                             (float) slider.getProperties().getWithDefault("panelIndicatorCentreY", 0.0));

    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const float offset = (float) slider.getProperties().getWithDefault("panelIndicatorOffset", 0.0);
    float indicatorAngle = angle - juce::MathConstants<float>::halfPi + offset;
    if (slider.getProperties().getWithDefault("clockMultIndicator", false))
    {
        constexpr std::array<float, 9> clockMultDegrees {
             133.0f, 161.0f, -167.0f, -136.0f, -90.0f,
             -44.0f, -12.0f,   19.0f,   47.0f
        };
        const int index = juce::jlimit(0, 8, juce::roundToInt(slider.getValue()));
        indicatorAngle = juce::degreesToRadians(clockMultDegrees[(size_t) index]);
    }
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
    presetBox.setComponentID(kPanelControlId);
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

    presetPrevButton.onClick = [this]() { stepPreset(-1); };
    presetNextButton.onClick = [this]() { stepPreset(1); };
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

    presetPrevButton.setComponentID(kPanelControlId);
    presetNextButton.setComponentID(kPanelControlId);
    presetSaveButton.setComponentID(kPanelControlId);
    presetDeleteButton.setComponentID(kPanelControlId);
    presetInitButton.setComponentID(kPanelControlId);
    addAndMakeVisible(presetPrevButton);
    addAndMakeVisible(presetNextButton);
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
    seqPitchModBox.setInterceptsMouseClicks(false, false);
    seqPitchModBox.onChange = [this]() { repaint(); };
    addAndMakeVisible(seqPitchModBox);

    seqPitchModButton.setComponentID(kPanelControlId);
    seqPitchModButton.onClick = [this]()
    {
        static constexpr std::array<int, 3> panelOrderIds { 1, 3, 2 };
        const int currentId = seqPitchModBox.getSelectedId();
        const auto* current = std::find(panelOrderIds.begin(), panelOrderIds.end(), currentId);
        const auto next = current == panelOrderIds.end() ? panelOrderIds.begin()
                                                         : std::next(current);
        seqPitchModBox.setSelectedId(next == panelOrderIds.end() ? panelOrderIds.front() : *next,
                                     juce::sendNotification);
    };
    addAndMakeVisible(seqPitchModButton);

    hardSyncBox.addItem("OFF", 1);
    hardSyncBox.addItem("ON", 2);
    hardSyncBox.setComponentID(kPanelControlId);
    hardSyncBox.setInterceptsMouseClicks(false, false);
    hardSyncBox.onChange = [this]() { repaint(); };
    addAndMakeVisible(hardSyncBox);

    hardSyncButton.setComponentID(kPanelControlId);
    hardSyncButton.onClick = [this]()
    {
        hardSyncBox.setSelectedId(hardSyncBox.getSelectedId() == 2 ? 1 : 2,
                                  juce::sendNotification);
    };
    addAndMakeVisible(hardSyncButton);

    vco1WaveBox.addItem("SQUARE", 1);
    vco1WaveBox.addItem("TRIANGLE", 2);
    vco1WaveBox.setComponentID(kPanelControlId);
    vco1WaveBox.setInterceptsMouseClicks(false, false);
    vco1WaveBox.onChange = [this]() { repaint(); };
    addAndMakeVisible(vco1WaveBox);

    vco1WaveButton.setComponentID(kPanelControlId);
    vco1WaveButton.onClick = [this]()
    {
        vco1WaveBox.setSelectedId(vco1WaveBox.getSelectedId() == 2 ? 1 : 2,
                                  juce::sendNotification);
    };
    addAndMakeVisible(vco1WaveButton);

    vco2WaveBox.addItem("SQUARE", 1);
    vco2WaveBox.addItem("TRIANGLE", 2);
    vco2WaveBox.setComponentID(kPanelControlId);
    vco2WaveBox.setInterceptsMouseClicks(false, false);
    vco2WaveBox.onChange = [this]() { repaint(); };
    addAndMakeVisible(vco2WaveBox);

    vco2WaveButton.setComponentID(kPanelControlId);
    vco2WaveButton.onClick = [this]()
    {
        vco2WaveBox.setSelectedId(vco2WaveBox.getSelectedId() == 2 ? 1 : 2,
                                  juce::sendNotification);
    };
    addAndMakeVisible(vco2WaveButton);

    vcfModeBox.addItem("LP", 1);
    vcfModeBox.addItem("HP", 2);
    vcfModeBox.setComponentID(kPanelControlId);
    vcfModeBox.setInterceptsMouseClicks(false, false);
    vcfModeBox.onChange = [this]() { repaint(); };
    addAndMakeVisible(vcfModeBox);

    vcfModeButton.setComponentID(kPanelControlId);
    vcfModeButton.onClick = [this]()
    {
        vcfModeBox.setSelectedId(vcfModeBox.getSelectedId() == 2 ? 1 : 2,
                                 juce::sendNotification);
    };
    addAndMakeVisible(vcfModeButton);

    setupKnob(clockMult);
    clockMult.setRange(0.0, 8.0, 1.0);
    clockMult.setDoubleClickReturnValue(true, 4.0);
    clockMult.setMouseDragSensitivity(180);
    clockMult.getProperties().set("panelIndicatorOuter", 0.33);
    clockMult.getProperties().set("clockMultIndicator", true);
    addAndMakeVisible(clockMult);

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
    clockMultAtt = std::make_unique<SliderAttachment>(apvts, "clockMult", clockMult);

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
    repaint();
}

void DFAFEditor::promptSavePreset()
{
    const auto currentPreset = processor.getCurrentPresetName();
    if (currentPreset.isNotEmpty()
        && currentPreset != "Init"
        && processor.getAvailablePresetNames().contains(currentPreset))
    {
        presetSaveChoiceWindow.reset();
        presetSaveChoiceArt.reset();

        presetSaveChoiceArt = std::make_unique<RoyalCatSaveArt>();
        presetSaveChoiceWindow = std::make_unique<juce::AlertWindow>("Save DFAF Preset",
                                                                     "Save changes to \"" + currentPreset + "\"?",
                                                                     juce::AlertWindow::NoIcon,
                                                                     this);
        presetSaveChoiceWindow->addCustomComponent(presetSaveChoiceArt.get());
        presetSaveChoiceWindow->addButton("Overwrite", 1);
        presetSaveChoiceWindow->addButton("Save As...", 2);
        presetSaveChoiceWindow->addButton("Cancel", 0);

        juce::Component::SafePointer<DFAFEditor> safeThis(this);
        presetSaveChoiceWindow->enterModalState(true,
                                                juce::ModalCallbackFunction::create([safeThis, currentPreset](int result)
                                                {
                                                    if (safeThis == nullptr)
                                                        return;

                                                    auto* editor = safeThis.getComponent();
                                                    editor->presetSaveChoiceWindow.reset();
                                                    editor->presetSaveChoiceArt.reset();

                                                    if (result == 1)
                                                    {
                                                        editor->processor.saveCurrentPreset();
                                                        editor->refreshPresetControls();
                                                    }
                                                    else if (result == 2)
                                                    {
                                                        editor->showSavePresetChooser(currentPreset + " Copy");
                                                    }
                                                }),
                                                false);

        return;
    }

    showSavePresetChooser(currentPreset == "Init" ? juce::String("New Preset") : currentPreset);
}

void DFAFEditor::showSavePresetChooser(const juce::String& suggestedName)
{
    auto presetDirectory = getDefaultPresetDirectory();
    presetDirectory.createDirectory();

    const auto fallbackName = suggestedName.isNotEmpty() ? suggestedName : juce::String("New Preset");
    const auto suggestedFile = presetDirectory.getChildFile(fallbackName + ".dfafpreset");

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

void DFAFEditor::stepPreset(int direction)
{
    const auto presetNames = processor.getAvailablePresetNames();
    const int numPresetSlots = presetNames.size() + 1;
    if (numPresetSlots <= 0)
        return;

    int currentIndex = 0;
    const auto currentPreset = processor.getCurrentPresetName();
    if (currentPreset.isNotEmpty() && currentPreset != "Init")
    {
        const int userIndex = presetNames.indexOf(currentPreset);
        if (userIndex >= 0)
            currentIndex = userIndex + 1;
    }

    const int nextIndex = (currentIndex + direction + numPresetSlots) % numPresetSlots;
    presetBox.setSelectedId(nextIndex == 0 ? kPresetInitId : kPresetFirstUserId + nextIndex - 1,
                            juce::sendNotification);
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
    return mapPanelRect(1414.0f, 86.0f, 195.0f, 781.0f);
}

juce::Point<int> DFAFEditor::getJackCentre(PatchPoint pp) const
{
    const auto inputs = inputOrder();
    const auto outputs = outputOrder();
    constexpr std::array<float, 7> ys { 183.0f, 278.0f, 373.0f, 463.0f, 560.0f, 659.0f, 758.0f };
    constexpr float inX = 1486.0f;
    constexpr float outX = 1577.0f;

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
        drawCable(g, src, dst, cableColour, 0.96f, false, cableIndex);
    }

    cableIndex = 0;
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
    s.getProperties().set("panelIndicatorInner", 0.0);
    s.getProperties().set("panelIndicatorOuter", small ? 0.25 : 0.27);
    s.getProperties().set("panelIndicatorWidth", small ? 2.2 : 2.6);
    s.onValueChange = [this]() { repaint(); };
    if (small)
        s.setScrollWheelEnabled(false);
}

void DFAFEditor::drawPanelSwitches(juce::Graphics& g) const
{
    const int seqPitchModIndex = seqPitchModPanelIndex(seqPitchModBox.getSelectedItemIndex());
    drawPanelToggle(g, mapPanelRect(222.0f, 143.0f, 86.0f, 37.0f).toFloat(),
                    seqPitchModIndex, 3);

    drawPanelToggle(g, mapPanelRect(645.0f, 142.0f, 57.0f, 36.0f).toFloat(),
                    wavePanelIndex(vco1WaveBox.getSelectedItemIndex()), 2);
    drawPanelToggle(g, mapPanelRect(986.0f, 143.0f, 52.0f, 35.0f).toFloat(),
                    juce::jlimit(0, 1, vcfModeBox.getSelectedItemIndex()), 2);

    drawPanelToggle(g, mapPanelRect(234.0f, 383.0f, 60.0f, 36.0f).toFloat(),
                    juce::jlimit(0, 1, hardSyncBox.getSelectedItemIndex()), 2);

    drawPanelToggle(g, mapPanelRect(644.0f, 384.0f, 57.0f, 35.0f).toFloat(),
                    wavePanelIndex(vco2WaveBox.getSelectedItemIndex()), 2);

    if (resetLedActive)
    {
        auto led = mapPanelRect(171.0f, 729.0f, 9.0f, 9.0f).toFloat();
        g.setColour(kLedRed.withAlpha(0.28f));
        g.fillEllipse(led.expanded(5.0f));
        g.setColour(kLedRed);
        g.fillEllipse(led);
    }
}

void DFAFEditor::drawPresetDisplay(juce::Graphics& g) const
{
    auto screenArea = mapPanelRect(175.0f, 830.0f, 329.0f, 45.0f).toFloat();
    auto textArea = mapPanelRect(199.0f, 836.0f, 282.0f, 32.0f).toFloat();
    auto selectedId = presetBox.getSelectedId();
    if (selectedId == 0)
        selectedId = kPresetInitId;

    const int displayNumber = selectedId == kPresetMissingId ? 999 : juce::jlimit(1, 999, selectedId);
    auto name = presetBox.getText().trim();
    if (name.isEmpty())
        name = "INIT";

    auto displayText = juce::String(displayNumber).paddedLeft('0', 3)
        + "  "
        + name.replaceCharacter('*', ' ').trim().toUpperCase();

    const auto fontHeight = textArea.getHeight() * 0.64f;
    auto font = juce::Font(juce::FontOptions("Menlo", fontHeight, juce::Font::plain))
        .withHorizontalScale(0.94f);

    g.setColour(juce::Colour(0xff0a0b0a));
    g.fillRoundedRectangle(screenArea, 2.0f);
    g.setColour(juce::Colour(0xff151310));
    g.fillRoundedRectangle(screenArea.reduced(3.0f, 3.0f), 1.2f);
    g.setColour(juce::Colour(0x55000000));
    g.fillRoundedRectangle(screenArea.withTrimmedTop(screenArea.getHeight() * 0.52f).reduced(3.0f, 1.0f), 1.0f);

    g.saveState();
    g.reduceClipRegion(screenArea.toNearestInt());

    g.setFont(font);

    for (int i = 0; i < 4; ++i)
    {
        const auto y = screenArea.getY() + 6.0f + (float) i * 7.0f;
        g.setColour(juce::Colour(0x22000000));
        g.drawLine(screenArea.getX() + 4.0f, y, screenArea.getRight() - 4.0f, y, 1.0f);
    }

    g.setColour(juce::Colour(0xffff1f18).withAlpha(0.20f));
    g.drawText(displayText, textArea.translated(-0.8f, 0.0f), juce::Justification::centredLeft, false);
    g.drawText(displayText, textArea.translated(0.8f, 0.0f), juce::Justification::centredLeft, false);
    g.setColour(juce::Colour(0xffff3a2f).withAlpha(0.94f));
    g.drawText(displayText, textArea, juce::Justification::centredLeft, false);
    g.setColour(juce::Colour(0xffff8a78).withAlpha(0.34f));
    g.drawText(displayText, textArea.translated(0.0f, -0.7f), juce::Justification::centredLeft, false);

    g.restoreState();
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
    drawPresetDisplay(g);

    auto drawActiveLed = [this, &g](float cx, float cy)
    {
        const auto led = mapPanelRect(cx - 4.5f, cy - 4.5f, 9.0f, 9.0f).toFloat();
        g.setColour(kLedRed.withAlpha(0.28f));
        g.fillEllipse(led.expanded(5.0f));
        g.setColour(kLedRed);
        g.fillEllipse(led);
        g.setColour(juce::Colour(0xffffd0bd).withAlpha(0.65f));
        g.fillEllipse(led.reduced(2.7f).translated(-0.8f, -0.8f));
    };

    constexpr std::array<juce::Point<float>, 9> clockMultLedCentres {
        juce::Point<float> { 130.3f, 704.5f }, // 1/5
        juce::Point<float> { 116.6f, 676.4f }, // 1/4
        juce::Point<float> { 115.9f, 642.0f }, // 1/3
        juce::Point<float> { 131.5f, 613.0f }, // 1/2
        juce::Point<float> { 175.9f, 601.3f }, // 1x
        juce::Point<float> { 220.8f, 612.8f }, // 2x
        juce::Point<float> { 237.4f, 643.3f }, // 3x
        juce::Point<float> { 236.8f, 677.4f }, // 4x
        juce::Point<float> { 221.5f, 704.8f }  // 8x
    };
    const int clockMultLedIndex = juce::jlimit(0, 8, juce::roundToInt(clockMult.getValue()));
    drawActiveLed(clockMultLedCentres[(size_t) clockMultLedIndex].x,
                  clockMultLedCentres[(size_t) clockMultLedIndex].y);

    for (int i = 0; i < 8; ++i)
    {
        constexpr std::array<juce::Point<float>, 8> stepLedCentres {
            juce::Point<float> { 433.1f, 591.3f },
            juce::Point<float> { 528.3f, 591.3f },
            juce::Point<float> { 621.5f, 591.3f },
            juce::Point<float> { 714.5f, 591.4f },
            juce::Point<float> { 806.5f, 591.3f },
            juce::Point<float> { 900.6f, 591.0f },
            juce::Point<float> { 995.2f, 591.6f },
            juce::Point<float> { 1089.4f, 591.2f }
        };
        if (i == currentLedStep)
            drawActiveLed(stepLedCentres[(size_t) i].x, stepLedCentres[(size_t) i].y);
    }

    std::vector<PatchCable> cables;
    processor.getCableSnapshot(cables);
    drawJackPanel(g, getPatchAreaBounds(), cables, pendingOut);
}

void DFAFEditor::resized()
{
    const auto panelScale = getPanelImageBounds().getWidth() / kPanelSourceWidth;

    auto setKnobCentre = [this](juce::Slider& slider, float cx, float cy, float size)
    {
        slider.setBounds(mapPanelRect(cx - size * 0.5f, cy - size * 0.5f, size, size));
    };

    auto setIndicatorCentre = [panelScale](juce::Slider& slider, float dx, float dy)
    {
        slider.getProperties().set("panelIndicatorCentreX", dx * panelScale);
        slider.getProperties().set("panelIndicatorCentreY", dy * panelScale);
    };

    setKnobCentre(vcoDecay, 132.5f, 160.0f, 62.0f);
    setIndicatorCentre(vcoDecay, -2.0f, -3.5f);
    setKnobCentre(vco1EgAmount, 399.0f, 159.5f, 62.0f);
    setIndicatorCentre(vco1EgAmount, 2.0f, -3.0f);
    setKnobCentre(vco1Frequency, 543.0f, 161.0f, 82.0f);
    setIndicatorCentre(vco1Frequency, 0.0f, -2.0f);
    setKnobCentre(vco1Level, 785.0f, 159.0f, 60.0f);
    setIndicatorCentre(vco1Level, 2.0f, -2.5f);
    setKnobCentre(noiseLevel, 905.0f, 160.5f, 60.0f);
    setIndicatorCentre(noiseLevel, 1.0f, -4.0f);
    setKnobCentre(cutoff, 1134.0f, 165.0f, 78.0f);
    setIndicatorCentre(cutoff, 0.0f, -2.0f);
    setKnobCentre(resonance, 1252.0f, 160.5f, 62.0f);
    setIndicatorCentre(resonance, -1.5f, -3.5f);
    setKnobCentre(vcaEg, 1381.5f, 160.5f, 62.0f);
    setIndicatorCentre(vcaEg, -1.0f, -4.5f);

    setKnobCentre(fmAmount, 130.0f, 400.5f, 62.0f);
    setIndicatorCentre(fmAmount, 0.0f, -4.0f);
    setKnobCentre(vco2EgAmount, 399.5f, 400.0f, 62.0f);
    setIndicatorCentre(vco2EgAmount, 0.0f, -3.5f);
    setKnobCentre(vco2Frequency, 543.5f, 401.5f, 82.0f);
    setIndicatorCentre(vco2Frequency, 0.0f, -2.0f);
    setKnobCentre(vco2Level, 783.5f, 401.0f, 60.0f);
    setIndicatorCentre(vco2Level, -0.5f, -5.0f);
    setKnobCentre(vcfDecay, 960.0f, 401.0f, 62.0f);
    setIndicatorCentre(vcfDecay, -1.0f, -5.0f);
    setKnobCentre(vcfEgAmount, 1096.5f, 400.5f, 62.0f);
    setIndicatorCentre(vcfEgAmount, -0.5f, -4.5f);
    setKnobCentre(noiseVcfMod, 1230.5f, 401.0f, 62.0f);
    setIndicatorCentre(noiseVcfMod, -0.5f, -5.0f);
    setKnobCentre(vcaDecay, 1381.5f, 313.5f, 62.0f);
    setIndicatorCentre(vcaDecay, -1.5f, 0.5f);

    setKnobCentre(volume, 1381.5f, 458.0f, 62.0f);
    setIndicatorCentre(volume, -1.0f, -1.0f);
    setKnobCentre(clockMult, 176.0f, 656.5f, 88.0f);
    setIndicatorCentre(clockMult, 0.0f, -11.5f);
    resetButton.setBounds(mapPanelRect(151.0f, 747.0f, 49.0f, 37.0f));

    seqPitchModBox.setBounds(mapPanelRect(222.0f, 143.0f, 86.0f, 76.0f));
    seqPitchModButton.setBounds(seqPitchModBox.getBounds());
    vco1WaveBox.setBounds(mapPanelRect(645.0f, 142.0f, 57.0f, 83.0f));
    vco1WaveButton.setBounds(vco1WaveBox.getBounds());
    vcfModeBox.setBounds(mapPanelRect(986.0f, 143.0f, 52.0f, 83.0f));
    vcfModeButton.setBounds(vcfModeBox.getBounds());
    hardSyncBox.setBounds(mapPanelRect(234.0f, 383.0f, 60.0f, 71.0f));
    hardSyncButton.setBounds(hardSyncBox.getBounds());
    vco2WaveBox.setBounds(mapPanelRect(644.0f, 384.0f, 57.0f, 76.0f));
    vco2WaveButton.setBounds(vco2WaveBox.getBounds());

    constexpr std::array<float, 8> stepPitchXs { 431.5f, 527.0f, 620.5f, 713.5f,
                                                 806.0f, 900.0f, 994.5f, 1089.5f };
    constexpr std::array<float, 8> stepVelocityXs { 431.0f, 527.0f, 620.0f, 713.0f,
                                                    805.0f, 900.0f, 994.5f, 1089.0f };
    constexpr std::array<float, 8> stepPitchIndicatorDx { -0.5f, -0.5f, -0.5f, -0.5f,
                                                          -0.5f, -0.5f, -0.5f, -0.5f };
    constexpr std::array<float, 8> stepPitchIndicatorDy { 0.0f, -0.5f, -0.5f, -1.0f,
                                                          -1.0f, -1.0f, -1.5f, -1.5f };
    constexpr std::array<float, 8> stepVelocityIndicatorDy { 1.5f, 1.0f, 1.0f, 0.5f,
                                                             -0.5f, -0.5f, -1.0f, -1.0f };
    for (int i = 0; i < 8; ++i)
    {
        setKnobCentre(stepPitch[i], stepPitchXs[(size_t) i], 631.5f, 50.0f);
        setKnobCentre(stepVelocity[i], stepVelocityXs[(size_t) i], 711.0f, 50.0f);
        setIndicatorCentre(stepPitch[i],
                           stepPitchIndicatorDx[(size_t) i],
                           stepPitchIndicatorDy[(size_t) i]);
        setIndicatorCentre(stepVelocity[i], 0.0f, stepVelocityIndicatorDy[(size_t) i]);
    }

    presetBox.setBounds(mapPanelRect(175.0f, 830.0f, 329.0f, 45.0f));
    presetPrevButton.setBounds(mapPanelRect(548.0f, 834.0f, 48.0f, 39.0f));
    presetNextButton.setBounds(mapPanelRect(616.0f, 834.0f, 48.0f, 39.0f));
    presetSaveButton.setBounds(mapPanelRect(706.0f, 834.0f, 55.0f, 39.0f));
    presetDeleteButton.setBounds(mapPanelRect(793.0f, 834.0f, 56.0f, 39.0f));
    presetInitButton.setBounds(mapPanelRect(883.0f, 834.0f, 56.0f, 39.0f));
}
