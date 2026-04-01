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

    void setDecayTime(float seconds)      { vcoEnvelope.setDecayTime(seconds); }
    void setVcaDecayTime(float seconds)   { vcaEnvelope.setDecayTime(seconds); }
    void setVcfDecayTime(float seconds)   { vcfEnvelope.setDecayTime(seconds); }
    void setFmAmount(float amount)        { fm = amount; }
    void setVco1EgAmount(float semitones) { vco1EgAmt = semitones; }
    void setVco2EgAmount(float semitones) { vco2EgAmt = semitones; }
    void setVcfEgAmount(float amount)     { vcfEgAmt = amount; }
    void setNoiseLevel(float level)       { noiseLevel = level; }
    void setVco1Level(float level)        { vco1Level = level; }
    void setVco2Level(float level)        { vco2Level = level; }
    void setVcaEgAmount(float amount)     { vcaEgAmount = amount; }
    void setVco1BaseFreq(float hz)        { vco1BaseFreq = hz; }
    void setVco2BaseFreq(float hz)        { vco2BaseFreq = hz; }
    void setSeqPitchRouting(int routing)  { seqPitchRouting = routing; }
    void setHardSync(bool enabled)        { hardSync = enabled; }
        void setVco1Wave(int wave)            { vco1Wave = wave; } // 0=sine, 1=triangle
        void setVco2Wave(int wave)            { vco2Wave = wave; }

    float getVcfEnvValue() const { return lastVcfEnv; }

    void trigger(float midiNote, float velocity, float fmAmount = 0.3f)
    {
        baseMidiNote = midiNote;
        float seqOffset = midiNote - 60.0f;

        float base1 = vco1BaseFreq > 0.0f ? vco1BaseFreq : midiNoteToHz(60.0f);
        float base2 = vco2BaseFreq > 0.0f ? vco2BaseFreq : midiNoteToHz(67.0f);

        if (seqPitchRouting == 0) // VCO 1&2
        {
            freq1 = base1 * std::pow(2.0f, seqOffset / 12.0f);
            freq2 = base2 * std::pow(2.0f, seqOffset / 12.0f);
        }
        else if (seqPitchRouting == 1) // OFF
        {
            freq1 = base1;
            freq2 = base2;
        }
        else // VCO 2 only
        {
            freq1 = base1;
            freq2 = base2 * std::pow(2.0f, seqOffset / 12.0f);
        }

        fm  = fmAmount;
        vel = velocity;
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

        float modFreq1 = freq1 * std::pow(2.0f, vco1EgAmt * vcoEnv / 12.0f);
        float modFreq2 = freq2 * std::pow(2.0f, vco2EgAmt * vcoEnv / 12.0f);

        float vco2out = (vco2Wave == 0)
                    ? (phase2 < 0.5f ? 1.0f : -1.0f)          // square
                    : 1.0f - 4.0f * std::abs(phase2 - 0.5f);  // triangle
        phase2 += modFreq2 / (float)sr;
        if (phase2 >= 1.0f) phase2 -= 1.0f;

        float fmOffset = fm * modFreq1 * vco2out;
        float vco1out = (vco1Wave == 0)
                    ? (phase1 < 0.5f ? 1.0f : -1.0f)          // square
                    : 1.0f - 4.0f * std::abs(phase1 - 0.5f);  // triangle
        phase1 += (modFreq1 + fmOffset) / (float)sr;
        if (phase1 >= 1.0f)
        {
            phase1 -= 1.0f;
            if (hardSync) phase2 = 0.0f;
        }

        float noise = random.nextFloat() * 2.0f - 1.0f;

        float toneAmp = vcoEnv;
        float mix = vco1out * vco1Level * toneAmp + vco2out * vco2Level * toneAmp + noise * noiseLevel;
        float vcaLinear = vcaEnv * vcaEnv;
        return mix * vcaLinear * vcaEgAmount * vel;
    }

    bool isActive() const { return vcaEnvelope.isActive(); }

private:
    static float midiNoteToHz(float note)
    {
        return 440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f);
    }

    double sr             = 44100.0;
    float baseMidiNote    = 69.0f;
    float freq1           = 440.0f;
    float freq2           = 660.0f;
    float fm              = 0.3f;
    float vel             = 1.0f;
    float phase1          = 0.0f;
    float phase2          = 0.0f;
    float vco1EgAmt       = 0.0f;
    float vco2EgAmt       = 0.0f;
    float vcfEgAmt        = 0.0f;
    float lastVcfEnv      = 0.0f;
    float noiseLevel      = 0.2f;
    float vco1Level       = 0.6f;
    float vco2Level       = 0.2f;
    float vcaEgAmount     = 0.5f;
    float vco1BaseFreq    = 0.0f;
    float vco2BaseFreq    = 0.0f;
    int   seqPitchRouting = 0;
        int   vco1Wave        = 0;
        int   vco2Wave        = 0;
    bool  hardSync        = false;

    DecayEnvelope vcoEnvelope;
    DecayEnvelope vcaEnvelope;
    DecayEnvelope vcfEnvelope;
    juce::Random  random;
};
