# Phase 2: all engines, all voices (spec, draft for review)

Status: **reviewed** (2026-11-12): questions answered (section 5). Phase 2 of [port-plan.md](port-plan.md), after release 0.1 (FM on
Drum 1–3). Release **0.2** at the end. As in Phase 1, the code is the Eurorack's (LX_UMA `a179390`), the pages are
the Eurorack's, and each build is checked on the LXR before the next.

## 1. What comes

| Feature | Eurorack | Where on the desktop |
|---|---|---|
| **Envelopes** (Drum 1–3) | hold `hld`; gate mode `gat` with `sus` `rel`; filter envelope `env` | Amp EG and Filter pages, as on the Eurorack |
| **Low-pass gate** (Drum 1–3) | filter type `lpg` with `dcy` `col` `exc` | Filter page |
| **Wavefolder** (FM, mdk, Plaits) | `fld fty sym lev` | Transient page, screen 2 |
| **Randomise** (FM, mdk) | `rpl`, `rto rev rsw rsd`, `rfn`; for mdk `rmc rkn` | Mod page (question 2) |
| **mdk** | 30 drum machines (OP-XY-MNMD) on every voice | `eng` `mdk`; the snare, cymbal and hi-hat get an engine setting |
| **Plaits** | 12 models on every voice (no speech, as on the Eurorack), 6-op with the built-in banks A–C, TWIST, the sub on wavetable and terrain | `eng` `pla` |
| **Quantizer** | `oct key scl` per voice | the Osc page's last screen |

**Not in Phase 2:** prk (decided: no prk on the desktop); velocity `vel` and the mod matrix, LFO B, Turing Machines,
kit morph (Phase 3, they come with the modulation layer); card files: DX7 banks, wavetables, terrains from the card
(Phase 4: until then the 6-op has banks A–C and the wavetable and terrain their built-in tables).

## 2. The budget

Measured on the Eurorack build (its map file), and what's left on the desktop after 0.1:

| Part | Flash | RAM |
|---|---|---|
| Plaits (adapter + Mutable's code) | 157 KB | 42 KB |
| mdk | 113 KB | 5 KB |
| Envelopes, LPG, quantizer, randomise, voice code for the snare/cymbal/hi-hat | ~13 KB | ~6 KB |
| Menus and pages for all of it | ~15 KB (estimate) | — |
| **Phase 2 total** | **~300 KB** | **~53 KB** |
| **Left after 0.1** | **326 KB** | **~108 KB** (stack at the top of it) |
| Still to come: Phase 3 (modulation, automation) + Phase 4 (card files: 24 KB RAM) | ~30–40 KB | ~30 KB |

**RAM fits** (~25 KB left at the end). Flash was ~10–20 KB short by the end of Phase 4; with the operator waveforms
removed (question 1) **448 KB are spare** after 0.2a's first step: room for Phases 2–4.

**CPU:** a busy stock pattern is 67 % (`pk`). On the Eurorack each Plaits voice adds ~6 %, FM a little less, mdk
less. Decided in Phase 0: every engine on every voice, the manual says how many heavy voices are safe; silent voices
skip their engine (FM, Plaits' tonal models), as on the Eurorack.

## 3. Desktop differences

- **Gates.** On the Eurorack a gate is a trigger jack held high or a MIDI note held. The desktop's sequencer steps
  have no length, so its hits are triggers; **a held MIDI note** (DIN or USB) is the gate: gate mode, the 6-op's
  held key, FM Tone legato (question 3).
- **The snare, cymbal and hi-hat** get the engine setting on their Osc pages, as on the Eurorack (`lxr mdk pla`;
  no prk). The hi-hat's open trigger plays the machine's open version (mdk), as on the Eurorack.
- **The Mod page** in FM modes kept the desktop's velocity screen (`vdt vam vvo`) in 0.1; randomise needs room
  there too (question 2).
- **Kit records** gain the defaults of the new settings (mdk machines, Plaits models, quantizer off, envelopes as
  now): kits saved by 0.1 sound the same. Record format 3.

## 4. Builds

| Build | What | You test |
|---|---|---|
| **0.2a** | Flash room (question 1); hold, gate mode, filter envelope, LPG, wavefolder on Drum 1–3 | Hold and LPG as on the Eurorack; gate with MIDI notes; `pk` |
| **0.2b** | Randomise for FM (`rpl`, `rto rev rsw rsd`, `rfn`) | Re-rolls as on the Eurorack |
| **0.2c** | mdk on Drum 1–3, its randomise | The machines as on the Eurorack |
| **0.2d** | The snare, cymbal, hi-hat: engine setting, mdk on them | Pages; the open hi-hat |
| **0.2e** | Plaits on every voice: 12 models, 6-op A–C, TWIST, the sub | Models as on the Eurorack; `pk` with several Plaits voices |
| **0.2f** | Quantizer and the pitch screen | Scales as on the Eurorack |
| **0.2** | Release: manual chapters, notes | |

## 5. Decided (2026-11-12)

1. **Flash:** the FM operator waveforms (`wvc`, `wvm`) are **removed** on the desktop (the user doesn't use them):
   every operator is a sine, `FmWaves.c`'s 120 KB of tables aren't linked (`FMD_NO_WAVES` in `FmDrum.c`, the one
   change to the copied engine, `docs/SOURCES.md`). The kit record keeps the two settings (unused), so kits stay
   byte-compatible with the Eurorack's list. FM Drum's FM page loses its extra screen; FM Tone's last FM screen is
   `fna fnb`. Image 294,796 bytes, **448 KB spare**.
2. **Mod page** (FM, mdk): randomise on screen 2 as on the Eurorack; velocity (`vdt vam vvo`) moves to screen 3.
3. **Gates:** MIDI notes only; sequencer hits are triggers.

## 6. 0.2a as built

- **No operator waveforms** (section 5): 448 KB spare before the rest of 0.2a.
- **Envelopes and LPG** (Drum 1–3): the Eurorack's `uma_env.c` (adapted: no trigger jacks, a held MIDI note on DIN or
  USB is the gate; the desktop's track mute ends a gate), `Lpg.c` and its `SlopeEg2` (hold, sustain, release, gate
  end) copied unchanged. Pages as on the Eurorack: Amp EG `atk dec slp hld | sus rel gat`; Filter
  `frq res typ drv | env`, `typ` with `lpg` after `off`, then screen 2 is `dcy col exc`.
- **Wavefolder** on FM voices: Transient screen 2 `fld fty sym lev`; its drive follows the amp EG or the LPG.
- `tools/port_names.py`: ports menu names from the Eurorack's tables (used from here on).
- Image 304,168 bytes (~433 KB spare), RAM 269.5 KB, ITCM 15,224 B (+144: the envelope's hold and release).

## 7. 0.2b as built

- **Randomise on FM** (Drum 1–3): the Eurorack's `uma_random.c` and `uma_voicepar.c` (where each voice's settings
  are) copied unchanged; `uma_mod.h` is a stand-in until Phase 3 (a setting's value is the kit's). prk's randomise
  rules come with `PrkDrum.c`, but only `prkDrum_random` is linked (1.2 KB; no prk engine, no prk RAM).
- Runs once per audio block before the drums read their settings (`mixer.c`); a hit marks the drum; loading a kit
  (by hand, by a pattern, at boot from `tmp.snd`) makes it the restore point, saving keeps.
- **Mod page** (FM): `pd slp amt rpl | rto rev rsw rsd`, then `rfn vdt vam vvo` (random fine, then the desktop's
  velocity settings).
- Image 318,532 bytes; RAM 271.6 KB (+2 KB, randomise's restore copies).
