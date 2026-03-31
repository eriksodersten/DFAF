#pragma once
#include <cmath>

// Huovilainen Moog Ladder Filter (4-pole lowpass)
class MoogLadderFilter
{
public:
    MoogLadderFilter() = default;

    void prepare(double sampleRate)
    {
        sr = sampleRate;
        reset();
    }

    void reset()
    {
        s[0] = s[1] = s[2] = s[3] = 0.0f;
        t[0] = t[1] = t[2] = t[3] = 0.0f;
    }

    // cutoff = Hz, resonance = 0.0–1.0
    void setCutoff(float cutoffHz)
    {
        cutoff = cutoffHz;
        updateCoeffs();
    }

    void setResonance(float res)
    {
        resonance = res;
        updateCoeffs();
    }

    float process(float input)
    {
        // Feedback
        float feedback = 4.0f * resonance * s[3];

        float x = input - feedback;
        x = tanhf(x);

        float s0 = s[0] + g * (x    - t[0]);
        float s1 = s[1] + g * (t[0] - t[1]);
        float s2 = s[2] + g * (t[1] - t[2]);
        float s3 = s[3] + g * (t[2] - t[3]);

        t[0] = tanhf(s0);
        t[1] = tanhf(s1);
        t[2] = tanhf(s2);
        t[3] = tanhf(s3);

        s[0] = s0;
        s[1] = s1;
        s[2] = s2;
        s[3] = s3;

        return s3;
    }

private:
    void updateCoeffs()
    {
        // Frequency warping
        float fc = cutoff / (float)sr;
        fc = juce::jmin(fc, 0.49f);
        g = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * fc);
    }

    double sr       = 44100.0;
    float cutoff    = 1000.0f;
    float resonance = 0.0f;
    float g         = 0.0f;
    float s[4]      = {};
    float t[4]      = {};
};
