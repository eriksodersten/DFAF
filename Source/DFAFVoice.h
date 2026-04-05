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
        smoothedVcoEnv.reset(sampleRate, 0.001);
        vcaAttack.reset(sampleRate, 0.001);
        vcaAttack.setCurrentAndTargetValue(1.0f);
        smoothedAmp = 0.0f;
        const float dezipperSeconds = 0.001f;
        ampDezipperCoeff = 1.0f - std::exp(-1.0f / ((float)sampleRate * dezipperSeconds));
        vcfAttackCoeff   = ampDezipperCoeff;
    }

    void setDecayTime(float seconds)      { vcoEnvelope.setDecayTime(seconds); }
    void setVcaDecayTime(float seconds)   { vcaEnvelope.setDecayTime(seconds); }
    void setVcfDecayTime(float seconds)   { vcfDecaySeconds = seconds; }
    void setFmAmount(float amount)        { fm = amount; }
    void setVco1EgAmount(float semitones) { vco1EgAmt = semitones; }
    void setVco2EgAmount(float semitones) { vco2EgAmt = semitones; }
    void setVcfEgAmount(float amount)     { vcfEgAmt = amount; }
    void setNoiseLevel(float level)       { noiseLevel = level; }
    void setVco1Level(float level)        { vco1Level = level; }
    void setVco2Level(float level)        { vco2Level = level; }
    void setVcaEgAmount(float amount)     { vcaEgAmount = amount; }
    void setVcaAttackTime(float seconds)  { vcaAttackSeconds = seconds; }
    void setVco1BaseFreq(float hz)        { vco1BaseFreq = hz; }
    void setVco2BaseFreq(float hz)        { vco2BaseFreq = hz; }
    void setSeqPitchRouting(int routing)  { seqPitchRouting = routing; }
    void setHardSync(bool enabled)        { hardSync = enabled; }
    void setVco1Wave(int wave)            { vco1Wave = wave; }
    void setVco2Wave(int wave)            { vco2Wave = wave; }

    float getVcfEnvValue() const { return lastVcfEnv; }

    struct Frame {
        float raw     = 0.0f;
        float ampGain = 0.0f;
        float vcfEnv  = 0.0f;
        float vcoEnv  = 0.0f;   // smoothed VCO envelope (0..1)
        float noiseRaw = 0.0f;
    };

    Frame processFrame()
    {
        Frame f{};
        float targetAmp = 0.0f;

        if (vcaEnvelope.isActive())
        {
            float rawVcoEnv = vcoEnvelope.process();
            smoothedVcoEnv.setTargetValue(rawVcoEnv);
            float vcoEnv     = smoothedVcoEnv.getNextValue();
            float vcaEnv     = vcaEnvelope.process();
            float attackGain = vcaAttack.getNextValue();
            float targetVcfEnv = vcfEnvelope.process();
            if (targetVcfEnv > lastVcfEnv)
                lastVcfEnv += (targetVcfEnv - lastVcfEnv) * vcfAttackCoeff;
            else
                lastVcfEnv = targetVcfEnv;

            float modFreq1 = freq1 * std::pow(2.0f, vco1EgAmt * vcoEnv * vel / 12.0f);
            float inst1    = modFreq1 / (float)sr * phaseDir1;
            phase1 += inst1;
            bool sync1 = false;
            if (phase1 >= 1.0f) { phase1 = 2.0f - phase1; phaseDir1 = -phaseDir1; sync1 = true; }
            if (phase1 <  0.0f) { phase1 = -phase1;        phaseDir1 = -phaseDir1; sync1 = true; }

            float vco1out;
            if (vco1Wave == 0) {
                vco1out = (phase1 < 0.5f ? 1.0f : -1.0f) * phaseDir1;
            } else {
                float p   = phase1 * 4.0f;
                float tri = (p < 1.0f) ? p : (p < 3.0f) ? 2.0f - p : p - 4.0f;
                vco1out   = std::tanh(2.2f * tri) / std::tanh(2.2f) * phaseDir1;
            }

            float vco2out;
            if (vco2Wave == 0) {
                vco2out = (phase2 < 0.5f ? 1.0f : -1.0f);
            } else {
                float p   = phase2 * 4.0f;
                float tri = (p < 1.0f) ? p : (p < 3.0f) ? 2.0f - p : p - 4.0f;
                vco2out   = std::tanh(2.2f * tri) / std::tanh(2.2f);
            }

            float modulatedFreq2 = freq2 * std::pow(2.0f, vco2EgAmt * vcoEnv * vel / 12.0f + fm * vco1out * 2.0f);
            float inst2          = modulatedFreq2 / (float)sr * phaseDir2;
            phase2 += inst2;
            if (phase2 >= 1.0f) { phase2 = 2.0f - phase2; phaseDir2 = -phaseDir2; }
            if (phase2 <  0.0f) { phase2 = -phase2;        phaseDir2 = -phaseDir2; }
            if (hardSync && sync1) { phase2 = 0.0f; phaseDir2 = 1.0f; }

            float noise   = random.nextFloat() * 2.0f - 1.0f;
            float toneAmp = vcoEnv;
            f.raw      = vco1out * vco1Level * toneAmp + vco2out * vco2Level * toneAmp + noise * noiseLevel;
            f.vcfEnv   = lastVcfEnv * vel;
            f.vcoEnv   = vcoEnv;
            f.noiseRaw = noise;
            targetAmp = vcaEnv * attackGain;
        }
        else
        {
            lastVcfEnv = 0.0f;
            f.vcfEnv   = 0.0f;
            targetAmp  = 0.0f;
        }

        smoothedAmp += (targetAmp - smoothedAmp) * ampDezipperCoeff;
        if (std::abs(smoothedAmp) < 1.0e-6f)
            smoothedAmp = 0.0f;

        lastVcaEnv = smoothedAmp;
        f.ampGain  = smoothedAmp;
        return f;
    }

    void trigger(float midiNote, float velocity, float fmAmount = 0.3f)
    {
        baseMidiNote = midiNote;
        float seqOffset = midiNote - 60.0f;

        float base1 = vco1BaseFreq > 0.0f ? vco1BaseFreq : midiNoteToHz(60.0f);
        float base2 = vco2BaseFreq > 0.0f ? vco2BaseFreq : midiNoteToHz(67.0f);

        if (seqPitchRouting == 0)
        {
            freq1 = base1 * std::pow(2.0f, seqOffset / 12.0f);
            freq2 = base2 * std::pow(2.0f, seqOffset / 12.0f);
        }
        else if (seqPitchRouting == 1)
        {
            freq1 = base1;
            freq2 = base2;
        }
        else
        {
            freq1 = base1;
            freq2 = base2 * std::pow(2.0f, seqOffset / 12.0f);
        }

        fm  = fmAmount;
                if (vco1Wave == 1) phaseDir1 = 1.0f;
                if (vco2Wave == 1) phaseDir2 = 1.0f;
                smoothedVcoEnv.setCurrentAndTargetValue(smoothedVcoEnv.getCurrentValue());

        if (velocity > 0.0f)
                        {
                            vel = velocity;
                            vcoEnvelope.trigger();
                            float vcfDecayScaled = vcfDecaySeconds * (1.0f - vel * 0.5f);
                            vcfDecayScaled = juce::jmax(0.01f, vcfDecayScaled);
                            vcfEnvelope.setDecayTime(vcfDecayScaled);
                            vcfEnvelope.trigger();
                            vcaAttack.reset((float)sr, vcaAttackSeconds);
                            vcaAttack.setCurrentAndTargetValue(0.0f);
                            vcaAttack.setTargetValue(1.0f);
                            vcaEnvelope.trigger(vel);
                        }
    }

    bool isActive() const { return vcaEnvelope.isActive(); }

private:
    static float midiNoteToHz(float note)
    {
        return 440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f);
    }

    double sr              = 44100.0;
    float baseMidiNote     = 69.0f;
    float freq1            = 440.0f;
    float freq2            = 660.0f;
    float fm               = 0.3f;
    float vel              = 1.0f;
    float phase1           = 0.0f;
    float phase2           = 0.0f;
    float phaseDir1        = 1.0f;
    float phaseDir2        = 1.0f;
    float vco1EgAmt        = 0.0f;
    float vco2EgAmt        = 0.0f;
    float vcfEgAmt         = 0.0f;
    float lastVcfEnv       = 0.0f;
    float lastVcaEnv       = 0.0f;
    float noiseLevel       = 0.2f;
    float vco1Level        = 0.6f;
    float vco2Level        = 0.2f;
    float vcaEgAmount      = 0.5f;
    float vcaAttackSeconds  = 0.001f;
        float vcfDecaySeconds   = 0.3f;
    float vco1BaseFreq     = 0.0f;
    float vco2BaseFreq     = 0.0f;
    int   seqPitchRouting  = 0;
    int   vco1Wave         = 0;
    int   vco2Wave         = 0;
    bool  hardSync         = false;
    float smoothedAmp      = 0.0f;
    float ampDezipperCoeff = 1.0f;
    float vcfAttackCoeff   = 1.0f;

    DecayEnvelope vcoEnvelope;
    DecayEnvelope vcaEnvelope;
    DecayEnvelope vcfEnvelope;
    juce::Random  random;
    juce::SmoothedValue<float> smoothedVcoEnv;
    juce::SmoothedValue<float> vcaAttack;
};
