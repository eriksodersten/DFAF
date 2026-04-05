# DFAF – Claude-kontext

## Vad är projektet
DFAF är ett analogt trumsynt-plugin (VST3/AU/Standalone) byggt med C++17 och JUCE. En röst, inbyggd 8-stegs sequencer, Moog Ladder-filter.

## Signalkedja
```
Sequencer → VCO1/VCO2/Noise → VCF (MoogLadderFilter) → VCA → Output
```

## Filstruktur
- `Source/` — all källkod
- `build/` — cmake-build (används av deploy)
- `build-xcode/` — xcode-build (används EJ av deploy, ignorera)
- `deploy.sh` — bygger och kopierar AU till `~/Library/Audio/Plug-Ins/Components/`
- `PATCHING.md` — designregler för patchpanelen

## Regler för ändringar
1. **Redigera alltid filer i huvudrepon:** `/Users/eriksode/Projects/DFAF/Source/`
2. **Worktreen används EJ för edits**
3. **Commit och push från huvudrepon** på branch `vcf-before-vca`, pusha som main: `git push origin vcf-before-vca:main`
4. **Deploy efter varje edit:** `/usr/local/bin/cmake --build /Users/eriksode/Projects/DFAF/build && cp -r /Users/eriksode/Projects/DFAF/build/DFAF_artefacts/Debug/AU/DFAF.component ~/Library/Audio/Plug-Ins/Components/`
5. **Reviewa alltid** förslag innan kodning — fråga användaren om avvikelser

## Deploy (manuellt)
```bash
/usr/local/bin/cmake --build /Users/eriksode/Projects/DFAF/build
cp -r /Users/eriksode/Projects/DFAF/build/DFAF_artefacts/Debug/AU/DFAF.component ~/Library/Audio/Plug-Ins/Components/
```

## Git
- Working branch: `vcf-before-vca`, pushas som `origin/main`
- Remote: `eriksodersten/DFAF` på GitHub
- Senaste commit på main: `827abd1`

---

## Patchsystem – aktuellt läge (commit 827abd1)

### Arkitektur
- `PatchPoint` enum i `PluginProcessor.h` — indexerar `patchSourceValues[]` och `patchInputSums[]`
- `PatchPointMeta kPatchMeta[]` — inline const i headern, innehåller namn, `PD_Out`/`PD_In`, `bipolar`
- `PatchCable` struct — src, dst, amount (0..1), enabled
- `CableStore` — fixed-size array `[kMaxCables=16]` + count, skyddad av **seqlock**
  - `cableSeq` atomic: jämn = stabil, udda = skrivning pågår
  - `cableWriteLock` CriticalSection: serialiserar message-thread-skrivningar
  - Audio-tråden tar **aldrig lås** — seqlock-loop med retry
- API: `connectPatch()`, `disconnectPatch()`, `clearPatches()`, `getCableSnapshot()`
- Kablar serialiseras i `getStateInformation` / `setStateInformation`

### Implementerade patchpunkter
| PatchPoint     | Index | Dir | Bipolar | Signal                          |
|----------------|-------|-----|---------|--------------------------------|
| PP_VCF_EG      | 0     | OUT | false   | frame.vcfEnv (vel-skalad 0..1) |
| PP_VCF_MOD     | 1     | IN  | true    | multiplicativ cutoff-mod       |
| PP_VCA_EG      | 2     | OUT | false   | frame.ampGain (0..1)           |
| PP_VCA_CV      | 3     | IN  | false   | additiv VCA gain               |
| PP_VELOCITY    | 4     | OUT | false   | hållen step-velocity (0..1)    |
| PP_VCO_EG      | 5     | OUT | false   | frame.vcoEnv (0..1)            |
| PP_VCF_DECAY   | 6     | IN  | true    | additiv i normaliserad domän   |

### DSP-modeller per destination
- **PP_VCF_MOD**: `hasVcfModCable` → patch ersätter noise som källa; `noiseVcfMod`-ratten styr depth för båda. Formel: `cutoffVal * pow(2, noiseVcfMod * vcfModSignal * 2)`
- **PP_VCA_CV**: `vcaGain = jlimit(0, 1, frame.ampGain + patchInputSums[PP_VCA_CV])`
- **PP_VCF_DECAY**: panel sätter bas i normaliserad domän; CV adderas ovanpå. `hasVcfDecayCable` pre-computed från snapshot. `lastVcfDecayMod` samplas sista sample per block.

### GUI
- `getJackCentre(PatchPoint)` i PluginEditor.cpp — mappar patchpunkter till skärmkoordinater
- `jackHitTest()` + `mouseDown()` — klicka OUT-jack (gul ring) → gröna IN-jacks → klicka IN = connect; klicka igen = disconnect
- `drawJackPanel()` ritar kabellinjer (blå) + connected-ring (grön) + selected-ring (gul)

### Designregler (se PATCHING.md)
- Panelkontroll = grundvärde, patch-CV modulerar ovanpå (eller ersätter beroende på destination)
- VCF MOD: patch **ersätter** noise som källa
- Decay-inputs: patch **adderas** i normaliserad parameterdomän (panel är alltid bas)
- Outputs = verkliga interna signaler (inte parameteralias)
- Routing: endast OUT → IN

---

## Nästa steg (planerat)
1. `PP_VCO_DECAY` (IN) — samma additiva normaliserade modell som VCF DECAY
2. Fler patchpunkter: eventuellt `PP_VCO1_CV`, `PP_VCO2_CV`, `PP_NOISE_LEVEL`
3. LFO — låg prioritet, ej nu

## Viktiga DSP-beslut (äldre sessioner)
- `vcfAttackCoeff = ampDezipperCoeff` — 1ms tidskonstant
- `frame.vcfEnv` är velocity-skalad inuti `processFrame()`
- `DFAFSequencer::setCurrentStep()` används — `currentStep` är privat
- Noise CV-kedja: LP(291Hz) → HP(18Hz) → tanh(×1.5) → cutoff-mod
- Decay-rattar: `setSkewForCentre()` — VCO/VCA=0.40s, VCF=1.50s
- Cutoff-ratt: `setSkewForCentre(800Hz)` på 20–8000Hz
- `vcfEgAmt` formas med `sign(x)*x²`

## Arbetsflöde med ChatGPT
ChatGPT används parallellt för review. GitHub main ska alltid vara uppdaterad. Vid CDN-cache-problem: hämta med commit-hash direkt, t.ex. `https://raw.githubusercontent.com/eriksodersten/DFAF/827abd1/Source/PluginProcessor.h`
