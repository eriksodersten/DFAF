#pragma once
#include <JuceHeader.h>

struct SequencerStep
{
    float pitch    = 60.0f;  // MIDI note number
    float velocity = 0.8f;   // 0.0 – 1.0
};

class DFAFSequencer
{
public:
    DFAFSequencer()
    {
        // Default pattern – kick-like pitches
        steps[0] = { 36.0f, 1.0f };
        steps[1] = { 48.0f, 0.6f };
        steps[2] = { 38.0f, 0.8f };
        steps[3] = { 48.0f, 0.4f };
        steps[4] = { 36.0f, 1.0f };
        steps[5] = { 50.0f, 0.5f };
        steps[6] = { 38.0f, 0.7f };
        steps[7] = { 48.0f, 0.6f };
    }

    void prepare(double sampleRate)
    {
        sr = sampleRate;
        updateSamplesPerStep();
    }

    void setTempo(float bpm)
    {
        tempo = bpm;
        updateSamplesPerStep();
    }

    void setStep(int index, float pitch, float velocity)
    {
        jassert(index >= 0 && index < numSteps);
        steps[index] = { pitch, velocity };
    }

    const SequencerStep& getStep(int index) const
    {
        return steps[index];
    }

    // Call once per sample. Returns true on a new step trigger.
    bool tick(int& outStepIndex)
    {
        sampleCounter++;
        if (sampleCounter >= samplesPerStep)
        {
            sampleCounter = 0;
            currentStep = (currentStep + 1) % numSteps;
            outStepIndex = currentStep;
            return true;
        }
        return false;
    }

    int getCurrentStep() const { return currentStep; }

    static constexpr int numSteps = 8;

private:
    void updateSamplesPerStep()
    {
        // 1 step = 1 sixteenth note
        // BPM = quarter notes per minute
        // sixteenth note duration = 60 / (BPM * 4) seconds
        if (sr > 0.0 && tempo > 0.0f)
            samplesPerStep = (int)(sr * 60.0 / (tempo * 4.0));
    }

    SequencerStep steps[numSteps];
    double sr            = 44100.0;
    float  tempo         = 120.0f;
    int    samplesPerStep = 0;
    int    sampleCounter  = 0;
    int    currentStep    = -1;
};
