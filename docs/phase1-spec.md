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
