# DFAF

DFAF is a semi-modular percussion synth plugin with an 8-step sequencer, patch panel routing, dual oscillators, filter modulation, and MIDI control support.

## Build

To build and deploy the current plugin locally:

```bash
./deploy.sh
```

## Presets

DFAF now has an internal preset footer in the plugin UI.

- `Init` resets all parameters and clears the patch panel.
- Selecting a saved preset from the preset menu loads it immediately.
- `SAVE` overwrites the current saved preset, or prompts for a new preset filename when the current state is `Init` or unsaved.
- `DELETE` removes the currently selected saved preset.

User presets are stored in:

```text
~/Library/Application Support/DFAF/Presets
```

## MIDI CC Map

The current MIDI implementation uses a fixed CC map. Continuous parameters follow CC `0..127` directly. Switches and combo-box style parameters are quantized to their nearest valid state.

### Knobs

| CC | Parameter |
| --- | --- |
| 20 | VCO Decay |
| 21 | VCO 1 EG Amt |
| 22 | VCO 1 Freq |
| 23 | VCO 1 Level |
| 24 | Noise Level |
| 25 | Cutoff |
| 26 | Resonance |
| 27 | VCA EG |
| 28 | Volume |
| 29 | FM Amount |
| 30 | VCO 2 EG Amt |
| 31 | VCO 2 Freq |
| 32 | VCO 2 Level |
| 33 | VCF Decay |
| 34 | VCF EG Amt |
| 35 | Noise VCF Mod |
| 36 | VCA Decay |

### Sequencer

| CC | Parameter |
| --- | --- |
| 37 | Step Pitch 1 |
| 38 | Step Pitch 2 |
| 39 | Step Pitch 3 |
| 40 | Step Pitch 4 |
| 41 | Step Pitch 5 |
| 42 | Step Pitch 6 |
| 43 | Step Pitch 7 |
| 44 | Step Pitch 8 |
| 45 | Step Velocity 1 |
| 46 | Step Velocity 2 |
| 47 | Step Velocity 3 |
| 48 | Step Velocity 4 |
| 49 | Step Velocity 5 |
| 50 | Step Velocity 6 |
| 51 | Step Velocity 7 |
| 52 | Step Velocity 8 |

### Switches And Menus

| CC | Parameter |
| --- | --- |
| 53 | Seq Pitch Mod |
| 54 | Hard Sync |
| 55 | VCO 1 Wave |
| 56 | VCO 2 Wave |
| 57 | VCF Mode |
| 58 | Clock Mult |
