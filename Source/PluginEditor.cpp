#include "PluginEditor.h"

static const juce::Colour panelWhite = juce::Colour(0xfff0ede8);
static const juce::Colour labelBlack = juce::Colour(0xff111111);

DFAFEditor::DFAFEditor(DFAFProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(1200, 480);
    setLookAndFeel(&laf);

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
    add(vco1Level, true); add(noiseLevel, true); add(cutoff); add(resonance); add(vcaEg); add(volume);
    add(fmAmount); add(vco2EgAmount); add(vco2Frequency);
    add(vco2Level, true); add(vcfDecay); add(vcfEgAmount); add(noiseVcfMod); add(vcaDecay);
    add(tempo);
    for (int i = 0; i < 8; ++i) { add(stepPitch[i], true); add(stepVelocity[i], true); }

    auto& apvts = p.apvts;
    vcoDecayAtt       = std::make_unique<SliderAttachment>(apvts, "vcoDecay", vcoDecay);
    seqPitchModBoxAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, "seqPitchMod", seqPitchModBox);
        hardSyncBoxAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, "hardSync", hardSyncBox);
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
    tempoAtt       = std::make_unique<SliderAttachment>(apvts, "tempo",       tempo);
}

DFAFEditor::~DFAFEditor()
{
    setLookAndFeel(nullptr);
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

void DFAFEditor::drawJackPanel(juce::Graphics& g, int x, int y, int w, int h) const
{
    g.setColour(juce::Colour(0xffe8e5e0));
    g.fillRect(x, y, w, h);
    g.setColour(juce::Colour(0xffbbbbbb));
    g.drawRect(x, y, w, h, 1);

    auto drawJack = [&](int jx, int jy, const juce::String& lbl) {
        g.setColour(juce::Colour(0xff333333));
        g.fillEllipse((float)(jx-8), (float)(jy-8), 16.0f, 16.0f);
        g.setColour(juce::Colour(0xff666666));
        g.drawEllipse((float)(jx-8), (float)(jy-8), 16.0f, 16.0f, 1.5f);
        g.setColour(juce::Colour(0xff999999));
        g.fillEllipse((float)(jx-3), (float)(jy-3), 6.0f, 6.0f);
        g.setColour(labelBlack);
        g.setFont(juce::FontOptions(6.0f));
        g.drawText(lbl, jx-20, jy+10, 40, 8, juce::Justification::centred);
    };

    struct JackDef { const char* label; };
    JackDef jacks[] = {
        {"TRIGGER"},{"VCA CV"},{"VCA"},
        {"VELOCITY"},{"VCA DEC"},{"VCA EG"},
        {"EXT AUD"},{"VCF DEC"},{"VCF EG"},
        {"NOISE LV"},{"VCO DEC"},{"VCO EG"},
        {"VCF MOD"},{"VCO1 CV"},{"VCO 1"},
        {"1-2 FAMT"},{"VCO2 CV"},{"VCO 2"},
        {"TEMPO"},{"RUN/STP"},{"ADV/CLK"},
        {"TRIGGER"},{"VELOCTY"},{"PITCH"}
    };

    int col1 = x + 16, col2 = x + 44, col3 = x + 72;
    int startY = y + 20;
    int stride = 28;

    g.setColour(labelBlack);
    g.setFont(juce::FontOptions(6.5f).withStyle("Bold"));
    g.drawText("IN/OUT", x, y + 4, w, 10, juce::Justification::centred);

    for (int r = 0; r < 8; ++r)
    {
        int idx = r * 3;
        if (idx < 24) drawJack(col1, startY + r * stride, jacks[idx].label);
        if (idx+1 < 24) drawJack(col2, startY + r * stride, jacks[idx+1].label);
        if (idx+2 < 24) drawJack(col3, startY + r * stride, jacks[idx+2].label);
    }
}

void DFAFEditor::paint(juce::Graphics& g)
{
    const int W     = getWidth();
    const int H     = getHeight();
    const int wood  = 18;
    const int jackW = 88;

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
    drawJackPanel(g, W - wood - jackW, 0, jackW, H);

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

    // VCF HP/LP switch label
    g.drawText("VCF", offX + 6*kS - kS/2, 8, kS/2, 10, juce::Justification::centred);

    // Switches row 1: VCO1 WAVE, VCF HP/LP
    drawSwitch(g, (float)(offX + 4*kS), 20.0f, (float)kS, 118.0f, "",
               juce::StringArray({"Л", "^"}));
    drawSwitch(g, (float)(offX + 6*kS - kS/2), 20.0f, (float)(kS/2), 118.0f, "",
               juce::StringArray({"HP", "LP"}));

    // Switches row 2: HARD SYNC, VCO2 WAVE, VCA EG FAST/SLOW
    drawSwitch(g, (float)(offX + 1*kS), 178.0f, (float)kS, 100.0f, "",
               juce::StringArray({"ON", "OFF"}));
    drawSwitch(g, (float)(offX + 4*kS), 178.0f, (float)kS, 100.0f, "",
               juce::StringArray({"Л", "^"}));

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
        g.setColour(juce::Colour(0xff333333));
        g.fillEllipse(lx - 5, ly - 5, 10, 10);
        g.setColour(juce::Colour(0xff555555));
        g.drawEllipse(lx - 5, ly - 5, 10, 10, 1.0f);
    }

    // Buttons
    drawButton(g, (float)(wood+45), 355.0f, 12.0f, "TRIGGER");
    drawButton(g, (float)(wood+45), 418.0f, 18.0f, "RUN/STOP", true);
    drawButton(g, (float)(wood+85), 418.0f, 12.0f, "ADVANCE");

    // RUN/STOP LED indicator
    g.setColour(juce::Colour(0xff333333));
    g.fillEllipse((float)(wood+30), 392.0f, 8.0f, 8.0f);
    g.setColour(juce::Colour(0xff555555));
    g.drawEllipse((float)(wood+30), 392.0f, 8.0f, 8.0f, 1.0f);
    g.setColour(labelBlack);
    g.setFont(juce::FontOptions(6.5f));
    g.drawText("RUN/STOP", wood+20, 402, 50, 8, juce::Justification::centred);

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
    const int jackW = 88;
    const int mainW = W - wood * 2 - jackW;
    const int kS    = (mainW - 60) / 11;
    const int offX  = wood + 30;
    const int kSz   = 54;
    const int sSz   = 30;

    seqPitchModBox.setBounds(offX + 1*kS + (kS-60)/2, 50, 60, 22);
        hardSyncBox.setBounds(offX + 1*kS + (kS-60)/2, 210, 60, 22);

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
    tempo.setBounds(wood+38, 326, kSz+10, kSz+10);

    const int seqX  = wood + 140;
    const int stepW = (W - wood - jackW - seqX) / 8;
    for (int i = 0; i < 8; ++i)
    {
        int x = seqX + i*stepW + (stepW-sSz)/2;
        stepPitch[i].setBounds(x, 326, sSz, sSz);
        stepVelocity[i].setBounds(x, 432, sSz, sSz);
    }
}
