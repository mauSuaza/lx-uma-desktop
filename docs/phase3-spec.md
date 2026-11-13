# Phase 3: modulation and the sequencer (spec, draft for review)

Status: **reviewed** (2026-11-13): questions answered (section 6). Phase 3 of [port-plan.md](port-plan.md), after release 0.2. Release **0.3**
at the end. The code is the Eurorack's (LX_UMA `a179390`) where it exists; the desktop's sequencer automation is new.

## 1. Decided with the user (2026-11-13)

- **Sources** without the Eurorack's jacks: no CV inputs (`cv1`–`cv5`) and no accent jacks (`ac1`–`ac6`). **Velocity**
  is a source (`VL1`–`VL6`, the voice's last hit: the step's velocity, or a MIDI note's). No extra accent source (the
  steps have velocity), no bar ramp.
- **No MIDI integration for now:** no MIDI CC sources (`MC1`–`MC8`), no UMA settings over MIDI CC / NRPN. The desktop
  is a whole instrument with its own UI and sequencer.
- **Pitch:** a modulation on `v/o` (a Turing Machine, an LFO) transposes the note that plays, the step's or a MIDI
  note's, then the quantizer snaps it (as on the Eurorack).
- **UI:** the LFO page (select 7) for the LFOs and the Turing Machines; **SHIFT + the LFO button** for the mod slots.

## 2. What comes

| Part | Eurorack | On the desktop |
|---|---|---|
| **Mod slots** | 8 per voice: `src amt dst pol` | The same, all 8 in the UMA record (on the Eurorack slots 1–3 are the stock CV matrix's settings, which the desktop doesn't have: 54 new bytes in the record) |
| **Sources** | cv, LFO A, accent, Turing A–D, LFO B, EN / EI, VL, MC | **LFO A, Turing A–D, LFO B, EN / EI, VL** per voice (6 × 10 = 60 sources) |
| **Targets** | each voice's own list (`umaModTargets.c`): stock settings, every engine's, randomise, slot amounts, `oct key scl` | the same lists, the stock part mapped to the desktop's parameters |
| **LFO B** | per voice: frequency, offset, shape, retrigger, sync | the same; sync to the sequencer's tempo |
| **Turing Machines** | 4 per voice: length, probability (lock … random), clock, divider; 64 steps | the same; `clk` steps on the sequencer's sixteenths (or the voice's own hits) |
| **Kit morph for UMA settings** | the morph reaches UMA's settings (lists step or switch, `lst`) | the same on the desktop's morph; the morph kit's UMA record joins the playing and the next one in RAM (+1 KB) |
| **Automation of UMA settings** | — (no sequencer) | **new, section 4** |

## 3. The UI

- **LFO page (select 7):** screen 1-2 the stock LFO (LFO A); pressing the button again steps through **LFO B**,
  **Turing A | B**, **Turing C | D**, then back. As the Eurorack's SHIFT + MOD views, on the desktop's own button.
- **SHIFT + select 7: the mod view.** In it the **8 select buttons pick slots 1–8** of the selected voice (one screen
  each, `src amt dst pol`); the voice buttons change the voice. Leaving it: SHIFT + select 7 again (in the view
  select 7 picks slot 7), or any page that isn't a voice page.
  To check in the code (0.3a): holding SHIFT with a select button starts sub-step automation recording in voice mode,
  so the mod view may need a short press only (released before the recording timer).
- **Sources and targets by name** on the slot's edit screen, as on the Eurorack ("LFO B Drum2", "TM C Cymbl";
  "Op A Ratio").

## 4. Automation of UMA settings (new)

**The problem.** A step locks up to 2 settings (`param1Nr` / `param2Nr` with a value). Each is an **8-bit parameter
number**, and the stock target list is already 241 entries long; UMA's settings are numbered from 304. Widening the
step would change every pattern file and the 228 KB of patterns in RAM.

**Proposal: UMA lanes.** Each voice gets **4 UMA lanes** in the kit (its UMA record): each lane points at one of the
voice's UMA settings (from the same list as the mod slots' targets). A step can lock a lane like any setting ("Lane 1
= Op A Ratio"), stored in the step as one of a few reserved target codes. So patterns keep their format, a lane's
setting comes with the kit, and the step's value moves the setting it points at.

- The automation target list on the desktop gains `UL1`–`UL4` per voice; the edit screen names the lane's setting.
- To check in the code (0.3d): which target codes are safe to reserve (unused by the stock firmware, ignored by it
  when a pattern is loaded there).
- Recording automation (turning a knob while recording) on a UMA setting assigned to a lane records the lane.

## 5. Builds

| Build | What | You test |
|---|---|---|
| **0.3a** | The modulation layer (`uma_mod.c`), the 8 slots, sources LFO A / EN / EI / VL; the mod view (SHIFT + select 7) | Slots move settings; the view's buttons |
| **0.3b** | LFO B; the LFO page's views | LFO B as on the Eurorack; synced to the tempo |
| **0.3c** | Turing Machines (4 per voice), their views and sources | Locked loops, `v/o` melodies through the quantizer |
| **0.3d** | Automation of UMA settings: the UMA lanes | A step moves an FM ratio, a Plaits model, `rto` |
| **0.3e** | Kit morph for UMA settings | Morphing two kits with FM / Plaits settings |
| **0.3** | Release: manual chapters, notes | |

## 6. Decided (2026-11-13)

1. **LFO A stays stock** (Erica's 8 shapes and its sync list: kits read the same on Erica's firmware). LFO B gets the
   Eurorack's 16-shape sweep and folder. For a kit converter: desktop → Eurorack is exact (the Eurorack converts the
   stock shapes, `lfoShapes_fromOldWave` / `fromOldSync`); Eurorack → desktop rounds LFO A to the nearest stock shape
   and division. The user converts mainly desktop → Eurorack.
2. **No `vel` setting** on the desktop: the steps have velocity; `VL1`–`VL6` is the velocity as a source.
3. **4 UMA lanes per voice.**
4. **Turing Machine clock:** `clk` chooses the sequencer's sixteenths (with the divider) or each hit of its voice.

## 7. 0.3a as built

- **Record format 4:** slots 1–3 of each voice at the record's end (`PAR_UMA_DSK_MS*`; the Eurorack's stock CV slots),
  slots 4–8 the Eurorack's; `v/o` per voice (`PAR_UMA_DSK_VOCT*`, 0–127 = 0–60 semitones up, before the quantizer;
  continuous with the quantizer off). New slots start on their voice's LFO A, amount 0.
- **Modulation:** the Eurorack's `uma_mod.c` (UMA settings PAR_UMA_ENG1 … PAR_UMA_END, the slot amounts among them;
  kit morph of UMA settings: 0.3e); `uma_modmatrix.c` each block before randomise: sources LFO A (the stock LFO's
  value, kept now in `Lfo.value`), EN / EI, VL; Turing and LFO B neutral until 0.3c / 0.3b; the Eurorack's source
  numbers (its cv, accent and MIDI CC places skipped). The modulation node sends UMA targets to `umaMod_apply` and
  never indexes the stock parameter table with them.
- **Targets:** the Eurorack's lists entry for entry (`umaModTargets.c`), the snare's, cymbal's and hi-hat's from the
  Eurorack's shared-list blocks (embedded); never active on the desktop: `vel`, `wvc` `wvm`, morph (0.3e), prk's.
- **The mod view:** SHIFT + select 7 (a short press) toggles it; select 1–8 = slot 1–8 of the voice (`src amt dst
  pol`, the slot number in the top right); the edit screen names the source ("LFO A Drum2") and the target in full
  ("fm only" for another engine's). Slot destinations are set on kit load, on edits, and when a drum's engine changes.
- Image 603,980 bytes, RAM 350 KB of 384 (~26 KB stack), ITCM 15,408 B.
