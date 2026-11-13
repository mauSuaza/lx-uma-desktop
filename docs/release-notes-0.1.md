# LX_UMA desktop 0.1

The first release of LX_UMA for the LXR-02 desktop drum machine.

## New

- **`eng` on Drum 1–3** (Osc page, after `fin`): `lxr` (the LXR's oscillators, as before), `fmd`, `fmt`.
- **FM Drum (`fmd`):** three operators after the Digitone II's FM DRUM: 7 algorithms, 51 ratios (whole numbers,
  intervals, metallic), fine ratios, operator envelopes, feedback, operator waveforms.
- **FM Tone (`fmt`):** four operators after the Digitone's FM TONE: 8 algorithms, ratios for every operator, B2 with
  its own envelope, detune, X/Y mix, a sub oscillator (shape, octave, fold).
- The FM pages are the Eurorack firmware's ([manual](manual.md)). The voice's pitch envelope, transient, filter, amp
  envelope, drive and velocity work on the FM voices. A voice whose amp envelope has died away doesn't run its engine.
- **Kits** keep their UMA settings, appended to the kit file; `UMAKITS.DAT` on the card is the working copy. Projects
  saved by LX_UMA load on Erica's firmware (as stock sounds); Erica's projects load with every drum on `lxr`.
- **CPU readout** `pk` on the global page's 2nd screen.
- The start-up screen shows "LX_UMA v0.1" and the base firmware's version.

## Not yet

Hold and gate modes, filter envelope, low-pass gate, wavefolder, randomise, the mdk and Plaits engines, the
quantizer, modulation (mod matrix, LFO B, Turing Machines, MIDI CC), kit morph, sequencer automation and MIDI for UMA
settings, card files. They follow, as on the Eurorack firmware.

## Notes

- UMA settings aren't sent or received as MIDI CC yet.
- Back up your SD card before installing.
