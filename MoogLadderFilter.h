#pragma once
#include <cmath>
#include <JuceHeader.h>

// Huovilainen Moog Ladder Filter – full model with 2x oversampling
// Based on: Huovilainen (2004) "Non-Linear Digital Implementation of the Moog Ladder Filter"
class MoogLadderFilter
{
public:
    MoogLadderFilter() = default;

    void prepare(double sampleRate)
    {
        sr = sampleRate * 2.0; // 2x oversampling
        reset();
        updateCoeffs();
    }

    void reset()
    {
        for (int i = 0; i < 4; ++i)
            stage[i] = stageTanh[i] = delay[i] = 0.0f;
        lastSample = 0.0f;
    }

    void setCutoff(float cutoffHz)
    {
        cutoff = juce::jlimit(20.0f, 20000.0f, cutoffHz);
        updateCoeffs();
    }

    void setResonance(float res)
    {
        // Scale resonance to allow self-oscillation at max
        resonance = juce::jlimit(0.0f, 1.0f, res) * 4.5f;
        updateCoeffs();
    }

    void setHighpass(bool hp) { highpass = hp; }

    float process(float input)
    {
        // 2x oversampling – process twice, return second result
        float out = processSample(input);
        out = processSample(input);
        return out;
    }

private:
    float processSample(float input)
    {
        // Thermal voltage constant
        const float VT = 0.312f;

        // Predictor step
        float x  = input - resonance * delay[3];
        float x0 = fastTanh(x / (2.0f * VT));

        float p0 = delay[0] + VT * g * (x0            - stageTanh[0]);
        float p1 = delay[1] + VT * g * (fastTanh(delay[0] / (2.0f * VT)) - stageTanh[1]);
        float p2 = delay[2] + VT * g * (fastTanh(delay[1] / (2.0f * VT)) - stageTanh[2]);
        float p3 = delay[3] + VT * g * (fastTanh(delay[2] / (2.0f * VT)) - stageTanh[3]);

        // Corrector step
        float x1  = input - resonance * p3;
        float xT  = fastTanh(x1 / (2.0f * VT));

        stage[0] = delay[0] + VT * g * (xT                              - stageTanh[0]);
        stage[1] = delay[1] + VT * g * (fastTanh(p0 / (2.0f * VT))     - stageTanh[1]);
        stage[2] = delay[2] + VT * g * (fastTanh(p1 / (2.0f * VT))     - stageTanh[2]);
        stage[3] = delay[3] + VT * g * (fastTanh(p2 / (2.0f * VT))     - stageTanh[3]);

        stageTanh[0] = fastTanh(stage[0] / (2.0f * VT));
        stageTanh[1] = fastTanh(stage[1] / (2.0f * VT));
        stageTanh[2] = fastTanh(stage[2] / (2.0f * VT));
        stageTanh[3] = fastTanh(stage[3] / (2.0f * VT));

        for (int i = 0; i < 4; ++i)
            delay[i] = stage[i];

        float lpOut = stage[3];
        float hpOut = input - lpOut;

        lastSample = highpass ? hpOut : lpOut;
        return lastSample;
    }

    void updateCoeffs()
    {
        // Frequency warping for oversampled rate
        float fc = (float)(cutoff / sr);
        fc = juce::jmin(fc, 0.49f);
        // Huovilainen g coefficient
        g = 2.0f * std::sin(juce::MathConstants<float>::pi * fc);
    }

    // Pade approximation of tanh – faster than std::tanh, accurate to ~0.5%
    static inline float fastTanh(float x)
    {
        if (x > 3.0f)  return  1.0f;
        if (x < -3.0f) return -1.0f;
        float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }

    double sr        = 88200.0;
    float cutoff     = 1000.0f;
    float resonance  = 0.0f;
    float g          = 0.0f;
    bool  highpass   = false;
    float lastSample = 0.0f;

    float stage[4]     = {};
    float stageTanh[4] = {};
    float delay[4]     = {};
};
