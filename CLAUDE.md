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

## Regler för ändringar
1. **Redigera alltid filer i huvudrepon:** `/Users/eriksode/Projects/DFAF/Source/` — det är dessa cmake bygger
2. **Worktreen används EJ för edits** — den är bara för git/branch-hantering
3. **Commit och push från huvudrepon** på branch `vcf-before-vca`, pusha som main: `git push origin vcf-before-vca:main`
4. **Deploy efter varje edit:** `/usr/local/bin/cmake --build /Users/eriksode/Projects/DFAF/build && cp -r /Users/eriksode/Projects/DFAF/build/DFAF_artefacts/Debug/AU/DFAF.component ~/Library/Audio/Plug-Ins/Components/`
5. **Reviewa alltid** förslag innan kodning — fråga användaren om avvikelser från deras förslag

## Deploy (manuellt om hook inte kör)
```bash
/usr/local/bin/cmake --build /Users/eriksode/Projects/DFAF/build
cp -r /Users/eriksode/Projects/DFAF/build/DFAF_artefacts/Debug/AU/DFAF.component ~/Library/Audio/Plug-Ins/Components/
```

## Git-struktur
- Working branch: `main` (i worktreen)
- Remote: `origin/main` på GitHub (eriksodersten/DFAF)
- Commits görs i worktreen och pushas direkt: `git push origin main`

## Viktiga designbeslut (senaste sessionen)
- `vcfAttackCoeff = ampDezipperCoeff` — samma 1ms tidskonstant
- `frame.vcfEnv` är velocity-skalad inuti `processFrame()`
- `DFAFSequencer::setCurrentStep()` används — `currentStep` är privat
- Noise CV-kedja: LP(291Hz) → HP(18Hz) → tanh(×1.5) → cutoff-mod
- Decay-rattar använder `setSkewForCentre()`: VCO/VCA=0.40s, VCF=1.50s
- Cutoff-ratt: `setSkewForCentre(800Hz)` på 20–8000Hz
- `vcfEgAmt` formas med `sign(x)*x²` för mer upplösning nära 0
- Trim-loggern är borttagen
- Den målade hard sync-switchen i `paint()` är borttagen

## Arbetsflöde med ChatGPT
ChatGPT används parallellt för review. GitHub main ska alltid vara uppdaterad så att ChatGPT kan läsa aktuell kod.
