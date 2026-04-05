# DFAF patching dev notes

Kortfattade regler för hur patchpanelen ska bete sig i DFAF, baserat på DFAM-manualens patchlogik.

## Grundregel

**När en kabel kopplas in ersätter patch-CV panelkontrollens värde.**

Panelkontrollen fungerar som grundvärde endast när inget är kopplat.

---

## 1. Routing

Tillåt bara:

- **OUT -> IN**

Tillåt inte:

- OUT -> OUT
- IN -> IN

---

## 2. Patch outputs

Outputs ska vara **verkliga interna signaler/CV-källor**, t.ex.:

- `VCF EG`
- `VCA EG`
- `VCO EG`
- `VELOCITY`
- `PITCH`

Inte abstrakta parameteralias.

---

## 3. Vanlig input-regel

För de flesta patch-inputs gäller:

> **patch-CV ersätter panelens grundvärde**

Detta följer DFAM-logiken där en inkopplad kabel tar över kontrollen från panelen. Gäller bland annat:

- `VCA DECAY`
- `VCF DECAY`
- `VCO DECAY`
- `VCA CV`
- `VCO 1 CV`
- `VCO 2 CV`
- `NOISE LEVEL`
- `TEMPO`
- `12 FM AMT`

Alltså normalt:

```text
effektivt värde = patch-CV        (när kabel är kopplad)
effektivt värde = panel-grundvärde (när inget är kopplat)
```

---

## 4. Decay-inputs (normaliserad parameterdomän)

Decay-inputs följer **additivmodellen**: panelratten sätter alltid basvärdet och patch-CV läggs ovanpå, i normaliserad parameterdomän:

```text
norm = convertTo0to1(panel-värde)        (panel är alltid bas)
norm = clamp(norm + patch-CV, 0, 1)      (CV modulerar ovanpå)
effektiva sekunder = convertFrom0to1(norm)
```

Skäl: DFAM-manualen beskriver decay-CV som summerad med panelratten. Normaliserad domän ger proportionell modulationsdjup oavsett rattens basvärde (tack vare skew-kurvan).

Skäl: skew-kurvan (setSkewForCentre) gör att modulation i sekunder ger inkonsekvent djup över rattens range. Normaliserad domän ger proportionell känsla oavsett grundvärde.

---

## 5. Signaldomäner

| Källa       | Domän      | Kommentar                         |
|-------------|------------|-----------------------------------|
| VCF EG      | 0..1       | velocity-skalad                   |
| VCA EG      | 0..1       | smoothad ampGain                  |
| VCO EG      | 0..1       | smoothad VCO-kurva                |
| VELOCITY    | 0..1       | hålls från trigger till nästa     |
| PITCH       | (framtida) | normaliserad MIDI-not             |

Bipolära källor (t.ex. framtida LFO) markeras `bipolar: true` i `PatchPointMeta`.

---

## 6. Implementationsmodell (aktuell)

- `PatchPoint` enum indexerar `patchSourceValues[]` och `patchInputSums[]`
- `kPatchMeta[]` innehåller namn, riktning (`PD_Out`/`PD_In`) och polaritet (`bipolar`)
- Kabelstore: seqlock med dubbelbuffrad `CableStore[kMaxCables]` — audio-tråden tar aldrig lås
- `connectPatch` / `disconnectPatch` / `clearPatches` kallas från message-tråden
- Kablar serialiseras i `getStateInformation` / `setStateInformation`
