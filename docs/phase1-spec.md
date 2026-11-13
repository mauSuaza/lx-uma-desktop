# Phase 1: foundation + FM (spec, draft for review)

Status: **reviewed** (2026-11-12): option A and appended kit files chosen; 0.1a built. Phase 1 of [port-plan.md](port-plan.md): UMA's settings and kit format on
the desktop, the menu framework, and FM Drum / FM Tone on Drum 1–3. Release **0.1** at the end.

## 1. What Phase 0 found (the numbers this spec works from)

| | Stock (0.0a) | 0.0c (engines in, no menus) | Limit |
|---|---|---|---|
| Flash (whole image) | 275 KB | 695 KB | 736 KB (sector 7 isn't erased: 0.0b2) |
| RAM (main, 384 KB) | 263 KB | 351 KB (24 KB left for the stack) | |
| DTCM (128 KB) | 105 KB | 109 KB | |
| ITCM (16 KB) | 15.0 KB | 15.1 KB | |
| CPU `pk` | 67 % | 83 % (3 engines always running) | 100 % |

## 2. Decided (2026-11-12)

- **Flash:** compile the non-audio code for size first (menus, FatFs, HAL, inputs, MIDI, as the Eurorack's 0.33l:
  ~30 KB there, no change in sound). The FM waveform tables shrink only if that isn't enough.
- **Card files:** 6 slots (24 KB, saves 16 KB of RAM).
- **CPU:** engines allowed on every voice; silent voices skip their engine (as on the Eurorack); the manual says how
  many heavy voices are safe; `pk` shows the load.

## 3. To decide: where the kits' UMA settings live (RAM)

**The problem.** The desktop keeps all 64 kits of the project in RAM (`mainParameters`, 64 × 247 bytes = 15.8 KB),
so that a pattern change can switch kits at once, from the sequencer, in the audio loop (`PAR_PATTERN_MODE` auto,
song mode). The factory projects use all 64 kits. UMA's settings add about **800 bytes per kit** (FM, Plaits, mdk,
envelopes, mod slots, Turing Machines, LFO B, quantizer; the Eurorack's kit is 1,148 bytes, minus its CV and prk
settings): **about 51 KB for 64 kits**. With the UMA core's ~32 KB, that is ~83 KB more against ~40 KB free (24 KB +
16 KB from the card slots). Section 2's earlier estimate (+30 KB) was too low.

**Options:**

| | How | Cost | Limits |
|---|---|---|---|
| **A. UMA part from the card, prefetched** | RAM keeps the stock part of all 64 kits, as now, plus the UMA part of **3 kits**: the playing one, the next one, the morph kit (~2.4 KB). The next pattern's kit is read from the card *before* the pattern starts: a manual pattern change is queued until the bar ends; song mode knows its next entry. Loading a kit by hand reads it from the card (a few ms, audio from the interrupt as during saves) | ~3 KB RAM; a prefetch in the main loop; the project's kits stay as files in its folder (they already are: `00-NAME.SND`) | A pattern chosen in the last few ms of a bar plays its first notes with the old kit's UMA part (the stock part still switches at once). To measure: the card's read time |
| **B. UMA settings for 16 kits** | Kits 1–16 hold UMA settings in RAM (13 KB), kits 17–64 are stock only (their Drum 1–3 play the LXR engine) | 13 KB RAM, simple | Projects with kit-per-pattern beyond 16 lose UMA there |
| **C. All 64 in RAM, made smaller** | Pack the settings (Turing Machine patterns, unused slots) to ~500 bytes | still ~32 KB | Not enough on its own; complex |

**Recommendation: A.** All 64 kits keep everything; RAM grows by ~3 KB, not 51. The risk is the card's read time, so
**0.1a measures it first** (a kit's UMA part read from the card while a pattern plays, timed, and `pk`). If it turns
out too slow, B is the fallback.

## 4. Settings and kit files

- **Numbering:** the desktop's parameters 0–301 stay as they are (patterns, automation, MIDI and stock kits use these
  numbers). UMA's settings are numbered from 302 up, in the Eurorack's order, minus the Eurorack-only ones (CV
  inputs, prk). The menu code already takes 16-bit parameter numbers; names above 255 come from the Eurorack
  (`UMA_NAME_TEST`, 0.27n).
- **Kit file:** the stock 255 bytes (8-byte header + 247 settings), then UMA's settings with the kit format byte.
  A stock desktop firmware reads only the first 255 bytes, so UMA kits still load there (as stock sounds). Stock kits
  (shorter files) load in UMA with defaults: every voice on its stock engine.
- **Kit format:** starts at 1 on the desktop (its own numbering; the Eurorack's conversions of formats 9–40 aren't
  needed, there are no older desktop UMA kits).
- **Morph:** stock morph covers parameters below 256; UMA's kit morph (Eurorack 0.26) comes in Phase 3.

## 5. Menus

- **Framework from the Eurorack:** engine-dependent pages (`menu_getPage`: a voice's Osc/FM/Mod pages change with
  its `eng`), 16-bit names, the extra value types (ratios, algorithms, waveform sweeps), each ported onto the
  desktop's `menu.c`, which differs from the Eurorack's by ~950 lines. The desktop's buttons and LEDs stay as they are.
- **Phase 1 shows:** `eng` (`lxr` / `fmd` / `fmt`) on Drum 1–3 and the FM pages as on the Eurorack (FM page, ratios,
  operator envelopes, waveforms, FM Tone's sub). Envelopes, LPG, folder, randomise, the other engines and the
  modulation pages follow in Phases 2–3.
- **CPU readout:** stays where it is (global page, 2nd sub-page).

## 6. FM on Drum 1–3

The Eurorack's voice integration (`DrumVoice.c` + `uma_voice.c`: settings → engine each block, the silent-voice
skip, the trigger) replaces 0.0c's test hook. The engine's code stays unchanged (`docs/SOURCES.md`). ITCM: the voice
code needs ~3.6 KB more than the 0.9 KB left, so the engines' per-block setup moves to flash (`noinline`, as on the
Eurorack) and, if still needed, a stock function or two leaves ITCM (CPU measured).

## 7. Builds

| Build | What | Module test |
|---|---|---|
| **0.1a** | Size: UI/FatFs/HAL/IO/MIDI at -Os; card pool 6 slots; 0.0c's engines stay. **Card read test** for option A: a kit's UMA-sized block read while a pattern plays, the time shown on the CPU screen | Sound and `pk` unchanged; the read time |
| **0.1b** | Settings and kit files (section 4, and A or B); no new menus yet | Stock kits and projects load and save unchanged; a UMA kit loads on a stock desktop firmware (0.0a2) |
| **0.1c** | Menu framework; `eng` on Drum 1–3 | Pages switch with `eng`; stock pages unchanged |
| **0.1d** | FM Drum and FM Tone with their pages; the test hook gone | Same settings sound as on the Eurorack (A/B); `pk` |
| **0.1** | Release: manual chapter, README, GitHub release | |

## 8. Decided (2026-11-12)

1. Kit settings: **option A** (the UMA part from the card, prefetched). B stays the fallback if 0.1a's read time is
   too slow.
2. UMA's settings are **appended** to the stock kit file; on stock firmware a UMA kit plays as a stock sound.

## 9. 0.1a as built

- `-Os` for the HAL, FatFs, USB MIDI, `lxr_ui`, `lxr_io`, the MIDI parsers (not `ParameterArray.c`, ITCM) and
  `initKit.c`; the sequencer and the sound stay `-O3`. Card pool `UMA_SDF_SLOTS` 6 (`docs/SOURCES.md`).
- Image 670,176 bytes, ends at 0x080AB9E0: **81.5 KB spare** (0.0c: 57). RAM 336 KB (0.0c: 351). ITCM unchanged.
- Read test `Core/Src/uma_sdreadtest.c`: twice a second while the CPU screen shows, `NN-*.snd` found, opened,
  1,055 bytes read, closed, audio from the interrupt; the longest of the last 2 s shows as `rd` in tenths of a ms in
  `tou`'s place (2nd sub-page: `ptm tin rd pk`).
- `-Os` shows a stock bug (menu.c, clear-automation dialog: "autom. 1]?" is one byte too long for the line, its
  terminator lands in the next line's first byte, which is redrawn): left as is.
- **Result (2026-11-12, user's LXR-02):** `rd` **15 = 1.5 ms** for the slowest kit read: option A holds (a bar at
  120 BPM is 2,000 ms). Most kits clean, `pk` 70-80 with the FX. Kits 13 NERVOUS and 30 SAWER clip a lot, `pk` above
  90 (the user won't use the FX on the desktop). Open: CPU over-run or level? (0.0c's engines run on Drum 1-3 every
  block at fixed, loud settings; the real voice skips silent engines.)

## 10. 0.1b as built: the kits' UMA records (option A)

- **The record:** 1,024 bytes per kit (`UMA_KIT_BYTES`, `Core/Inc/uma_kits.h`): byte 0 the format (1), then UMA's
  settings (Phase 1c on; PAR_UMA_* = `PAR_UMA_BLOCK` (= `NUM_PARAMS`, 302) + the offset). In RAM:
  `activeParameters` grew by one record (the playing kit's), plus a prefetch buffer and a card buffer: +3.6 KB.
- **The store** `/UMAKITS.DAT` (card root, 64 KB): every kit's working record, at kit × 1,024. Outside `/tmp`, which a
  project save deletes.
- **Where records move:**

| Event | Stock | UMA record |
|---|---|---|
| Project load | each `NN-*.snd` → `mainParameters` | the file's appended record → the store (none: defaults; no file: defaults) |
| Boot, `/tmp` overlay | `/tmp` kit files → `mainParameters` | their records → the store (a missing file keeps the project's) |
| Kit load by hand / reload / project load | `mainParameters[n]` → edit buffer | store[n] → edit buffer, read at once (~1.5 ms, audio from the interrupt) |
| Pattern change (kit follows), MIDI bank change | the same, in the audio path | the prefetched record if it is kit n, else queued and read by the main loop a few ms later |
| Prefetch (main loop) | | the next pattern's kit (a queued pattern, or song mode's next entry) read ahead |
| Kit save | edit buffer → `mainParameters[n]` → `/tmp/NN-NAME.snd` | edit buffer → store[n] → appended to the file |
| Project save | every kit → `/projNN/NN-NAME.snd` | store[n] appended to each |
| Autosave `/tmp/tmp.snd` | the edit buffer | its record appended; read back at boot (older files: the kit's record) |

- **Test build:** `UMA_KIT_TEST` stamps a kit's record with its number + 1 when it is saved; the CPU screen shows the
  playing kit's stamp as `uk` (2nd sub-page: `ptm tin uk pk`).
- Image 671,328 bytes (80.4 KB spare); RAM 338.6 KB; ITCM +16 B.
- **Result (2026-11-12, user's LXR-02):** verified: stamps follow their kits through kit save/load, project save, power
  cycle, and pattern changes (prefetch), audio clean ("works great").

## 11. 0.1c and 0.1d: menus and FM (spec, for review)

### What carries over from the Eurorack

The desktop's voice sub-pages are in the Eurorack's order where FM uses them (Osc 1, Amp EG 2, Mod 3, FM 4,
Transient 5, Filter 6; LFO and Mix are swapped, which matters only in Phase 3), the display is the same 16 × 2,
the 8 select buttons pick the sub-page and pressing one again flips to its 2nd screen, as on the Eurorack. So the
**FM pages are the Eurorack's, unchanged**:

| Engine | Page | Screen 1 | Screen 2 | More (the button again) |
|---|---|---|---|---|
| `lxr` | Osc | `cor fin eng wav` | `pwm` | |
| `fmd` (FM Drum) | Osc | `cor fin eng alg` | `rtA fnA rtB fnB` | |
| | FM | `mdA atA dcA enA` | `mdB atB dcB enB` | `wvC wAB` |
| | Mod | `pd slp amt` | | |
| | Transient | `wav vol frq fdb` | | |
| `fmt` (FM Tone) | Osc | `cor fin eng alg` | `rtC rtA rtB rB2` | `sub sbs sbo sbf` |
| | FM | `mdA atA dcA enA` | `mdB atB dcB enB` | `mB2 aB2 dB2 eB2`, then `wvC wAB fnA fnB` |
| | Mod | `pd slp amt` | | |
| | Transient | `wav vol frq fdb` | | `dtn mix` |

The Eurorack's Mod page 2nd screen (randomise) and Transient 2nd screen (wavefolder), the Amp EG's hold and gate,
and the Osc page's pitch screen come with their features in Phase 2: **until then those places stay empty**, so
nothing on screen does nothing.

### Under the hood (0.1c)

- **Settings:** the Eurorack's `PAR_UMA_*` list, in its order, numbered from `PAR_UMA_BLOCK` + 2 (after the record's
  format and stamp bytes), so the copied engine and voice code keeps its offsets. Left out: the Eurorack-only
  settings (CV inputs, their calibration) and the globals (clock, MIDI CC channels: the global file, Phase 3).
  The record holds ~840 of its 1,022 bytes. Defaults: the Eurorack's (`lxr` on every drum).
- **Menus, ported from the Eurorack's `menu.c`:** 16-bit names (`Page.top*`, the name tables past 255); value types
  for UMA settings (engine, algorithm, ratio, fine, waveform sweep, sub shape and octave) in a table of their own for
  the numbers from `PAR_UMA_BLOCK` on; `menu_getPage` choosing a drum's pages by its `eng`; the extra screens.
- **Pots and encoder** edit UMA settings like stock ones (to check: the pot code takes 16-bit parameter numbers).
- 0.1c shows and saves the settings; **the sound follows in 0.1d** (0.0c's test hook stays until then).

### 0.1d

The Eurorack's voice code (`DrumVoice.c` FM parts, `uma_voice.c` / `uma_voicepar.c` for FM) reads the settings each
block; silent voices skip the engine; the test hook and `uk` go. Test: the same settings on both machines sound
the same; `pk` with a pattern.

### Decided (2026-11-12)

1. The Eurorack's page layouts, unchanged.
2. Places for Phase 2 features stay empty until they work.

### 0.1c as built

- `Core/Inc/uma_params.h`: the Eurorack's 847 UMA settings (generated from its `Parameters.h`), from
  `PAR_UMA_BLOCK` + 2 = 304; record format 2 (format-1 records from 0.1b get the defaults and keep their stamp).
  Defaults: the Eurorack's for FM, the body envelope, LPG and fold (`umaKit_makeDefaults`).
- Value types: `parameter_dtypes` grew to `PAR_UMA_END` with the Eurorack's UMA entries (mod-slot sources and targets
  plain numbers until Phase 3); checked in the ELF (Drum 1's `eng` at 304 = list, engine names).
- Names: 29 FM names and their short / category / long texts added to the desktop's tables; `Page.top*` 16-bit.
  Lists: engine (`lxr fmd fmt`), algorithm (FM Drum 7, FM Tone 8), ratio (51), sub shape, sub octave.
- `menu_getPage` (Drum 1-3 by `eng`) and the extra screens (the select button again), as on the Eurorack, also for the
  pots (`adcPots.c`). In FM modes the Mod page keeps the desktop's velocity screen (`vdt vam vvo`), where the
  Eurorack has randomise (Phase 2).
- UMA settings are never sent as MIDI CC (numbers above the sound parameters go to `menu_parseGlobalParam`, which
  ignores them): MIDI and automation for them are Phase 3.
- Image 676,816 bytes (75 KB spare); RAM unchanged; the sound is still 0.0c's test hook.
