#pragma once
#include <JuceHeader.h>

class DecayEnvelope
{
public:
    DecayEnvelope() = default;

    void prepare(double sampleRate)
    {
        sr = sampleRate;
        updateCoeff();
    }

    void setDecayTime(float decaySeconds)
    {
        decayTime = decaySeconds;
        updateCoeff();
    }

    void trigger()
    {
        value = 1.0f;
    }

    void triggerSoft(float startValue)
    {
        value = startValue;
    }

    float process()
    {
        value *= coeff;
        return value;
    }

    bool isActive() const { return value > 0.0001f; }

private:
    void updateCoeff()
    {
        if (sr > 0.0 && decayTime > 0.0f)
            coeff = std::exp(-1.0f / (decayTime * (float)sr));
        else
            coeff = 0.0f;
    }

    double sr       = 44100.0;
    float decayTime = 0.5f;
    float coeff     = 0.0f;
    float value     = 0.0f;
};
