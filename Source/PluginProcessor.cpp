#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout DFAFProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>("vcoDecay",    "VCO Decay",
            juce::NormalisableRange<float>(0.01f, 2.0f, 0.0f, 0.3f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("vco1Freq",    "VCO 1 Freq",    20.0f, 2000.0f, 220.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("vco1EgAmt",   "VCO 1 EG Amt", -60.0f, 60.0f,   0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("fmAmount",    "FM Amount",     0.0f,  1.0f,    0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("vco2Freq",    "VCO 2 Freq",    20.0f, 2000.0f, 330.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("vco2EgAmt",   "VCO 2 EG Amt", -60.0f, 60.0f,   0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("noiseLevel",  "Noise Level",   0.0f,  1.0f,    0.2f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("vco1Level",   "VCO 1 Level",   0.0f,  1.0f,    0.6f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("vco2Level",   "VCO 2 Level",   0.0f,  1.0f,    0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("cutoff",      "Cutoff",        20.0f, 8000.0f, 800.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("resonance",   "Resonance",     0.0f,  1.0f,    0.4f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("vcfDecay",    "VCF Decay",
                juce::NormalisableRange<float>(0.01f, 10.0f, 0.0f, 0.3f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("vcfEgAmt",    "VCF EG Amt",   -1.0f,  1.0f,    0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("noiseVcfMod", "Noise VCF Mod", 0.0f,  1.0f,    0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("vcaDecay",    "VCA Decay",
            juce::NormalisableRange<float>(0.01f, 2.0f, 0.0f, 0.3f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("vcaEg",       "VCA EG",        0.0f,  1.0f,    0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("preTrim",     "Pre Trim",      0.1f,  2.0f,    0.84f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("volume",      "Volume",        0.0f,  1.0f,    0.8f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("clockMult", "Clock Multiplier",
                juce::StringArray({ "1/8", "1/5", "1/4", "1/3", "1/2", "1x", "2x", "3x", "4x", "5x" }), 5));
    for (int i = 0; i < 8; ++i)
        {
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                            "stepPitch"    + juce::String(i), "Step Pitch "    + juce::String(i+1), 0.0f, 120.0f, 60.0f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                "stepVel"      + juce::String(i), "Step Velocity " + juce::String(i+1), 0.0f,  1.0f,  0.8f));
        }
        params.push_back(std::make_unique<juce::AudioParameterChoice>("seqPitchMod", "SEQ Pitch Mod",
            juce::StringArray({ "VCO 1&2", "OFF", "VCO 2" }), 0));
    params.push_back(std::make_unique<juce::AudioParameterBool>("hardSync", "Hard Sync", false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("vco1Wave", "VCO 1 Wave",
            juce::StringArray({ "Square", "Triangle" }), 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("vco2Wave", "VCO 2 Wave",
                juce::StringArray({ "Square", "Triangle" }), 0));
        params.push_back(std::make_unique<juce::AudioParameterChoice>("vcfMode", "VCF Mode",
                juce::StringArray({ "LP", "HP" }), 0));

    return { params.begin(), params.end() };
}

DFAFProcessor::DFAFProcessor()
    : AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "DFAF", createParameterLayout())
{}

DFAFProcessor::~DFAFProcessor() {}

void DFAFProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    sequencer.prepare(sampleRate);
    voice.prepare(sampleRate);
    filter.prepare(sampleRate);
    filter.setCutoff(800.0f);
    filter.setResonance(0.4f);
    noiseModCoeff = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * 291.0f / (float)sampleRate);
}

void DFAFProcessor::releaseResources() {}

void DFAFProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();
    juce::ignoreUnused(midiMessages);

    float bpm = 120.0f;
        bool isPlaying = false;
        double ppqPosition = 0.0;
    if (auto* ph = getPlayHead())
            {
                if (auto pos = ph->getPosition())
                {
                    if (pos->getBpm().hasValue())
                        bpm = (float)*pos->getBpm();
                    isPlaying = pos->getIsPlaying();
                    if (pos->getPpqPosition().hasValue())
                        ppqPosition = *pos->getPpqPosition();
                }
            }

    int multIndex = (int)apvts.getRawParameterValue("clockMult")->load();
            const float multTable[] = { 8.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f, 0.5f, 1.0f/3.0f, 0.25f, 0.2f };
            float mult = multTable[multIndex];
            float ppqPerStep = 0.25f * mult;
    float cutoffVal   = apvts.getRawParameterValue("cutoff")->load();
    float resVal      = apvts.getRawParameterValue("resonance")->load();
    float volumeVal   = apvts.getRawParameterValue("volume")->load();
    float vcoDecayVal = apvts.getRawParameterValue("vcoDecay")->load();
    float vcaDecayVal = apvts.getRawParameterValue("vcaDecay")->load();
    float vcfDecayVal = apvts.getRawParameterValue("vcfDecay")->load();
    float fmVal       = apvts.getRawParameterValue("fmAmount")->load();
    float vco1Freq       = apvts.getRawParameterValue("vco1Freq")->load();
        float vco2Freq       = apvts.getRawParameterValue("vco2Freq")->load();
        int   seqPitchRouting = (int)apvts.getRawParameterValue("seqPitchMod")->load();
        float vco1EgAmt      = apvts.getRawParameterValue("vco1EgAmt")->load();
    float vco2EgAmt   = apvts.getRawParameterValue("vco2EgAmt")->load();
    float vcfEgAmt      = apvts.getRawParameterValue("vcfEgAmt")->load();
        float noiseVcfMod   = apvts.getRawParameterValue("noiseVcfMod")->load();    float noiseLevelVal = apvts.getRawParameterValue("noiseLevel")->load();
    float vco1LevelVal  = apvts.getRawParameterValue("vco1Level")->load();
        float vco2LevelVal  = apvts.getRawParameterValue("vco2Level")->load();
    float vcaEgVal      = apvts.getRawParameterValue("vcaEg")->load();
        float preTrimVal    = apvts.getRawParameterValue("preTrim")->load();

    for (int i = 0; i < 8; ++i)
        {
            float pitch = apvts.getRawParameterValue("stepPitch" + juce::String(i))->load();
            float vel   = apvts.getRawParameterValue("stepVel"   + juce::String(i))->load();
            sequencer.setStep(i, pitch, vel);
        }

        filter.setResonance(resVal);
    voice.setDecayTime(vcoDecayVal);
    voice.setVcaDecayTime(vcaDecayVal);
    voice.setVcfDecayTime(vcfDecayVal);
    voice.setFmAmount(fmVal);
    voice.setVco1BaseFreq(vco1Freq);
        voice.setVco2BaseFreq(vco2Freq);
    voice.setSeqPitchRouting(seqPitchRouting);
    voice.setHardSync(apvts.getRawParameterValue("hardSync")->load() > 0.5f);
    voice.setVco1Wave((int)apvts.getRawParameterValue("vco1Wave")->load());
        voice.setVco2Wave((int)apvts.getRawParameterValue("vco2Wave")->load());
        filter.setHighpass((int)apvts.getRawParameterValue("vcfMode")->load() == 1);
        voice.setVco1EgAmount(vco1EgAmt);
    voice.setVco2EgAmount(vco2EgAmt);
    voice.setVcfEgAmount(vcfEgAmt);
    voice.setNoiseLevel(noiseLevelVal);
    voice.setVco1Level(vco1LevelVal);
        voice.setVco2Level(vco2LevelVal);
    voice.setVcaEgAmount(vcaEgVal);
    voice.setVcaAttackTime(0.001f + vcaEgVal * 0.099f);
    auto* left  = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                if (isPlaying)
                    {
                        double samplePpq = ppqPosition + (double)i * bpm / (60.0 * currentSampleRate);
                        int currentStep = (int)(samplePpq / ppqPerStep) % 8;

                        if (currentStep != lastStep)
                        {
                            lastStep = currentStep;
                            sequencer.currentStep = currentStep;
                            const auto& step = sequencer.getStep(currentStep);
                            voice.trigger(step.pitch, step.velocity, fmVal);
                        }
                    }
                else
                    {
                        lastStep = -1;
                    }

                auto frame = voice.processFrame();

                smoothedNoiseMod += (frame.noiseRaw - smoothedNoiseMod) * noiseModCoeff;
                float vcfEnvMod = frame.vcfEnv * voice.getVelocity();
                float vcfEgHz = (vcfEgAmt >= 0.0f)
                    ? (vcfEgAmt * vcfEnvMod * 8500.0f)
                    : (vcfEgAmt * vcfEnvMod * (cutoffVal - 20.0f));
                float noisedCutoff = cutoffVal * std::pow(2.0f, noiseVcfMod * smoothedNoiseMod * 2.0f);
                float modulatedCutoff = noisedCutoff + vcfEgHz;
                modulatedCutoff = juce::jlimit(20.0f, 20000.0f, modulatedCutoff);
                filter.setCutoff(modulatedCutoff);
                float sample = filter.process(frame.raw * preTrimVal) * frame.ampGain * volumeVal;
        left[i]  = sample;
        right[i] = sample;
    }
}

juce::AudioProcessorEditor* DFAFProcessor::createEditor()
{
    return new DFAFEditor(*this);
}

void DFAFProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void DFAFProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DFAFProcessor();
}
