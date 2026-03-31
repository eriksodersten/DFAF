#pragma once
#include <JuceHeader.h>
#include "DecayEnvelope.h"

class DFAFVoice
{
public:
    DFAFVoice() = default;

    void prepare(double sampleRate)
    {
        sr = sampleRate;
        vcoEnvelope.prepare(sampleRate);
        vcaEnvelope.prepare(sampleRate);
        vcfEnvelope.prepare(sampleRate);
    }

    void setDecayTime(float seconds)    { vcoEnvelope.setDecayTime(seconds); }
    void setVcaDecayTime(float seconds) { vcaEnvelope.setDecayTime(seconds); }
    void setVcfDecayTime(float seconds) { vcfEnvelope.setDecayTime(seconds); }
    void setFmAmount(float amount)      { fm = amount; }
    void setVco1EgAmount(float semitones) { vco1EgAmt = semitones; }
    void setVco2EgAmount(float semitones) { vco2EgAmt = semitones; }
    void setVcfEgAmount(float amount)     { vcfEgAmt = amount; }

    // Returnerar senaste VCF EG-värde för användning i processorn
    float getVcfEnvValue() const { return lastVcfEnv; }

    void trigger(float midiNote, float velocity, float fmAmount = 0.3f)
    {
        baseMidiNote = midiNote;
        freq1 = midiNoteToHz(midiNote);
        freq2 = midiNoteToHz(midiNote + 7.0f);
        fm    = fmAmount;
        vel   = velocity;
        vcoEnvelope.trigger();
        vcaEnvelope.trigger();
        vcfEnvelope.trigger();
        phase1 = 0.0f;
        phase2 = 0.0f;
    }

    float process()
    {
        if (!vcaEnvelope.isActive())
            return 0.0f;

        float vcoEnv = vcoEnvelope.process();
        float vcaEnv = vcaEnvelope.process();
        lastVcfEnv   = vcfEnvelope.process();

        // VCO EG pitch modulation
        float note1 = baseMidiNote + vco1EgAmt * vcoEnv;
        float note2 = baseMidiNote + 7.0f + vco2EgAmt * vcoEnv;
        float modFreq1 = midiNoteToHz(note1);
        float modFreq2 = midiNoteToHz(note2);

        // VCO2
        float vco2 = std::sin(phase2 * juce::MathConstants<float>::twoPi);
        phase2 += modFreq2 / (float)sr;
        if (phase2 >= 1.0f) phase2 -= 1.0f;

        // VCO1 med FM
        float fmOffset = fm * modFreq1 * vco2;
        float vco1 = std::sin(phase1 * juce::MathConstants<float>::twoPi);
        phase1 += (modFreq1 + fmOffset) / (float)sr;
        if (phase1 >= 1.0f) phase1 -= 1.0f;

        // Noise
        float noise = random.nextFloat() * 2.0f - 1.0f;

        float toneAmp = vcoEnv;  // VCO decay dämpar tonen (VCO1+VCO2)
                float mix = vco1 * 0.6f * toneAmp + vco2 * 0.2f * toneAmp + noise * 0.2f;
                float vcaLinear = vcaEnv * vcaEnv;
                return mix * vcaLinear * vel;
    }

    bool isActive() const { return vcaEnvelope.isActive(); }

private:
    static float midiNoteToHz(float note)
    {
        return 440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f);
    }

    double sr          = 44100.0;
    float baseMidiNote = 69.0f;
    float freq1        = 440.0f;
    float freq2        = 660.0f;
    float fm           = 0.3f;
    float vel          = 1.0f;
    float phase1       = 0.0f;
    float phase2       = 0.0f;
    float vco1EgAmt    = 0.0f;
    float vco2EgAmt    = 0.0f;
    float vcfEgAmt     = 0.0f;
    float lastVcfEnv   = 0.0f;

    DecayEnvelope vcoEnvelope;
    DecayEnvelope vcaEnvelope;
    DecayEnvelope vcfEnvelope;
    juce::Random  random;
};
