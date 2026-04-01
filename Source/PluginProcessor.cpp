#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout DFAFProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>("vcoDecay",    "VCO Decay",     0.01f, 2.0f,    0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("vco1Freq",    "VCO 1 Freq",    20.0f, 2000.0f, 220.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("vco1EgAmt",   "VCO 1 EG Amt", -24.0f, 24.0f,   0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("fmAmount",    "FM Amount",     0.0f,  1.0f,    0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("vco2Freq",    "VCO 2 Freq",    20.0f, 2000.0f, 330.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("vco2EgAmt",   "VCO 2 EG Amt", -24.0f, 24.0f,   0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("noiseLevel",  "Noise Level",   0.0f,  1.0f,    0.2f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("vco1Level",   "VCO 1 Level",   0.0f,  1.0f,    0.6f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("vco2Level",   "VCO 2 Level",   0.0f,  1.0f,    0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("cutoff",      "Cutoff",        20.0f, 8000.0f, 800.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("resonance",   "Resonance",     0.0f,  1.0f,    0.4f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("vcfDecay",    "VCF Decay",     0.01f, 2.0f,    0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("vcfEgAmt",    "VCF EG Amt",    0.0f,  1.0f,    0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("noiseVcfMod", "Noise VCF Mod", 0.0f,  1.0f,    0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("vcaDecay",    "VCA Decay",     0.01f, 2.0f,    0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("vcaEg",       "VCA EG",        0.0f,  1.0f,    0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("volume",      "Volume",        0.0f,  1.0f,    0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("tempo",       "Tempo",         40.0f, 240.0f,  120.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("seqPitchMod", "SEQ Pitch Mod",
            juce::StringArray({ "VCO 1&2", "OFF", "VCO 2" }), 0));
        params.push_back(std::make_unique<juce::AudioParameterBool>("hardSync", "Hard Sync", false));

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
    sequencer.setTempo(120.0f);
    voice.prepare(sampleRate);
    filter.prepare(sampleRate);
    filter.setCutoff(800.0f);
    filter.setResonance(0.4f);
}

void DFAFProcessor::releaseResources() {}

void DFAFProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    buffer.clear();

    float tempo       = apvts.getRawParameterValue("tempo")->load();
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
        float noiseVcfMod   = apvts.getRawParameterValue("noiseVcfMod")->load();
    float noiseLevelVal = apvts.getRawParameterValue("noiseLevel")->load();
        float vco1LevelVal  = apvts.getRawParameterValue("vco1Level")->load();
        float vco2LevelVal  = apvts.getRawParameterValue("vco2Level")->load();

    sequencer.setTempo(tempo);
    filter.setResonance(resVal);
    voice.setDecayTime(vcoDecayVal);
    voice.setVcaDecayTime(vcaDecayVal);
    voice.setVcfDecayTime(vcfDecayVal);
    voice.setFmAmount(fmVal);
    voice.setVco1BaseFreq(vco1Freq);
        voice.setVco2BaseFreq(vco2Freq);
    voice.setSeqPitchRouting(seqPitchRouting);
        voice.setHardSync(apvts.getRawParameterValue("hardSync")->load() > 0.5f);
        voice.setVco1EgAmount(vco1EgAmt);
    voice.setVco2EgAmount(vco2EgAmt);
    voice.setVcfEgAmount(vcfEgAmt);
    voice.setNoiseLevel(noiseLevelVal);
        voice.setVco1Level(vco1LevelVal);
        voice.setVco2Level(vco2LevelVal);

    auto* left  = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        int stepIndex = 0;
        if (sequencer.tick(stepIndex))
        {
            const auto& step = sequencer.getStep(stepIndex);
            voice.trigger(step.pitch, step.velocity, fmVal);
        }
        float vcfEnv = voice.getVcfEnvValue();
                float noiseVal = noiseRandom.nextFloat() * 2.0f - 1.0f;
                float modulatedCutoff = cutoffVal + vcfEgAmt * vcfEnv * 7000.0f + noiseVcfMod * noiseVal * 3000.0f;
                modulatedCutoff = juce::jlimit(20.0f, 20000.0f, modulatedCutoff);
                filter.setCutoff(modulatedCutoff);
        float sample = filter.process(voice.process()) * volumeVal;
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
