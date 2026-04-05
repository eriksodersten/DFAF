#include "PluginEditor.h"

static const juce::Colour panelWhite = juce::Colour(0xfff0ede8);
static const juce::Colour labelBlack = juce::Colour(0xff111111);

DFAFEditor::DFAFEditor(DFAFProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(1400, 480);
    setLookAndFeel(&laf);
    startTimerHz(30);

    resetButton.setButtonText("RESET");
    resetButton.onClick = [this]() {
            processor.resetSequencer();
            currentLedStep = 0;
            resetLedActive = true;
            repaint();
        };
    addAndMakeVisible(resetButton);

    auto add = [this](juce::Slider& s, bool small = false) {
        setupKnob(s, small);
        addAndMakeVisible(s);
    };

    add(vcoDecay); add(vco1EgAmount); add(vco1Frequency);
        seqPitchModBox.addItem("VCO 1&2", 1);
        seqPitchModBox.addItem("OFF",     2);
        seqPitchModBox.addItem("VCO 2",   3);
        seqPitchModBox.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(seqPitchModBox);

    hardSyncBox.addItem("ON",  1);
        hardSyncBox.addItem("OFF", 2);
        hardSyncBox.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(hardSyncBox);

    vco1WaveBox.addItem("Square",   1);
        vco1WaveBox.addItem("Triangle", 2);
        vco1WaveBox.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(vco1WaveBox);

    vco2WaveBox.addItem("Square",   1);
            vco2WaveBox.addItem("Triangle", 2);
            vco2WaveBox.setJustificationType(juce::Justification::centred);
            addAndMakeVisible(vco2WaveBox);
        vcfModeBox.addItem("LP", 1);
        vcfModeBox.addItem("HP", 2);
        vcfModeBox.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(vcfModeBox);
    
    add(vco1Level, true); add(noiseLevel, true); add(cutoff); add(resonance); add(vcaEg); add(volume);
    add(fmAmount); add(vco2EgAmount); add(vco2Frequency);
    add(vco2Level, true); add(vcfDecay); add(vcfEgAmount); add(noiseVcfMod); add(vcaDecay);    clockMultBox.addItem("1/8", 1);
        clockMultBox.addItem("1/5", 2);
        clockMultBox.addItem("1/4", 3);
        clockMultBox.addItem("1/3", 4);
        clockMultBox.addItem("1/2", 5);
        clockMultBox.addItem("1x",  6);
    clockMultBox.addItem("2x",  7);
        clockMultBox.addItem("3x",  8);
        clockMultBox.addItem("4x",  9);
        clockMultBox.addItem("5x",  10);
        clockMultBox.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(clockMultBox);
    for (int i = 0; i < 8; ++i) { add(stepPitch[i], true); add(stepVelocity[i], true); }

    auto& apvts = p.apvts;
    vcoDecayAtt       = std::make_unique<SliderAttachment>(apvts, "vcoDecay", vcoDecay);
    seqPitchModBoxAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, "seqPitchMod", seqPitchModBox);
    hardSyncBoxAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, "hardSync", hardSyncBox);
        vco1WaveBoxAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, "vco1Wave", vco1WaveBox);
    vco2WaveBoxAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, "vco2Wave", vco2WaveBox);
    vcfModeBoxAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, "vcfMode", vcfModeBox);
    vco1FreqAtt    = std::make_unique<SliderAttachment>(apvts, "vco1Freq",    vco1Frequency);
    vco1EgAmtAtt   = std::make_unique<SliderAttachment>(apvts, "vco1EgAmt",   vco1EgAmount);
    fmAmountAtt    = std::make_unique<SliderAttachment>(apvts, "fmAmount",    fmAmount);
    vco2FreqAtt    = std::make_unique<SliderAttachment>(apvts, "vco2Freq",    vco2Frequency);
    vco2EgAmtAtt   = std::make_unique<SliderAttachment>(apvts, "vco2EgAmt",   vco2EgAmount);
    noiseLevelAtt  = std::make_unique<SliderAttachment>(apvts, "noiseLevel",  noiseLevel);
        vco1LevelAtt   = std::make_unique<SliderAttachment>(apvts, "vco1Level",   vco1Level);
        vco2LevelAtt   = std::make_unique<SliderAttachment>(apvts, "vco2Level",   vco2Level);
    cutoffAtt      = std::make_unique<SliderAttachment>(apvts, "cutoff",      cutoff);
    resonanceAtt   = std::make_unique<SliderAttachment>(apvts, "resonance",   resonance);
    vcfDecayAtt    = std::make_unique<SliderAttachment>(apvts, "vcfDecay",    vcfDecay);
    vcfEgAmtAtt    = std::make_unique<SliderAttachment>(apvts, "vcfEgAmt",    vcfEgAmount);
    noiseVcfModAtt = std::make_unique<SliderAttachment>(apvts, "noiseVcfMod", noiseVcfMod);
    vcaDecayAtt    = std::make_unique<SliderAttachment>(apvts, "vcaDecay",    vcaDecay);
    vcaEgAtt       = std::make_unique<SliderAttachment>(apvts, "vcaEg",       vcaEg);
    volumeAtt      = std::make_unique<SliderAttachment>(apvts, "volume",      volume);
    clockMultBoxAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                apvts, "clockMult", clockMultBox);

        for (int i = 0; i < 8; ++i)
        {
            stepPitchAtt[i] = std::make_unique<SliderAttachment>(apvts, "stepPitch" + juce::String(i), stepPitch[i]);
            stepVelAtt[i]   = std::make_unique<SliderAttachment>(apvts, "stepVel"   + juce::String(i), stepVelocity[i]);
        }
}

DFAFEditor::~DFAFEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

// =============================================================================
// Patch GUI helpers
// =============================================================================

juce::Point<int> DFAFEditor::getJackCentre(PatchPoint pp) const
{
    // Must match drawJackPanel() constants exactly
    const int W     = getWidth();
    const int wood  = 18;
    const int jackW = 200;
    const int px    = W - wood - jackW;   // jack panel left edge
    const int py    = 0;                  // jack panel top edge

    const int col1   = px + 32;
    const int col2   = px + 100;
    const int col3   = px + 168;
    const int startY = py + 34;
    const int stride = 36;

    // ioRows layout (6 rows × 3 cols):  col1=IN  col2=IN  col3=OUT
    // r=0 col2 → VCA CV    (PP_VCA_CV)
    // r=1 col1 → VELOCITY  (PP_VELOCITY)
    // r=1 col3 → VCA EG    (PP_VCA_EG)
    // r=2 col3 → VCF EG    (PP_VCF_EG)
    // r=4 col1 → VCF MOD   (PP_VCF_MOD)
    switch (pp)
    {
        case PP_VCF_EG:   return { col3, startY + 2 * stride };
        case PP_VCF_MOD:  return { col1, startY + 4 * stride };
        case PP_VCA_EG:   return { col3, startY + 1 * stride };
        case PP_VCA_CV:   return { col2, startY + 0 * stride };
        case PP_VELOCITY: return { col1, startY + 1 * stride };
        default:          return { -1, -1 };
    }
}

PatchPoint DFAFEditor::jackHitTest(juce::Point<int> pos) const
{
    const int hitR2 = 14 * 14;   // hit radius² (slightly larger than jack radius 10)
    for (int p = 0; p < PP_NUM_POINTS; ++p)
    {
        auto pp = static_cast<PatchPoint>(p);
        auto  c = getJackCentre(pp);
        if (c.x < 0) continue;
        const int dx = pos.x - c.x, dy = pos.y - c.y;
        if (dx * dx + dy * dy <= hitR2)
            return pp;
    }
    return PP_NUM_POINTS;
}

void DFAFEditor::mouseDown(const juce::MouseEvent& e)
{
    PatchPoint hit = jackHitTest(e.getPosition());

    if (hit == PP_NUM_POINTS)                          // empty area → deselect
    {
        pendingOut = PP_NUM_POINTS;
        repaint();
        return;
    }

    if (kPatchMeta[hit].dir == PD_Out)                 // clicked an OUT jack
    {
        pendingOut = (pendingOut == hit) ? PP_NUM_POINTS : hit;
        repaint();
        return;
    }

    // Clicked an IN jack — need a pending OUT
    if (pendingOut == PP_NUM_POINTS) return;

    // Toggle: connect if not connected, disconnect if already connected
    std::vector<PatchCable> cables;
    processor.getCableSnapshot(cables);
    bool alreadyConnected = false;
    for (const auto& c : cables)
        if (c.src == pendingOut && c.dst == hit) { alreadyConnected = true; break; }

    if (alreadyConnected)
        processor.disconnectPatch(pendingOut, hit);
    else
        processor.connectPatch(pendingOut, hit, 1.0f);

    pendingOut = PP_NUM_POINTS;
    repaint();
}

// =============================================================================

void DFAFEditor::timerCallback()
{
    int step = processor.getCurrentStep();
    if (step >= 0)
        resetLedActive = false;

    int displayStep = resetLedActive ? 0 : step;
    if (displayStep != currentLedStep)
    {
        currentLedStep = displayStep;
        repaint();
    }
}

void DFAFEditor::setupKnob(juce::Slider& s, bool small)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    s.setRange(0.0, 1.0);
    s.setValue(0.5);
    juce::ignoreUnused(small);
}

void DFAFEditor::drawSwitch(juce::Graphics& g, float x, float y, float w, float h,
                             const juce::String& label, const juce::StringArray& options) const
{
    g.setColour(labelBlack);
    g.setFont(juce::FontOptions(7.0f).withStyle("Bold"));
    if (label.isNotEmpty())
        g.drawText(label, (int)x, (int)y, (int)w, 10, juce::Justification::centred);

    float bx = x + w * 0.5f - 10.0f;
    float by = y + 14.0f;
    float bw = 20.0f;
    float bh = h - 26.0f;

    g.setColour(juce::Colour(0xff888888));
    g.fillRoundedRectangle(bx, by, bw, bh, 4.0f);
    g.setColour(juce::Colour(0xff444444));
    g.drawRoundedRectangle(bx, by, bw, bh, 4.0f, 1.0f);

    float knobY = by + bh * 0.5f - 6.0f;
    g.setColour(juce::Colour(0xffdddddd));
    g.fillRoundedRectangle(bx + 2.0f, knobY, bw - 4.0f, 12.0f, 3.0f);

    g.setColour(labelBlack);
    g.setFont(juce::FontOptions(6.5f));
    if (options.size() >= 1)
        g.drawText(options[0], (int)(bx + bw + 3), (int)(by), 32, 9, juce::Justification::centredLeft);
    if (options.size() >= 2)
        g.drawText(options[options.size()-1], (int)(bx + bw + 3), (int)(by + bh - 9), 32, 9, juce::Justification::centredLeft);
}

void DFAFEditor::drawButton(juce::Graphics& g, float x, float y, float r,
                             const juce::String& label, bool red) const
{
    g.setColour(red ? juce::Colour(0xffcc2200) : juce::Colour(0xff222222));
    g.fillEllipse(x - r, y - r, r * 2, r * 2);
    g.setColour(juce::Colour(0xff555555));
    g.drawEllipse(x - r, y - r, r * 2, r * 2, 1.5f);
    g.setColour(labelBlack);
    g.setFont(juce::FontOptions(7.0f).withStyle("Bold"));
    g.drawText(label, (int)(x - 28), (int)(y + r + 3), 56, 9, juce::Justification::centred);
}

void DFAFEditor::drawJackPanel(juce::Graphics& g, int x, int y, int w, int h,
                               const std::vector<PatchCable>& cables,
                               PatchPoint selectedOut) const
{
    g.setColour(juce::Colour(0xffe8e5e0));
    g.fillRect(x, y, w, h);

    g.setColour(juce::Colour(0xffbbbbbb));
    g.drawRect(x, y, w, h, 1);

    auto drawJack = [&](int jx, int jy, const juce::String& lbl)
    {
        g.setColour(juce::Colour(0xff333333));
        g.fillEllipse((float)(jx - 10), (float)(jy - 10), 20.0f, 20.0f);

        g.setColour(juce::Colour(0xff666666));
        g.drawEllipse((float)(jx - 10), (float)(jy - 10), 20.0f, 20.0f, 1.5f);

        g.setColour(juce::Colour(0xff999999));
        g.fillEllipse((float)(jx - 4), (float)(jy - 4), 8.0f, 8.0f);

        g.setColour(labelBlack);
        g.setFont(juce::FontOptions(7.0f));
        g.drawText(lbl, jx - 24, jy + 12, 48, 9, juce::Justification::centred);
    };

    struct JackDef { const char* label; };

    JackDef ioRows[] = {
        {"TRIGGER"},  {"VCA CV"},  {"VCA"},
        {"VELOCITY"}, {"VCA DEC"}, {"VCA EG"},
        {"EXT AUD"},  {"VCF DEC"}, {"VCF EG"},
        {"NOISE LV"}, {"VCO DEC"}, {"VCO EG"},
        {"VCF MOD"},  {"VCO1 CV"}, {"VCO 1"},
        {"1-2 FAMT"}, {"VCO2 CV"}, {"VCO 2"}
    };

    JackDef outRow[] = {
        {"TRIGGER"}, {"VELOCTY"}, {"PITCH"}
    };

    const int col1 = x + 32;
    const int col2 = x + 100;
    const int col3 = x + 168;

    const int startY = y + 34;
    const int stride = 36;

    g.setColour(labelBlack);
    g.setFont(juce::FontOptions(7.5f).withStyle("Bold"));

    g.drawText("IN",  col1 - 18, y + 5, 36, 11, juce::Justification::centred);
    g.drawText("IN",  col2 - 18, y + 5, 36, 11, juce::Justification::centred);
    g.drawText("OUT", col3 - 20, y + 5, 40, 11, juce::Justification::centred);

    g.setColour(juce::Colour(0xffc7c3bd));
    g.drawLine((float)(x + 4), (float)(y + 19), (float)(x + w - 4), (float)(y + 19), 1.0f);

    for (int r = 0; r < 6; ++r)
    {
        const int idx = r * 3;
        const int jy = startY + r * stride;

        drawJack(col1, jy, ioRows[idx].label);
        drawJack(col2, jy, ioRows[idx + 1].label);
        drawJack(col3, jy, ioRows[idx + 2].label);
    }

    // Linje efter sista IN/OUT-raden (under label-texten på rad 5)
    const int line1Y = startY + 5 * stride + 26;
    g.setColour(juce::Colour(0xffc7c3bd));
    g.drawLine((float)(x + 4), (float)line1Y, (float)(x + w - 4), (float)line1Y, 1.0f);

    // OUT-rubrik centrerad mellan linje och OUT-jacks
    g.setColour(labelBlack);
    g.setFont(juce::FontOptions(7.5f).withStyle("Bold"));
    g.drawText("OUT", x, line1Y + 4, w, 11, juce::Justification::centred);

    // Linje under OUT-rubriken, ovanför jack-kroppar
    const int line2Y = line1Y + 18;
    g.setColour(juce::Colour(0xffc7c3bd));
    g.drawLine((float)(x + 4), (float)line2Y, (float)(x + w - 4), (float)line2Y, 1.0f);

    // OUT-jacks: centrum minst 14px under line2 (jack-topp vid outY-10 > line2Y)
    const int outY = line2Y + 24;
    drawJack(col1, outY, outRow[0].label);
    drawJack(col2, outY, outRow[1].label);
    drawJack(col3, outY, outRow[2].label);

    // --- Patch overlay ----------------------------------------------------
    // Active cables: draw line + green ring on destination
    for (const auto& cable : cables)
    {
        if (!cable.enabled) continue;
        auto src = getJackCentre(cable.src);
        auto dst = getJackCentre(cable.dst);
        if (src.x < 0 || dst.x < 0) continue;

        g.setColour(juce::Colour(0xcc44aaff));   // blue cable
        g.drawLine((float)src.x, (float)src.y,
                   (float)dst.x, (float)dst.y, 1.5f);

        // Green ring on IN jack
        g.setColour(juce::Colour(0xff44ee66));
        g.drawEllipse((float)(dst.x - 12), (float)(dst.y - 12), 24.0f, 24.0f, 1.5f);
    }

    // Selected OUT jack: yellow ring + highlight all available IN jacks in green
    if (selectedOut != PP_NUM_POINTS)
    {
        // Yellow ring on the selected OUT
        auto c = getJackCentre(selectedOut);
        if (c.x >= 0)
        {
            g.setColour(juce::Colour(0xffffcc00));
            g.drawEllipse((float)(c.x - 12), (float)(c.y - 12), 24.0f, 24.0f, 2.0f);
        }

        // Green ring on every available IN jack
        for (int p = 0; p < PP_NUM_POINTS; ++p)
        {
            if (kPatchMeta[p].dir != PD_In) continue;
            auto in = getJackCentre(static_cast<PatchPoint>(p));
            if (in.x < 0) continue;
            g.setColour(juce::Colour(0xff44ee66));
            g.drawEllipse((float)(in.x - 12), (float)(in.y - 12), 24.0f, 24.0f, 1.5f);
        }
    }
    // ----------------------------------------------------------------------
}

void DFAFEditor::paint(juce::Graphics& g)
{
    const int W     = getWidth();
    const int H     = getHeight();
    const int wood  = 18;
    const int jackW = 200;

    // Wood panels
    juce::ColourGradient woodGrad(
        juce::Colour(0xffa07820), 0, 0,
        juce::Colour(0xff6b4c10), 0, (float)H, false);
    g.setGradientFill(woodGrad);
    g.fillRect(0, 0, wood, H);
    g.fillRect(W - wood, 0, wood, H);
    g.setColour(juce::Colour(0x22000000));
    for (int i = 2; i < H; i += 5)
    {
        g.drawLine(1.0f, (float)i, (float)(wood-1), (float)i, 0.5f);
        g.drawLine((float)(W-wood+1), (float)i, (float)(W-1), (float)i, 0.5f);
    }

    // Main panel
    g.setColour(panelWhite);
    g.fillRect(wood, 0, W - wood * 2, H);
    g.setColour(juce::Colour(0xff888888));
    g.drawRect(wood, 0, W - wood * 2, H, 1);

    // Jack panel
    std::vector<PatchCable> cables;
    processor.getCableSnapshot(cables);
    drawJackPanel(g, W - wood - jackW, 0, jackW, H, cables, pendingOut);

    // Screws
    auto drawScrew = [&](float sx, float sy) {
        g.setColour(juce::Colour(0xff999999));
        g.fillEllipse(sx-6, sy-6, 12, 12);
        g.setColour(juce::Colour(0xff444444));
        g.drawEllipse(sx-6, sy-6, 12, 12, 1.0f);
        g.setColour(juce::Colour(0xff222222));
        g.drawLine(sx-3, sy, sx+3, sy, 1.2f);
        g.drawLine(sx, sy-3, sx, sy+3, 1.2f);
    };
    drawScrew((float)(wood+14), 14.0f);
    drawScrew((float)(W-wood-jackW-14), 14.0f);
    drawScrew((float)(wood+14), (float)(H-14));
    drawScrew((float)(W-wood-jackW-14), (float)(H-14));

    // Separator lines
    g.setColour(juce::Colour(0xffbbbbbb));
    g.drawLine((float)wood, 160.0f, (float)(W-wood-jackW), 160.0f, 0.8f);
    g.drawLine((float)wood, 308.0f, (float)(W-wood-jackW), 308.0f, 0.8f);

    const int mainW = W - wood * 2 - jackW;
    const int kS    = (mainW - 60) / 11;
    const int offX  = wood + 30;

    g.setColour(labelBlack);
    g.setFont(juce::FontOptions(7.5f).withStyle("Bold"));

    // Row 1 labels
    const char* top[] = {
            "VCO DECAY","SEQ PITCH MOD","VCO 1 EG AMT","VCO 1 FREQ",
            "VCO 1 WAVE","VCO 1 LEVEL","NOISE/EXT LVL","CUTOFF","RESONANCE","VCA EG AMT","VOLUME"
        };
    for (int i = 0; i < 11; ++i)
        g.drawText(top[i], offX + i * kS, 8, kS, 10, juce::Justification::centred);

    // Row 2 labels
    const char* bot[] = {
                "1-2 FM AMT","HARD SYNC","VCO 2 EG AMT","VCO 2 FREQ",
                "VCO 2 WAVE","VCO 2 LEVEL","VCF DECAY","VCF EG AMT","NOISE/VCF MOD","","VCA DECAY"
            };
    for (int i = 0; i < 11; ++i)
        g.drawText(bot[i], offX + i * kS, 167, kS, 10, juce::Justification::centred);

    // Sequencer labels
    g.setFont(juce::FontOptions(7.5f).withStyle("Bold"));
    g.drawText("TEMPO",    wood+30, 314, 90, 10, juce::Justification::centred);
    g.drawText("PITCH",    wood+30, 335, 90, 10, juce::Justification::centred);
    g.drawText("VELOCITY", wood+30, 445, 90, 10, juce::Justification::centred);

    const int seqX  = wood + 140;
    const int stepW = (W - wood - jackW - seqX) / 8;
    for (int i = 0; i < 8; ++i)
        g.drawText(juce::String(i+1), seqX + i*stepW, 314, stepW, 10, juce::Justification::centred);

    // LEDs – midpoint between pitch row (328–368) and velocity row (432–462)
    // pitch bottom = 368, velocity top = 432, midpoint = 400
    for (int i = 0; i < 8; ++i)
        {
            float lx = (float)(seqX + i * stepW + stepW / 2);
            float ly = 400.0f;
            bool active = (i == currentLedStep);

                    // Yttre glöd
                    if (active)
                    {
                        g.setColour(juce::Colour(0x22ff2200));
                        g.fillEllipse(lx - 10, ly - 10, 20, 20);
                        g.setColour(juce::Colour(0x44ff2200));
                        g.fillEllipse(lx - 8, ly - 8, 16, 16);
                    }

                    // LED-kropp
                    juce::ColourGradient ledGrad(
                        active ? juce::Colour(0xffff4422) : juce::Colour(0xff553322),
                        lx - 3, ly - 3,
                        active ? juce::Colour(0xffaa1100) : juce::Colour(0xff221111),
                        lx + 3, ly + 3,
                        true);
                    g.setGradientFill(ledGrad);
                    g.fillEllipse(lx - 5, ly - 5, 10, 10);

                    // Spegelreflex
                    if (active)
                    {
                        g.setColour(juce::Colour(0x88ffffff));
                        g.fillEllipse(lx - 2.5f, ly - 3.5f, 3.0f, 2.0f);
                    }

                    // Kant
                    g.setColour(active ? juce::Colour(0xff882200) : juce::Colour(0xff222222));
                    g.drawEllipse(lx - 5, ly - 5, 10, 10, 1.0f);
        }

    // Branding
    g.setFont(juce::FontOptions(22.0f).withStyle("Bold"));
    g.setColour(labelBlack);
    g.drawText("DFAF", wood+20, H-48, 80, 28, juce::Justification::centredLeft);
    g.setFont(juce::FontOptions(7.5f).withStyle("Bold"));
    g.drawText("DRUMMER FROM",   wood+104, H-48, 120, 14, juce::Justification::centredLeft);
    g.drawText("ANOTHER FATHER", wood+104, H-34, 120, 14, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xffaaaaaa));
    g.drawLine((float)(wood+100), (float)(H-50), (float)(wood+100), (float)(H-18), 0.8f);
    g.setColour(labelBlack);
    g.setFont(juce::FontOptions(7.5f));
    g.drawText("SEMI-MODULAR ANALOG",    wood+228, H-48, 160, 14, juce::Justification::centredLeft);
    g.drawText("PERCUSSION SYNTHESIZER", wood+228, H-34, 160, 14, juce::Justification::centredLeft);
    g.setFont(juce::FontOptions(30.0f).withStyle("Italic"));
    g.drawText("fjord", W-wood-jackW-20, H-52, 138, 36, juce::Justification::centredRight);
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("\xc2\xae", W-wood-jackW+116, H-44, 16, 16, juce::Justification::centredLeft);
}

void DFAFEditor::resized()
{
    const int W     = getWidth();
    const int wood  = 18;
    const int jackW = 200;
    const int mainW = W - wood * 2 - jackW;
    const int kS    = (mainW - 60) / 11;
    const int offX  = wood + 30;
    const int kSz   = 54;
    const int sSz   = 30;

    seqPitchModBox.setBounds(offX + 1*kS + (kS-60)/2, 50, 60, 22);
    hardSyncBox.setBounds(offX + 1*kS + (kS-60)/2, 210, 60, 22);
    vco1WaveBox.setBounds(offX + 4*kS + (kS-60)/2, 50, 60, 22);
    vco2WaveBox.setBounds(offX + 4*kS + (kS-60)/2, 210, 60, 22);
    vcfModeBox.setBounds(offX + 6*kS - kS/2 + (kS/2-60)/2, 50, 60, 22);

    int r1slots[]  = { 0, 2, 3, 5, 6, 7, 8, 9, 10 };
        juce::Slider* row1[] = {
            &vcoDecay, &vco1EgAmount, &vco1Frequency,
            &vco1Level, &noiseLevel, &cutoff, &resonance, &vcaEg, &volume
        };
        int r1sizes[] = { kSz,kSz,kSz, sSz,sSz, kSz,kSz,kSz,kSz };
        for (int i = 0; i < 9; ++i)
            row1[i]->setBounds(offX + r1slots[i]*kS + (kS-r1sizes[i])/2, 22, r1sizes[i], r1sizes[i]);

    // Row 2: positions 0, 2,3, 5, 6,7,8, 10
    int r2slots[]  = { 0, 2, 3, 5, 6, 7, 8, 10 };
        juce::Slider* row2[] = {
            &fmAmount, &vco2EgAmount, &vco2Frequency,
            &vco2Level, &vcfDecay, &vcfEgAmount, &noiseVcfMod, &vcaDecay
        };
        int r2sizes[] = { kSz,kSz,kSz, sSz,kSz,kSz,kSz,kSz };
    for (int i = 0; i < 8; ++i)
        row2[i]->setBounds(offX + r2slots[i]*kS + (kS-r2sizes[i])/2, 182, r2sizes[i], r2sizes[i]);

    // Sequencer
    clockMultBox.setBounds(wood+18, 326, 90, 22);
    resetButton.setBounds(wood+18, 408, 90, 24);

    const int seqX  = wood + 140;
    const int stepW = (W - wood - jackW - seqX) / 8;
    for (int i = 0; i < 8; ++i)
    {
        int x = seqX + i*stepW + (stepW-sSz)/2;
        stepPitch[i].setBounds(x, 326, sSz, sSz);
        stepVelocity[i].setBounds(x, 432, sSz, sSz);
    }
}
