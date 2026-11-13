# LX_UMA desktop 0.2

Every engine on every voice. See the [manual](manual.md).

## New

- **mdk** (drum machines): thirty machines after the Elektron Machinedrum (OP-XY-MNMD, MIT) on every voice, chosen by
  type and machine; the FM page shows each machine's own knobs by name; randomise picks machines and knobs.
- **Plaits** on every voice: twelve models (Mutable Instruments, MIT) in two banks, `hrm tmb mor`, TWIST, the outputs,
  the 6-op FM with banks A–C and its key mode, the sub on wavetable and terrain.
- **The snare, cymbal and hi-hat** get an engine setting (`lxr`, `mdk`, `pla`) and the filter envelope.
- **Envelopes** (Drum 1–3): hold, gate mode (sustain and release while a MIDI note is held, DIN or USB), the filter
  envelope. **Low-pass gate**: filter type `lpg`, after Plaits.
- **Wavefolder** on FM, mdk and Plaits voices.
- **Randomise** on FM Drum and FM Tone (tone, envelopes, sweep, fine, the ratio palette) and on mdk.
- **Quantizer**: octave, key and scale (51 scales and chords) on every voice, the Osc page's last screen.

## Changed

- **No FM operator waveforms** on the desktop (`wvc`, `wvm`): every operator is a sine. Their 120 KB of tables made
  room for the engines. Kits keep the two settings, unused.
- The FM Mod page: randomise on screen 2 (as on the Eurorack), random fine and the velocity settings on screen 3.
- Plaits and mdk are compiled with the same float flags as the rest (`-ffast-math`): the heaviest Plaits models
  (modal resonator, harmonic, phase distortion) went from 90 %+ with clicks to about 83 % on a busy pattern.

## Notes

- Kits saved by 0.1 load and sound the same; their records get the new settings' defaults once.
- On the drums `eng` lists `lxr fmd fmt --- mdk pla`: the `---` place (the Eurorack's prk) is skipped.
- Not yet: modulation (mod matrix, LFO B, Turing Machines, velocity, MIDI CC sources), kit morph, sequencer automation
  and MIDI for UMA settings, DX7 banks, wavetables and terrains from the card.
- Back up your SD card before installing.
