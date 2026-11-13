# Mutable Instruments code (MIT)

Code by Émilie Gillet / Mutable Instruments, used **unchanged** by UMA's `pla` engine
(`../PlaitsDrum.cpp`, design in `docs/design/plaits-engine.md`).

| Folder | From | What |
|---|---|---|
| `plaits/dsp/`, `plaits/resources.*` | [pichenettes/eurorack](https://github.com/pichenettes/eurorack) `plaits/` | Plaits' synthesis engines and their tables |
| `stmlib/` (a subset) | [pichenettes/stmlib](https://github.com/pichenettes/stmlib) | The DSP helpers Plaits uses |

The commits are in `UPSTREAM_COMMIT`. Each file keeps its own MIT notice; `LICENSE` repeats it. Not copied:
Plaits' hardware, UI and user-data files (`plaits/user_data.h` needs the module's flash driver and nothing here uses
it).

"Mutable Instruments" is a registered trademark and is not used as a name for anything in UMA; per the authors'
guidelines for derivative works, the code is credited, not the brand.

Any change must be a minimal, marked patch listed here and in `NOTICE.md`. The patches, all marked
`UMA: TWIST (from Plaits Palette, lylepmills/eurorack, MIT)`:

**TWIST (UMA 0.33l)**: Plaits Palette's fourth macro, its formulas taken from
[lylepmills/eurorack](https://github.com/lylepmills/eurorack) (commit 83f86ae; Émilie Gillet's MIT files with Rubato
Audio's changes). At `macro` 0.5, the default, every engine renders bit for bit as Mutable's
(`docs/design/plaits-engine.md`, 0.33l).

| File | Change |
|---|---|
| `plaits/dsp/engine/engine.h` | `EngineParameters::macro` (default 0.5) and `ApplyMacro` |
| `plaits/dsp/engine/wavetable_engine.cc` | phase warp (and the `sine_oscillator.h` include) |
| `plaits/dsp/engine/modal_engine.cc`, `physical_modelling/modal_voice.{h,cc}` | the exciter's Q (a trailing argument, 0 = stock) |
| `plaits/dsp/physical_modelling/string_voice.{h,cc}` | the pluck burst's length (a trailing argument, 1 = stock) |
| `plaits/dsp/engine2/wave_terrain_engine.{h,cc}` | the path's Y offset |
| `plaits/dsp/engine/virtual_analog_engine.cc` | the saw / square balance |
| `plaits/dsp/engine/waveshaping_engine.cc` | the folder's symmetry |
| `plaits/dsp/engine2/phase_distortion_engine.cc` | the modulator's octave |
| `plaits/dsp/engine/additive_engine.{h,cc}` | odd / even gains (trailing arguments, 1 = stock) |
| `plaits/dsp/engine/chord_engine.cc` | the root's level in `out` |
| `plaits/dsp/engine/speech_engine.{h,cc}` | the output sharpening / fade |
| `plaits/dsp/fm/voice.h` | `Parameters::modulator_detune` (default 0) |
