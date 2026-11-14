# Porting LX_UMA to the LXR desktop: plan, and Phase 0 (spec)

Status: **draft for review** (2026-11-12). The plan for bringing LX_UMA, custom firmware for the LXR Eurorack module
(release 0.34, `mauSuaza/LX_UMA`), to the LXR desktop drum machine, starting from Erica Synths' open-source desktop
firmware (`erica-synths/lxr-desktop-app`, v1.7). Phase 0 is specified in detail; the later phases are outlined, each
to get its own spec before code.

## 1. Decided so far

- **A separate repository**, this fork, so the desktop and the Eurorack firmware can each follow their own hardware.
  Public; commits use the GitHub noreply address.
- **No prk engine** on the desktop: the RAM it needs (about 85 KB) is what the desktop's sequencer uses.
  mdk and Plaits' drum models cover similar ground.
- **Everything else comes:** FM Drum and FM Tone (Drum 1–3), mdk and Plaits on every voice, the quantizer, envelopes,
  low-pass gate, wavefolder, randomise, the mod matrix, LFO B, Turing Machines, kit morph, and the 0.34 card files
  (DX7 banks, wavetables, terrains) with the Plaits sub.
- **Port in phases with a module test after each**, not engine by engine (the engines are finished and tested on
  the Mac) and not only at the end (memory, kits and the sequencer must work first).

## 2. What we start from (measured 2026-11-12)

Both firmwares run on the same chip (STM32F765, Cortex-M7 at 216 MHz) with the same memory map and the same
32 KB bootloader at 0x08000000. Of the desktop's 156 source files, 142 are in the Eurorack firmware too; the sound
engine files are almost identical (`DrumVoice.c` differs in 25 lines). What the desktop adds: the step sequencer
(`Sequencer/`), Euclidean patterns, USB MIDI, trigger outputs, a different front panel (buttons, LEDs). What only
the Eurorack has: the CV mod matrix (`ModMatrix_Processor.c`, 108 lines) and the trigger jacks.

| Build | Flash (code + data) | Main RAM (`.bss`) | DTCM | ITCM (16 KB) |
|---|---|---|---|---|
| Eurorack stock (v1.51) | 228 KB | 48 KB | 105 KB | 12.1 KB |
| **Desktop stock (v1.7)** | **274 KB** | **263 KB** (of 384) | 105 KB (of 128) | **14.6 KB** |
| LX_UMA 0.34 (Eurorack) | 718 KB | 347 KB | 110 KB | 15.7 KB |

The desktop's RAM goes on its patterns: `seq_patternSet` alone is 228 KB.

What LX_UMA adds on the Eurorack, by part:

| Part | Flash | RAM | On the desktop |
|---|---|---|---|
| Plaits (12 models) | 177 KB | 42 KB | yes |
| FM Drum + FM Tone | 165 KB (120 KB of it the operator waveform tables) | 4 KB | yes |
| mdk | 112 KB | 5 KB | yes |
| prk | 16 KB | ~85 KB | **no** |
| Card files (pool of 10 × 4 KB) | 2 KB | 44 KB | yes, maybe 6 slots (24 KB) |
| UMA core: mod, Turing Machines, LFO B, quantizer, envelopes | 23 KB | 32 KB | yes |
| Menus and pages | ~38 KB | (kit cache, below) | yes, redone |
| Kit cache: the Eurorack keeps all 64 kits in RAM (144 KB with UMA's settings) | — | 106 KB more than stock | **no**: the desktop doesn't keep all kits |

### The budget, estimated (Phase 0 measures it)

- **RAM:** desktop 263 KB + Plaits 42 + core 32 + card files 24 (6 slots) + FM, mdk 9 + bigger kits in the desktop's
  own kit buffers (UMA's settings: roughly 3× the desktop's 15 KB `mainParameters`, so +30 KB) ≈ **400 KB** of 384 KB
  main RAM, with about 20 KB of DTCM free. **Tight: within reach, not certain.** Room if needed: the card files in
  fewer slots or installed into flash, the Plaits work areas in DTCM, smaller Turing Machine and LFO state.
- **Flash:** desktop 274 KB + Plaits 177 + FM 165 + mdk 112 + core and menus ~60 ≈ **790 KB**. Against the
  **736 KB cap** the Eurorack's bootloader turned out to have (it doesn't erase sector 7; see LX_UMA's
  `plaits-engine.md`, 0.33l), that's ~55 KB over. **Shrinking the FM operator waveform tables** (fewer, shorter
  band-limited tables, ~30 KB instead of 120) brings it to about 700 KB. The desktop's update tool claims the full
  992 KB, as the Eurorack's does too, so Phase 0 tests it on the hardware.
- **ITCM:** 14.6 KB + UMA's 3.6 KB = 18.2 KB of 16. Some of the desktop's or UMA's ITCM functions move to flash
  (small CPU cost, measured).
- **CPU:** the desktop also runs the sequencer, and its FX bus as the Eurorack does. To measure with Plaits and FM
  voices playing a pattern.

## 3. What carries over, what's redone

| | How |
|---|---|
| **Sound engines** (FmDrum, FmWaves, MdDrum + mnmd/, PlaitsDrum + mi/, uma_sdfiles) | Copied unchanged; the Mac tests (`tools/fmtest`) come too and must give the same results |
| **UMA core** (uma_mod, uma_turing, uma_lfo, uma_quant, uma_env, uma_vel, uma_random, uma_voice*) | Copied; adapted where they touch the Eurorack's jacks or mod matrix |
| **Settings, kits** (`Parameters.h`, `presetManager.c`) | Redone on the desktop's files: its kit and pattern files must keep loading, and stock desktop kits must still load |
| **Menus** (`menu.c`, `menuPages.h`, `MenuText.h`) | Redone on the desktop's menu code (it differs from the Eurorack's by ~950 lines in `menu.c`) |
| **Eurorack-only** | CV inputs and their calibration, trigger-jack gates (the 6-op's held key, `fmt` legato), accent jacks, MIDI and clock on the OP HH jack: dropped or replaced by the desktop's own (MIDI notes held = the gate; DIN and USB MIDI) |
| **Desktop-only, new work** | The sequencer's parameter automation and MIDI CC/NRPN reaching UMA's settings (numbered above 255, past the stock limit); patterns staying compatible |

## 4. Phases

| Phase | What | Module test |
|---|---|---|
| **0. Will it fit?** | Below | Boots; the flash cap; CPU and memory with the engines in |
| **1. Foundation + FM** | Settings and kit format; the menu framework (extra screens, value types); the modulation layer; FM Drum and FM Tone on Drum 1–3, with the smaller waveform tables | Stock kits and patterns unchanged; the FM engines sound as on the Eurorack; kits save and load |
| **2. All engines, all voices** | mdk and Plaits on every voice (engine choice on the snare, cymbal, hi-hat); quantizer; envelopes, LPG, folder; randomise | Each engine against the Eurorack with the same settings; CPU with a pattern running |
| **3. Modulation + sequencer** (0.3a–0.3e2 built and tested 2026-11-14; not released: it waits for Phase 4; UMA lanes moved later) | Mod slots, LFO B, Turing Machines, velocity, MIDI CC sources, kit morph; the sequencer's automation and MIDI reaching UMA's settings | Automation and modulation move UMA settings in time |
| **4. Card files + release** | DX7, WAVES, TERRAIN and the Plaits sub; the desktop manual; release | As LX_UMA 0.34's checks |

## 5. Phase 0, in detail

Goal: before porting any UI, know whether everything fits and what has to give.

| Build | What | Mac | Module |
|---|---|---|---|
| **0.0a** | Stock desktop firmware, renamed (update file `LXR_V2_UMA_V0.0a.img`: the updater looks for `LXR*.img`; the start-up screen says it's LX_UMA), plus LX_UMA's **CPU readout** (peak and average load on a global page, as in Eurorack 0.2) | builds | Installs and boots; **CPU of the stock desktop** with a busy pattern and the FX on: the headroom we have |
| **0.0b** | **The flash cap test:** the same build padded so the image reaches into sector 7 (0x080C0000+), with a known pattern at its end that the firmware checks at start-up and reports on the screen ("S7 ok" / "S7 bad") | builds; the image's end address checked | Installs or fails ("flash write error" as on the Eurorack): **is the cap 736 KB or 992 KB?** |
| **0.0c** | **The engines in, without menus:** FmDrum + FmWaves, MdDrum, PlaitsDrum and uma_sdfiles linked in, behind a compile-time switch three voices play them with fixed settings (Drum 1 FM Tone, Drum 2 Plaits wavetable, Drum 3 mdk kick), the rest stock | the linker's memory report: flash, RAM, DTCM, ITCM | Boots; the three voices sound (from the sequencer); **CPU** with a pattern; RAM left for the stack |

### 0.0c as built

- Engines copied from LX_UMA `a179390` (`docs/SOURCES.md`): FmDrum + FmWaves, MdDrum + mnmd/, PlaitsDrum + mi/,
  uma_sdfiles; C++ added to the build as on the Eurorack. Test hook `Core/Src/uma_enginetest.c` (flash, noinline;
  `UMA_ENGINE_TEST` in uma.h): Drum 1 FM Tone (sub on), Drum 2 Plaits wavetable, Drum 3 the first mdk kick, in place
  of the voice's oscillators; the transient, filter, amp EG (skipped for the mdk kick, which has its own) and drive
  stay. The engines run every block, silent or not: the worst case.
- **Memory (linker):** image 695,312 bytes, ends at 0x080B1C10: **57 KB below the sector-7 cap**, with the full
  120 KB of FM waveform tables. RAM 351 KB of 384 (main stack at its top: ~24 KB left; stock had 113 KB free):
  `seq_patternSet` 228 KB, Plaits voices 39 KB, card pool 40 KB (10 slots), mdk voices 4 KB. DTCM 109 KB of 128.
  ITCM 15,080 B of 16 KB (+80 B for the hook's calls).
- Still to add on top (estimates from section 2): UMA core ~23 KB flash / 32 KB RAM, menus ~38 KB flash, bigger kits
  ~30 KB RAM. Flash: ~61 KB against 57 spare, so the FM tables shrink after all (or the trims). RAM: ~62 KB against
  24, so the card pool goes to 6 slots or fewer and the Plaits work areas are looked at (section 2's list).
- **Result (2026-11-12, user's LXR-02):** boots; `pk` **83 %** with a pattern playing and the three engines running
  every block (stock 67 %): +16 points for FM Tone + Plaits wavetable + mdk kick, close to the Eurorack (+18 for three
  Plaits voices). About 17 % left.

**Phase 0 decisions (2026-11-12):** size-compile the non-audio code first (FM tables shrink only if needed);
card pool 6 slots; engines on every voice, documented, no hard limit. Found: the kits' UMA settings need ~51 KB if
all 64 kits stay in RAM (section 2's +30 KB was too low): see [phase1-spec.md](phase1-spec.md), section 3.

**Out of Phase 0, decided with the user:**
- the flash budget: with the waveform tables shrunk or not, and anything else to trim;
- the RAM budget: the card files' slots (6, or installed into flash), where the Plaits work areas live;
- which ITCM functions move;
- whether the CPU allows Plaits and FM on every voice with the sequencer running, or a limit (as on the Eurorack's
  CPU guidance).

**Answered (2026-11-12):**
1. **Test hardware:** the user's LXR-02. Its card mounts as "NO NAME", the same name as the Eurorack module's card, so
   check the card's contents (`PROJ00`... on the desktop, `DX7/` and `LXRE_*.img` on the Eurorack) before copying.
   The card's projects were backed up to `~/src/eurorack/backups/desktop-card-2026-11-12/` before the first copy.
2. **Names:** `LXR_V2_UMA_V<version>.img` (version from `UMA_VERSION` in `Core/Inc/uma.h`); Phase 0 test builds
   0.0a, 0.0b, 0.0c; the first release with FM is 0.1. Start-up screen: "LX_UMA v<version>" / "base fw V 2.00".
3. **Engines in step:** copy the engine files at each port and record the LX_UMA commit they came from in
   `docs/SOURCES.md` (from 0.0c on). A shared repository later if both firmwares keep changing together.

### 0.0a as built

- The readout times the **sequencer + sound render** (`seq_tick` + the block), so it shows the real headroom with
  the sequencer. On the desktop the main loop renders (`soundengine_process`); the TIM2 interrupt does it only
  during SD saves (`USE_INTERRUPT_ONLY_DURING_SAVE_PROJ`), so both places are timed. 0.0a timed only the interrupt
  and showed 0; fixed in 0.0a2. The global page (SHIFT + step 12; pressed again it steps through the screens) has
  a full 1st screen, so the readout is the 4th slot of the **2nd sub-page** (`ptm tin tou pk`): `pk` = the longest
  render in the last 2 s in % of a 64-sample block (over 100 = audio drop-outs). The average is computed but not
  shown (one free slot).
- **Result (2026-11-12, user's LXR-02):** 0.0a2 installs and boots; `pk` **67 %** with a pattern playing: the
  stock desktop's headroom is about a third of the CPU.

### 0.0b as built

- A 4 KB pattern at 0x080C0000 (section `.uma_s7test`, `Core/Src/uma_s7test.c`), the image padded with zeros up to
  it (757,760 bytes, ends at 0x080C1000). The start-up screen's 2nd line shows "S7 ok" or "S7 BAD n" (bytes that
  differ) for 3 s. **0.0b2** writes the inverse pattern (`UMA_S7TEST 2`): over 0.0b's pattern that needs 0 bits
  turned into 1, which only an erase allows. Zeros alone can't show it (Eurorack 0.33l lesson).
- Reading: 0.0b and 0.0b2 both install and say "S7 ok" → the bootloader erases sector 7, the cap is 992 KB.
  Either fails ("flash write error") or says "S7 BAD" → the cap is 736 KB. After a failed install, reinstall 0.0a2.
- **Result (2026-11-12): the cap is 736 KB.** 0.0b installed and said "S7 ok" (the sector was blank there); 0.0b2
  failed with "flash write error": the desktop's bootloader doesn't erase sector 7 either. The linker script now
  fails any build whose image (with the ITCM/DTCM contents) reaches 0x080C0000, as on the Eurorack. So the flash
  budget of section 2 holds: ~790 KB estimated → the FM waveform tables must shrink, or more trimming (Eurorack 0.33l
  list: -Os on UI/FatFs/HAL, mdk -O2, cold Plaits code -Os, speech already dropped in 0.34).

- Memory after 0.0a: flash 274.5 KB (image 275,400 bytes), RAM 271 KB (.bss), DTCM 107 KB, ITCM 15.0 KB of 16.
