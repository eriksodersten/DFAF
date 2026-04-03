#include <fstream>
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
            juce::NormalisableRange<float>(0.01f, 2.0f, 0.0f, 0.3f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("vcfEgAmt",    "VCF EG Amt",    0.0f,  1.0f,    0.0f));
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
        float noiseVcfMod   = apvts.getRawParameterValue("noiseVcfMod")->load();
    float noiseLevelVal = apvts.getRawParameterValue("noiseLevel")->load();
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
                            pendingStepIndex = currentStep;
                            sequencer.triggerNextStep(pendingStepIndex);
                            const auto& step = sequencer.getStep(pendingStepIndex);
                            voice.trigger(step.pitch, step.velocity, fmVal);
                        }
                    }
                else
                    {
                        lastStep = -1;
                    }

                auto frame = voice.processFrame();

                            // TRIM LOGGER – one-shot, ta bort efter mätning
                            bool active = frame.ampGain > 0.0f;
                            if (!measurementDone && active)
                            {
                                measuring = true;
                                ampGains.push_back(frame.ampGain);
                                double raw2 = (double)frame.raw * (double)frame.raw;
                                double amp2 = (double)frame.ampGain * (double)frame.ampGain;
                                sumRaw2   += raw2;
                                sumOldIn2 += raw2 * amp2;
                            }
                            else if (!measurementDone && wasActive && measuring)
                            {
                                std::sort(ampGains.begin(), ampGains.end());
                                float maxAmp = ampGains.back();
                                float p95    = ampGains[(size_t)(0.95 * (ampGains.size() - 1))];
                                double sumSq = 0.0, sum = 0.0;
                                for (float a : ampGains) { sum += a; sumSq += (double)a * a; }
                                double mean       = sum / ampGains.size();
                                double rms        = std::sqrt(sumSq / ampGains.size());
                                double trimEnergy = (sumRaw2 > 0.0) ? std::sqrt(sumOldIn2 / sumRaw2) : 0.0;
                                {
                                                    std::ofstream f("/tmp/dfaf_trim.txt");
                                                    f << "max=" << maxAmp << " p95=" << p95
                                                      << " mean=" << mean << " rms=" << rms
                                                      << " trimEnergy=" << trimEnergy << "\n";
                                                }
                                ampGains.clear();
                                sumRaw2 = sumOldIn2 = 0.0;
                                measuring = false;
                                measurementDone = true;
                            }
                            wasActive = active;

                            float noiseVal = noiseRandom.nextFloat() * 2.0f - 1.0f;
                            float modulatedCutoff = cutoffVal + vcfEgAmt * frame.vcfEnv * 7000.0f + noiseVcfMod * noiseVal * 3000.0f;
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
