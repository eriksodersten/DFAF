#include "PluginProcessor.h"
#include "PluginEditor.h"


juce::AudioProcessorValueTreeState::ParameterLayout DFAFProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto vcoDecayRange = juce::NormalisableRange<float>(0.01f, 2.0f);
    vcoDecayRange.setSkewForCentre(0.40f);

    auto vcfDecayRange = juce::NormalisableRange<float>(0.01f, 10.0f);
    vcfDecayRange.setSkewForCentre(1.50f);

    auto vcaDecayRange = juce::NormalisableRange<float>(0.01f, 2.0f);
    vcaDecayRange.setSkewForCentre(0.40f);

    params.push_back(std::make_unique<juce::AudioParameterFloat>("vcoDecay",    "VCO Decay",
            vcoDecayRange, 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("vco1Freq",    "VCO 1 Freq",    20.0f, 2000.0f, 220.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("vco1EgAmt",   "VCO 1 EG Amt", -60.0f, 60.0f,   0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("fmAmount",    "FM Amount",     0.0f,  1.0f,    0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("vco2Freq",    "VCO 2 Freq",    20.0f, 2000.0f, 330.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("vco2EgAmt",   "VCO 2 EG Amt", -60.0f, 60.0f,   0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("noiseLevel",  "Noise Level",   0.0f,  1.0f,    0.2f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("vco1Level",   "VCO 1 Level",   0.0f,  1.0f,    0.6f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("vco2Level",   "VCO 2 Level",   0.0f,  1.0f,    0.2f));
    auto cutoffRange = juce::NormalisableRange<float>(20.0f, 8000.0f);
    cutoffRange.setSkewForCentre(800.0f);

    params.push_back(std::make_unique<juce::AudioParameterFloat>("cutoff",      "Cutoff",        cutoffRange, 800.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("resonance",   "Resonance",     0.0f,  1.0f,    0.4f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("vcfDecay",    "VCF Decay",
                vcfDecayRange, 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("vcfEgAmt",    "VCF EG Amt",   -1.0f,  1.0f,    0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("noiseVcfMod", "Noise VCF Mod", 0.0f,  1.0f,    0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("vcaDecay",    "VCA Decay",
            vcaDecayRange, 0.3f));
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
    noiseModCoeff   = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * 291.0f / (float)sampleRate);
    noiseModHpCoeff = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * 18.0f  / (float)sampleRate);
    smoothedNoiseMod = 0.0f;
    noiseModHpState  = 0.0f;
    for (int p = 0; p < PP_NUM_POINTS; ++p)
        patchSourceValues[p] = patchInputSums[p] = 0.0f;
    vcfDecayParam = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter("vcfDecay"));
    smoothedCutoff.reset(sampleRate, 0.01);
    smoothedVolume.reset(sampleRate, 0.01);
    smoothedVco1Level.reset(sampleRate, 0.01);
    smoothedVco2Level.reset(sampleRate, 0.01);
    smoothedCutoff.setCurrentAndTargetValue(apvts.getRawParameterValue("cutoff")->load());
    smoothedVolume.setCurrentAndTargetValue(apvts.getRawParameterValue("volume")->load());
    smoothedVco1Level.setCurrentAndTargetValue(apvts.getRawParameterValue("vco1Level")->load());
    smoothedVco2Level.setCurrentAndTargetValue(apvts.getRawParameterValue("vco2Level")->load());
}
void DFAFProcessor::releaseResources() {}

// =============================================================================
// Patch system API  (message thread – never called from audio thread)
// =============================================================================

// Helper: read store safely from message thread (writer lock already held or not needed
// because writer lock serialises all writes).
// We take writelock here so getCableSnapshot is also safe without holding it externally.


void DFAFProcessor::connectPatch(PatchPoint src, PatchPoint dst, float amount)
{
    jassert(kPatchMeta[src].dir == PD_Out);
    jassert(kPatchMeta[dst].dir == PD_In);
    const juce::ScopedLock sl(cableWriteLock);

    CableStore next = cableStore;   // start from current state
    for (int i = 0; i < next.count; ++i)
        if (next.data[i].src == src && next.data[i].dst == dst)
            { next.data[i].amount = amount; next.data[i].enabled = true; goto publish; }
    if (next.count < kMaxCables)
        next.data[next.count++] = { src, dst, amount, true };

publish:
    cableSeq.fetch_add(1, std::memory_order_release);   // odd: writing
    cableStore = next;
    cableSeq.fetch_add(1, std::memory_order_release);   // even: done
}

void DFAFProcessor::disconnectPatch(PatchPoint src, PatchPoint dst)
{
    const juce::ScopedLock sl(cableWriteLock);
    CableStore next = cableStore;
    int w = 0;
    for (int r = 0; r < next.count; ++r)
        if (!(next.data[r].src == src && next.data[r].dst == dst))
            next.data[w++] = next.data[r];
    next.count = w;

    cableSeq.fetch_add(1, std::memory_order_release);
    cableStore = next;
    cableSeq.fetch_add(1, std::memory_order_release);
}

void DFAFProcessor::clearPatches()
{
    const juce::ScopedLock sl(cableWriteLock);
    CableStore empty{};
    cableSeq.fetch_add(1, std::memory_order_release);
    cableStore = empty;
    cableSeq.fetch_add(1, std::memory_order_release);
}

void DFAFProcessor::getCableSnapshot(std::vector<PatchCable>& out) const
{
    // Message-thread read: take write lock so we never read a partial write
    const juce::ScopedLock sl(cableWriteLock);
    out.assign(cableStore.data, cableStore.data + cableStore.count);
}

// =============================================================================

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
    smoothedCutoff.setTargetValue(cutoffVal);
    smoothedVolume.setTargetValue(volumeVal);
    smoothedVco1Level.setTargetValue(apvts.getRawParameterValue("vco1Level")->load());
    smoothedVco2Level.setTargetValue(apvts.getRawParameterValue("vco2Level")->load());
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
    // VCF decay applied after cable snapshot (needs hasVcfDecayCable) – see below
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
    voice.setVcaEgAmount(vcaEgVal);
    voice.setVcaAttackTime(0.001f + vcaEgVal * 0.099f);

    // Seqlock snapshot – audio thread never takes a lock.
    // Retries only if a message-thread write lands exactly during the copy (extremely rare).
    CableStore cableSnap;
    uint32_t seq1, seq2;
    do {
        seq1 = cableSeq.load(std::memory_order_acquire);
        if (seq1 & 1u) continue;          // write in progress – retry
        cableSnap = cableStore;
        seq2 = cableSeq.load(std::memory_order_acquire);
    } while (seq1 != seq2);
    const int nCables = cableSnap.count;

    // Pre-compute which destinations have active cables (used in DSP below)
    bool hasVcfModCable   = false;
    bool hasVcfDecayCable = false;
    for (int c = 0; c < nCables; ++c)
    {
        if (!cableSnap.data[c].enabled) continue;
        if (cableSnap.data[c].dst == PP_VCF_MOD)   hasVcfModCable   = true;
        if (cableSnap.data[c].dst == PP_VCF_DECAY)  hasVcfDecayCable = true;
    }

    auto* left  = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        if (isPlaying)
        {
            double samplePpq = ppqPosition + (double)i * bpm / (60.0 * currentSampleRate);
            int hostStep = (int)(samplePpq / ppqPerStep) % DFAFSequencer::numSteps;

            if (sequencerResetPending.exchange(false, std::memory_order_acq_rel))
            {
                sequencerStepOffset = (DFAFSequencer::numSteps - hostStep) % DFAFSequencer::numSteps;
                lastStep = -1;
            }

            int currentStep = (hostStep + sequencerStepOffset) % DFAFSequencer::numSteps;

            if (currentStep != lastStep)
            {
                lastStep = currentStep;
                sequencer.setCurrentStep(currentStep);
                const auto& step = sequencer.getStep(currentStep);
                currentVelocity = step.velocity;
                patchSourceValues[PP_VELOCITY] = currentVelocity;
                voice.trigger(step.pitch, step.velocity);
            }
        }
        else
        {
            if (sequencerResetPending.exchange(false, std::memory_order_acq_rel))
            {
                sequencerStepOffset = 0;
                sequencer.setCurrentStep(0);
            }

            lastStep = -1;
        }

        voice.setVco1Level(smoothedVco1Level.getNextValue());
        voice.setVco2Level(smoothedVco2Level.getNextValue());
        auto frame = voice.processFrame();
        float cutoffNow = smoothedCutoff.getNextValue();
        float volumeNow = smoothedVolume.getNextValue();

        // --- Patch engine -------------------------------------------
        for (int p = 0; p < PP_NUM_POINTS; ++p) patchInputSums[p] = 0.0f;
        patchSourceValues[PP_VCF_EG]   = frame.vcfEnv;
        patchSourceValues[PP_VCA_EG]   = frame.ampGain;
        patchSourceValues[PP_VELOCITY] = currentVelocity;
        patchSourceValues[PP_VCO_EG]   = frame.vcoEnv;
        for (int c = 0; c < nCables; ++c)
            if (cableSnap.data[c].enabled)
                patchInputSums[cableSnap.data[c].dst] +=
                    patchSourceValues[cableSnap.data[c].src] * cableSnap.data[c].amount;
        // ------------------------------------------------------------

        // VCF decay: continuous CV from real patch sources in normalised parameter domain
        if (vcfDecayParam != nullptr)
        {
            float norm = vcfDecayParam->convertTo0to1(vcfDecayVal);
            if (hasVcfDecayCable)
            {
                constexpr float vcfDecayCvScale = 0.35f;
                norm = juce::jlimit(0.0f, 1.0f, norm + patchInputSums[PP_VCF_DECAY] * vcfDecayCvScale);
            }
            voice.setVcfDecayTime(vcfDecayParam->convertFrom0to1(norm));
        }
        else
        {
            voice.setVcfDecayTime(vcfDecayVal);
        }

        smoothedNoiseMod += (frame.noiseRaw - smoothedNoiseMod) * noiseModCoeff;
        noiseModHpState  += (smoothedNoiseMod - noiseModHpState) * noiseModHpCoeff;
        float bandLimitedNoiseMod = smoothedNoiseMod - noiseModHpState;
        float shapedNoiseMod = std::tanh(bandLimitedNoiseMod * 1.5f) / std::tanh(1.5f);
        float shapedVcfEgAmt = (vcfEgAmt >= 0.0f)
            ? (vcfEgAmt * vcfEgAmt)
            : -(vcfEgAmt * vcfEgAmt);

        float vcfEnvMod = frame.vcfEnv;
        float vcfEgHz = (shapedVcfEgAmt >= 0.0f)
            ? (shapedVcfEgAmt * vcfEnvMod * 8500.0f)
            : (shapedVcfEgAmt * vcfEnvMod * (cutoffNow - 20.0f));

        // VCF MOD: patched signal replaces noise as mod source;
        // noiseVcfMod knob sets depth for both paths.
        float vcfModSignal = hasVcfModCable
            ? juce::jlimit(-1.0f, 1.0f, patchInputSums[PP_VCF_MOD])
            : shapedNoiseMod;
        float noisedCutoff    = cutoffNow * std::pow(2.0f, noiseVcfMod * vcfModSignal * 2.0f);
        float modulatedCutoff = juce::jlimit(20.0f, 20000.0f, noisedCutoff + vcfEgHz);
        filter.setCutoff(modulatedCutoff);

        float vcaGain = juce::jlimit(0.0f, 1.0f, frame.ampGain + patchInputSums[PP_VCA_CV]);
        float sample  = filter.process(frame.raw * preTrimVal) * vcaGain * volumeNow;
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

    // Serialise patch cables as child elements
    auto* cablesXml = xml->createNewChildElement("PatchCables");
    {
        const juce::ScopedLock sl(cableWriteLock);
        for (int i = 0; i < cableStore.count; ++i)
        {
            const auto& c = cableStore.data[i];
            auto* el = cablesXml->createNewChildElement("Cable");
            el->setAttribute("src",     (int)c.src);
            el->setAttribute("dst",     (int)c.dst);
            el->setAttribute("amount",  c.amount);
            el->setAttribute("enabled", c.enabled ? 1 : 0);
        }
    }

    copyXmlToBinary(*xml, destData);
}

void DFAFProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (!xml) return;

    // Restore APVTS params (strip PatchCables child first so APVTS doesn't see it)
    if (auto* cablesXml = xml->getChildByName("PatchCables"))
        xml->removeChildElement(cablesXml, true);

    if (xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));

    // Restore cables – re-parse original XML (removeChildElement was non-owning)
    std::unique_ptr<juce::XmlElement> xml2(getXmlFromBinary(data, sizeInBytes));
    if (!xml2) return;
    CableStore next{};
    if (auto* cablesXml = xml2->getChildByName("PatchCables"))
    {
        for (auto* el : cablesXml->getChildIterator())
        {
            const int src = el->getIntAttribute("src", -1);
            const int dst = el->getIntAttribute("dst", -1);
            if (src < 0 || src >= PP_NUM_POINTS) continue;
            if (dst < 0 || dst >= PP_NUM_POINTS) continue;
            if (kPatchMeta[src].dir != PD_Out)   continue;
            if (kPatchMeta[dst].dir != PD_In)    continue;
            if (next.count >= kMaxCables)        break;
            next.data[next.count++] = {
                static_cast<PatchPoint>(src),
                static_cast<PatchPoint>(dst),
                (float)el->getDoubleAttribute("amount",  1.0),
                el->getIntAttribute("enabled", 1) != 0
            };
        }
    }
    const juce::ScopedLock sl(cableWriteLock);
    cableSeq.fetch_add(1, std::memory_order_release);
    cableStore = next;
    cableSeq.fetch_add(1, std::memory_order_release);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DFAFProcessor();
}
