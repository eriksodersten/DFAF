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
        envelope.prepare(sampleRate);
    }

    void setDecayTime(float seconds)
    {
        envelope.setDecayTime(seconds);
    }

    // pitch = MIDI note, velocity = 0.0–1.0, fmAmount = 0.0–1.0
    void trigger(float midiNote, float velocity, float fmAmount = 0.3f)
    {
        freq1 = midiNoteToHz(midiNote);
        freq2 = midiNoteToHz(midiNote + 7.0f);  // VCO2 a fifth up
        fm    = fmAmount;
        vel   = velocity;
        envelope.trigger();
        phase1 = 0.0f;
        phase2 = 0.0f;
    }

    float process()
    {
        if (!envelope.isActive())
            return 0.0f;

        float env = envelope.process();

        // VCO2 – simple sine
        float vco2 = std::sin(phase2 * juce::MathConstants<float>::twoPi);
        phase2 += freq2 / (float)sr;
        if (phase2 >= 1.0f) phase2 -= 1.0f;

        // VCO1 – sine with FM from VCO2
        float fmOffset = fm * freq1 * vco2;
        float instantFreq1 = freq1 + fmOffset;
        float vco1 = std::sin(phase1 * juce::MathConstants<float>::twoPi);
        phase1 += instantFreq1 / (float)sr;
        if (phase1 >= 1.0f) phase1 -= 1.0f;

        // Noise
        float noise = random.nextFloat() * 2.0f - 1.0f;

        // Mix: VCO1 60%, VCO2 20%, noise 20%
        float mix = vco1 * 0.6f + vco2 * 0.2f + noise * 0.2f;

        return mix * env * vel;
    }

    bool isActive() const { return envelope.isActive(); }

private:
    static float midiNoteToHz(float note)
    {
        return 440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f);
    }

    double sr     = 44100.0;
    float freq1   = 440.0f;
    float freq2   = 660.0f;
    float fm      = 0.3f;
    float vel     = 1.0f;
    float phase1  = 0.0f;
    float phase2  = 0.0f;

    DecayEnvelope envelope;
    juce::Random  random;
};
