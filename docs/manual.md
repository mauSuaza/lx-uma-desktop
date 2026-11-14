# LX_UMA for the LXR-02 desktop: user manual

Custom firmware for the Erica Synths LXR-02 drum machine, by the makers of LX_UMA for the LXR Eurorack module
([github.com/mauSuaza/LX_UMA](https://github.com/mauSuaza/LX_UMA)). It is not official Erica Synths firmware and
Erica Synths provides no support for it (see the README).

This manual covers release **0.2** (the FM, drum-machine and Plaits engines, envelopes, the low-pass gate, the
wavefolder, randomise and the quantizer) and the **0.3 test builds** (modulation: mod slots, LFO B, Turing
Machines, kit morph; not released yet, see [What 0.3 adds](#what-03-adds)). Everything else works as on Erica's
firmware v1.7; see the LXR-02 manual for it.

## Start here

### Installing

1. Back up your SD card (copy everything on it to your computer). LX_UMA keeps your projects, but adds to their kit
   files (below).
2. Copy `LXR_V2_UMA_V0.2.img` to the top folder of the card and delete any other `LXR*.img` there.
3. Put the card in the LXR, hold down the encoder button and switch the LXR on. Keep holding until "LXR Updater"
   appears. Wait for "Update OK!".
4. The start-up screen says **"LX_UMA v0.2"** and "base fw V 2.00".

To go back to Erica's firmware, install its update file the same way. Your projects still load there (the kits
play as stock sounds).

### What 0.2 adds

| | |
|---|---|
| **Engines** (`eng`, Osc page) | Drum 1–3: `lxr`, `fmd` FM Drum, `fmt` FM Tone, `mdk` drum machines, `pla` Plaits. Snare, cymbal, hi-hat: `lxr`, `mdk`, `pla` |
| **Envelopes** (Drum 1–3) | Hold, gate mode with MIDI notes; the filter envelope (also on the snare, cymbal, hi-hat) |
| **Low-pass gate** (Drum 1–3) | Filter type `lpg`, after Plaits |
| **Wavefolder** | On FM, mdk and Plaits voices |
| **Randomise** | FM (tone, envelopes, sweep, fine) and mdk (machine, knobs) |
| **Quantizer** | Octave, key and scale on every voice |
| **Kits** | Every kit keeps its UMA settings (below) |
| **CPU readout** | `pk` on the global page (below) |

### What 0.3 adds

*Test builds (`LXR_V2_UMA_V0.3e2.img`), not released yet: the release waits for Phase 4 (DX7 banks, wavetables and
terrains from the card).*

| | |
|---|---|
| **Mod slots** | 8 per voice, in the mod view: **SHIFT + LFO** ([Modulation](#modulation)) |
| **Sources** | LFO A and B, four Turing Machines, the envelope (and inverted) and velocity of every voice |
| **Pitch** | `v/o`: a modulation transposes the note, then the quantizer snaps it |
| **LFO B** | Every voice: 16 shapes to sweep through, a folder, sync to the tempo, restarts on hits or bars ([LFOs](#lfos)) |
| **Turing Machines** | Four per voice, on hits or the sequencer's sixteenths; patterns saved with the kit ([Turing Machines](#turing-machines)) |
| **Kit morph** | The X-fade morphs UMA's settings too; `lst` for list settings ([Kit morph](#kit-morph)) |

### Getting around

As on the stock firmware: a voice's select buttons choose its pages (Osc 1, Amp EG 2, Mod 3, FM 4, Transient 5,
Filter 6, LFO 7, Mix 8). Pressing the same select button again shows the page's 2nd screen. Some pages have **more
screens**: press the button again after the 2nd. The Osc page's last screen is always the pitch screen
`oct key scl`. The pots edit the four settings on screen.

`eng` sits on the Osc page after `fin` on every voice, so you can keep turning it while the page changes around it.
The Amp EG, Filter and Mix pages are the voice's own on every engine. On the drums `eng` lists `lxr fmd fmt --- mdk
pla`, on the snare, cymbal and hi-hat `lxr --- mdk pla`: the `---` place (the Eurorack's prk) is skipped.

### Amp EG: hold and gate mode

Drum 1–3, every engine:

| Short | Edit name | What it does | Values |
|---|---|---|---|
| `hld` | Veloc EG Hold | Full level after the attack, then the decay: flat, punchy bodies | 0 = off … 2 s (about 50 ms at 20, 0.5 s at 64) |
| `sus` | Veloc EG Sustain | Gate mode: the level it holds while the gate is high | 0–127 |
| `rel` | Veloc EG Release | Gate mode: the fade after the gate ends | 0–127, like `dec` |
| `gat` | Veloc EG Trg/Gate | `trg`: attack → hold → decay. `gat`: attack → hold → decay → sustain while the gate is held → release | `trg` / `gat` |

The gate is a **MIDI note held** (DIN or USB, on the voice's MIDI channel). The sequencer's hits are triggers: a
hit with no gate behind it (a step, a pad, a released note) plays as `trg`, so it never hangs. The Amp EG also drives the
filter envelope (`env`) and the wavefolder's depth. With `typ` = `lpg` the LPG sets the volume. On `mdk` the machine sets its own volume: the Amp EG
shapes the filter envelope.

### The filter envelope and the low-pass gate

**Filter envelope** (`env`, Filter screen 2, every voice and engine): the Amp EG moves the cutoff, positive opens
with the hit, negative closes. −63…+64, ±63 = ±6 octaves, 0 = off.

**The low-pass gate** (`typ` = `lpg`, Drum 1–3, every engine).

A low-pass gate ported from Mutable Instruments Plaits (Emilie Gillet, MIT license). A hit **pings a vactrol**
that sets **both the volume and the cutoff**; the brightness falls faster than the volume: a bright hit with a
dark, woody tail. Screen 2 shows the LPG's settings instead of `env`:

| Short | Edit name | What it does | Values |
|---|---|---|---|
| `frq` | Filter Frequncy | How far the gate opens (its brightness ceiling) | as on the LXR |
| `res` | Filter Resnance | Resonance (0 = Plaits' sound) | as on the LXR |
| `drv` | Filter Overdriv | Drive into the gate | as on the LXR |
| `dcy` | LPG Decay | How long it rings | ~35 ms (0) … 9 s (127); 50 ≈ 0.3 s |
| `col` | LPG Colour | 0 = full low-pass gate (dark, woody tail); up = closer to a VCA (stays bright, lets go a bit sooner) | 0–127 |
| `exc` | LPG Excite | How hard each hit strikes it (after the Steady State Gate's EXCITE): lower is quieter **and darker**. Taken at the hit | 0–127, default 127 |

In `lpg` mode the Amp EG doesn't set the volume. The pitch sweep and operator envelopes keep working.

### Pitch: octave, key and scale

The **last screen of every voice's Osc page**, on every engine: press the Osc page's button until `oct key scl`
appears (the screen after the engine's own; another press goes back to screen 1).

| Short | Edit name | What it does | Values |
|---|---|---|---|
| `oct` | Oscilltr Octave | Moves the voice by whole octaves | −4 … +4, 0 = none |
| `key` | Oscilltr Key | The root of the scale or chord | C … B |
| `scl` | Oscilltr Scale/chord | **The quantizer**: the voice only plays this scale's or chord's notes; the edit screen names it in full | `off` (default), then the list below |

**What gets snapped, in this order:** the step's or MIDI note → coarse tune and its modulation, and `oct` → **the quantizer**, to the nearest note of the
scale in any octave (exactly between two: the lower) → then fine tune, its modulation (vibrato stays smooth), the
pitch sweep and the snap. So a kick's sweep stays smooth and lands in key.

- `key` doesn't transpose: it says which notes the scale has. With `scl` `off` it does nothing. To transpose, use
  coarse tune, `oct` or the MIDI note; they're snapped too.
- A quantizer works on note names: a 9th is the 2nd in every octave. `M9` on C allows C D E G B everywhere.
- A held note keeps its note until the input is clearly past the midpoint to the next one (0.2 semitone).
- Kits from earlier versions load with `oct` 0 and `scl` `off`: their pitch exactly as before.

**The list** (`scl`): notes in semitones from `key`.

| Short | Name | Notes |
|---|---|---|
| `off` | Off (chromatic) | every note: nothing is snapped |
| `maj` | Major | 0 2 4 5 7 9 11 |
| `min` | Minor | 0 2 3 5 7 8 10 |
| `hmi` | Harmonic minor | 0 2 3 5 7 8 11 |
| `mmi` | Melodic minor | 0 2 3 5 7 9 11 |
| `dor` | Dorian | 0 2 3 5 7 9 10 |
| `phr` | Phrygian | 0 1 3 5 7 8 10 |
| `lyd` | Lydian | 0 2 4 6 7 9 11 |
| `mix` | Mixolydian | 0 2 4 5 7 9 10 |
| `pnM` | Pentatonic maj | 0 2 4 7 9 |
| `pnm` | Pentatonic min | 0 3 5 7 10 |
| `M` | Major | 0 4 7 |
| `M6` | Major 6 | 0 4 7 9 |
| `M7` | Major 7 | 0 4 7 11 |
| `7` | Dominant 7 | 0 4 7 10 |
| `ad9` | Add 9 | 0 2 4 7 |
| `M9` | Major 9 | 0 2 4 7 11 |
| `9` | Dominant 9 | 0 2 4 7 10 |
| `M11` | Major 11 | 0 2 4 5 7 11 |
| `11` | Dominant 11 | 0 2 4 5 7 10 |
| `#11` | Major 7 #11 | 0 4 6 7 11 |
| `M13` | Major 13 | 0 2 4 7 9 11 |
| `13` | Dominant 13 | 0 2 4 7 9 10 |
| `m` | Minor | 0 3 7 |
| `m6` | Minor 6 | 0 3 7 9 |
| `m7` | Minor 7 | 0 3 7 10 |
| `mM7` | Minor major 7 | 0 3 7 11 |
| `ma9` | Minor add 9 | 0 2 3 7 |
| `m9` | Minor 9 | 0 2 3 7 10 |
| `m11` | Minor 11 | 0 2 3 5 7 10 |
| `m13` | Minor 13 | 0 2 3 7 9 10 |
| `mb6` | Minor b6 | 0 3 7 8 |
| `m7-` | Half-dim m7b5 | 0 3 6 10 |
| `dim` | Diminished | 0 3 6 |
| `dm7` | Diminished 7 | 0 3 6 9 |
| `su2` | Sus 2 | 0 2 7 |
| `su4` | Sus 4 | 0 5 7 |
| `7s4` | 7 sus 4 | 0 5 7 10 |
| `aug` | Augmented | 0 4 8 |
| `7b9` | 7 b9 | 0 1 4 7 10 |
| `7#9` | 7 #9 | 0 3 4 7 10 |
| `7#5` | 7 #5 | 0 4 8 10 |
| `5` | Fifth | 0 7 |
| `oct` | Octaves | 0 |
| `h1` | Harm min i mM7 | 0 3 7 11 (in A: Am(maj7)) |
| `h2` | Harm min ii m7b5 | 2 5 8 0 (Bm7b5) |
| `h3` | Harm min III+M7 | 3 7 11 2 (Cmaj7#5) |
| `h4` | Harm min iv m7 | 5 8 0 3 (Dm7) |
| `h5` | Harm min V7b9 | 7 11 2 5 8 (E7b9) |
| `h6` | Harm min VI M7 | 8 0 3 7 (Fmaj7) |
| `h7` | Harm min vii dim | 11 2 5 8 (G#dim7) |

`h1`–`h7` are the chords on each step of the harmonic minor scale on `key`: stepping through them plays a progression in one key.


---

---

## lxr — the LXR's own engine

`eng` = `lxr` is the LXR's own sound, as on the stock firmware: see Erica Synths' LXR-02 manual. LX_UMA adds `eng` on
the Osc page (Drum 1–3: `coa fin eng wav` \| `pwm`), hold and gate mode on the Amp EG, the filter envelope and the
low-pass gate on the Filter page, and the pitch screen.

---

## fmd — FM Drum

`eng` = `fmd`, Drum 1–3. Three operators, after the Digitone II's FM DRUM.

### How it works

Three operators, after the Digitone II's FM DRUM: **C** is the carrier you hear; **A** and **B** modulate it
(in algorithms 5–7, B is also heard on its own). C plays at the drum's pitch (coarse, fine, the step's note, MIDI note); A and B run at C's pitch times their **ratio**, so the tone stays the same at every pitch. The
pitch sweep (Mod page) moves all three together. A and B each have an envelope that shapes how much they
modulate over the hit.

The FM sound then goes through the **wavefolder** and **body level**, is mixed with B's direct output (5–7) and
the transient, and runs through the **filter or the low-pass gate**, the **amp envelope**, drive and the mixer.

### The algorithms

| Algo | Routing | Feedback on | Character |
|---|---|---|---|
| 1 | A → C, B → C | B | Two modulators side by side |
| 2 | A → C, B → C | A and B | As 1, harsher with feedback |
| 3 | B → A, B → C, A → C | B | B colours both: more complex |
| 4 | B → A → C | B | A stack: the most complex; B only matters when A's `mod` is up |
| 5 | A → C; B heard | B | Two layers: the FM body plus B as its own sound |
| 6 | A → C; B heard | A | As 5, feedback roughens the body instead of B |
| 7 | A → C; B heard | A and B | As 5, both rough |

In 5–7, B's `mod` and envelope set its **volume**. The algorithms differ in where B and the feedback go: with
B's `mod` and the feedback at 0, all seven sound the same.

### The ratios

`rta`, `rtb`: A's or B's frequency as a multiple of C's, 51 steps, one per encoder click.

| Kind | Steps | Sound |
|---|---|---|
| Below C | `1/4 1/3 1/2 2/3 3/4` | sub tones: thick, growling (1/3, 1/4), the octave below (1/2), the fifth and fourth below (2/3, 3/4) |
| Whole numbers | `1 2 3 4 5 6 7 8 9 10 11 12 13 14 16` | the harmonic series: 1 saw-like, 2 square-like and hollow, higher = brighter, clicky with a short envelope |
| Intervals | `6/5` minor third · `5/4` major third · `4/3` fourth · `3/2` fifth · `8/5` minor sixth · `5/3` major sixth · `7/4` harmonic seventh · `9/5` minor seventh · `1.9` major seventh (15/8) | chordal: stabs, chords from one voice, vocal formants |
| Compound intervals | `9/4` ninth · `5/2` · `8/3` eleventh · `3.3` thirteenth (10/3) · `7/2` · `9/2` | as the intervals, an octave up: brighter, wider chords. `7/2` is the electric piano tine |
| Inharmonic | `.71` (1/√2) · `1.4` (√2) · `1.7` (√3) · `2.4` (1+√2) · `2.8` (2√2) · `3.1` (π) · `3.4` (2√3) · `4.2` (3√2) · `4.7` (3π/2) · `5.2` (3√3) · `5.7` (4√2) · `6.3` (2π) · `6.9` (4√3) · `7.9` (5π/2) · `8.5` (6√2) · `9.4` (3π) | metallic: bells, plates, cymbals, hats (the π family: noisy, scraped). With deep `mod`, and on both A and B, they get dense and noisy |

`fna` / `fnb` (fine ratio) move a ratio up to halfway to its neighbours either way: detuned, in-between,
metallic.

### Pages

| Page | Screen | Short | Edit name | What it does |
|---|---|---|---|---|
| Osc (select 1) | 1 | `coa` `fin` | Oscilltr Coarse / Fine | Pitch |
| | 1 | `eng` | Voice Engine | The engine |
| | 1 | `alg` | FM Algorthm | How the operators connect, 1–7 |
| | 2 | `rta` `rtb` | Op A / Op B Ratio | A's / B's frequency as a multiple of C's ([ratios](#the-ratios)) |
| | 2 | `fna` `fnb` | Op A / Op B Fine rt | Fine ratio: 0 = exact, ±63 = halfway to the ratio below / above (screen order `rta fna rtb fnb`) |
| Amp EG (select 2) | 1 | `atk` `dec` `slp` | Veloc EG Attack / Decay / Slope | The amp envelope, as on the LXR |
| | 1 | `hld` | Veloc EG Hold | Full level after the attack, then the decay: flat, punchy bodies. 0 = off … 2 s (about 50 ms at 20, 0.5 s at 64) |
| | 2 | `sus` `rel` `gat` | Veloc EG Sustain / Release / Trg/Gate | Gate mode ([Amp EG: hold and gate mode](#amp-eg-hold-and-gate-mode)) |
| Mod (select 3) | 1 | `dec` `slp` `mod` | Mod EG Decay / Slope / PitchMod Amount | The pitch sweep (moves all three operators) |
| | 1 | `rpl` | Random Palette | Which ratios `rto` draws: `all` `bas` `hrm` `met` ([Randomise](#randomise)) |
| | 2 | `rto` `rev` `rsw` `rsd` | Random Tone / Env / Sweep / Seed | Randomise; 0 restores ([Randomise](#randomise)) |
| | 3 | `rfn` | Random Fine | Re-rolls `fna` `fnb`; 0 = your fine |
| | 3 | `vdt` `vam` `vvo` | Velocity Dest / Amount / Vol | As on the LXR: velocity modulation of one setting, and of the volume |
| FM (select 4) | 1 | `mod` | Op A Mod | How much A modulates at its envelope's peak |
| | 1 | `atk` `dec` `end` | Op A Attack / Decay / End lvl | A's envelope: rise (0 instant … 2 s), fall (5 ms–5 s), the level it falls to and holds (share of `mod`) |
| | 2 | `mod` `atk` `dec` `end` | Op B … | B's, the same (in algorithms 5–7, B's volume) |
| Transient (select 5) | 1 | `wav` `vol` `frq` | Transnt Wave / Volume / Freq | The LXR's transient, layered on the hit |
| | 1 | `fdb` | FM Feedback | Self-modulation of the algorithm's feedback operator(s): brighter, gritty, then noise |
| | 2 | `fld` | FM Fold | Wavefolder. Its depth **follows the Amp EG** (or the LPG): each hit folds hard, the tail cleans up. 0 = off |
| | 2 | `fty` | FM FoldType | `sin`: smooth, round folds; `tri`: sharper, edgier |
| | 2 | `sym` | FM Symmetry | Off centre adds even harmonics: richer, buzzier. −63…+64 |
| | 2 | `lev` | FM Body lvl | The level after the folder, against the transient. 100 = unity |
| Filter (select 6) | 1 | `frq` `res` `typ` `drv` | Filter … | As on the LXR; `typ` adds **`lpg`**, the low-pass gate, after `off` |
| | 2 | `env` | Filter Env | The Amp EG moves the cutoff: −63…+64, ±63 = ±6 octaves, 0 = off ([filter envelope](#the-filter-envelope-and-the-low-pass-gate)) |
| | 2 | `dcy` `col` `exc` | LPG Decay / Colour / Excite | With `typ` `lpg`, in place of `env` ([low-pass gate](#the-filter-envelope-and-the-low-pass-gate)) |
| Mix (select 8) | 1 | `vol` `pan` `srt` `drv` | Voice … | Level, pan, sample-rate reduction, drive, as on the LXR |


---

---

## fmt — FM Tone

`eng` = `fmt`. A melodic FM engine for **bass** (and drums too: the transient and the pitch sweep stay), after
the Digitone's FM TONE machine, built on the FM Drum engine: four operators, eight algorithms, and a **sub
oscillator**. Everything around it works as for `fmd`: Amp EG with hold and gate mode, filter, LPG, the wavefolder. The FM Drum settings (ratios A and B, mods, envelopes, feedback,
fold) are shared: switching `fmd` ↔ `fmt` keeps them.

### How it works

Four operators: **C** (the carrier), **A**, **B1** and **B2**. Each runs at its own **ratio** of the played
pitch: `rtc` for C (×1 plays the note; ×0.5 drops the whole tone an octave), `rta`, `rtb` (B1) and `rb2`. A, B1
and B2 each have their own depth and operator envelope (`mod atk dec end` on the FM page: A, B1, B2), so one
modulator can snap while another lingers. Each algorithm has two outputs, **X** and **Y**,
and `mix` crossfades them. An operator that is heard (on X or Y) plays at the level its depth setting gives it.

**Pitch and legato:** the pitch is set every audio block from tune and the step's or MIDI note. With the Amp EG in
gate mode (`gat`) and a MIDI note held from one note to the next, a new note just moves the sound and nothing
restarts.

### The algorithms

| Algo | Modulation | Feedback | X | Y |
|---|---|---|---|---|
| 1 | A → C; B2 → B1 → C | A | C | B1 |
| 2 | A → C; B2 → B1 | B2 | C | B1 |
| 3 | A → C, B1 and B2 | A | C + B2 | B1 |
| 4 | B2 → B1 → A → C | B2 | C | B1 |
| 5 | B1 → B2; B1 and B2 → A → C | B1 | C | A |
| 6 | A and B2 → C and B1 | A | C | B1 |
| 7 | A → C; B2 → B1 | A | C + A | B1 + B2 |
| 8 | A → C; B1, B2 alone | B1 | C + B2 | B1 |

With `mix` at 0 you hear X: C with whatever modulates it (plus B2 or A in 3, 7, 8). Raise `mix` for Y.

### Detune

`dtn` pushes A and B2 sharp against C and B1: subtle below the middle (about 6 cents at 64: slow beating,
movement), wide at the top (50 cents: chorus, warble).

### The sub oscillator

`sub` (level), `sbs` (shape: `sin` `tri` `saw` `sqr`, band-limited), `sbo` (1 to 4 octaves below the **played**
pitch, not C's ratio) and `sbf` (a folder on the sub alone: 1–24 a light saturation that fades in, the "boom";
up to a hard fold at 127; smooth when modulated). The sub restarts with each hit, joins **after** the FM
wavefolder (clean low end) and goes through the filter or LPG and the Amp EG like the rest. With loud FM, turn
the FM part down with `lev` (body level) to bring the sub forward; `lev` doesn't change the sub. `sbo` `-3` and
`-4` are for a high, bright sound with its bass far below.

### Pages

| Page | Screen | Short | Edit name | What it does |
|---|---|---|---|---|
| Osc (select 1) | 1 | `coa` `fin` | Oscilltr Coarse / Fine | Pitch |
| | 1 | `eng` | Voice Engine | The engine |
| | 1 | `alg` | FM Algorthm | How the operators connect, 1–8 |
| | 2 | `rtc` | Op C Ratio | C's frequency as a multiple of the played pitch (×1 default) |
| | 2 | `rta` `rtb` `rb2` | Op A / Op B / Op B2 Ratio | A's, B1's, B2's frequency, multiples of the played pitch |
| | 3 | `sub` | Sub Level | The sub oscillator's level; 0 = off |
| | 3 | `sbs` | Sub Shape | `sin` `tri` `saw` `sqr` |
| | 3 | `sbo` | Sub Octave | `-1` … `-4` octaves below the played pitch |
| | 3 | `sbf` | Sub Fold | A folder on the sub alone: a little saturation at first, folds above; 0 = off |
| Amp EG (select 2) | 1 | `atk` `dec` `slp` | Veloc EG Attack / Decay / Slope | The amp envelope, as on the LXR |
| | 1 | `hld` | Veloc EG Hold | Full level after the attack, then the decay: flat, punchy bodies. 0 = off … 2 s (about 50 ms at 20, 0.5 s at 64) |
| | 2 | `sus` `rel` `gat` | Veloc EG Sustain / Release / Trg/Gate | Gate mode ([Amp EG: hold and gate mode](#amp-eg-hold-and-gate-mode)) |
| Mod (select 3) | 1 | `dec` `slp` `mod` | Mod EG Decay / Slope / PitchMod Amount | The pitch sweep |
| | 1 | `rpl` | Random Palette | `all` `bas` `hrm` `met` (below) |
| | 2 | `rto` `rev` `rsw` `rsd` | Random Tone / Env / Sweep / Seed | Randomise; 0 restores |
| | 3 | `rfn` | Random Fine | Re-rolls `fna` `fnb` |
| | 3 | `vdt` `vam` `vvo` | Velocity Dest / Amount / Vol | As on the LXR: velocity modulation of one setting, and of the volume |
| FM (select 4) | 1 | `mod` `atk` `dec` `end` | Op A … | A's depth and envelope |
| | 2 | `mod` `atk` `dec` `end` | Op B … | B1's |
| | 3 | `mod` `atk` `dec` `end` | Op B2 … | B2's: depth (where it modulates) or level (where it's heard), its own envelope |
| | 4 | `fna` `fnb` | Op A / Op B Fine rt | Fine ratios |
| Transient (select 5) | 1 | `wav` `vol` `frq` | Transnt Wave / Volume / Freq | The LXR's transient |
| | 1 | `fdb` | FM Feedback | The algorithm's feedback |
| | 2 | `fld` | FM Fold | Wavefolder. Its depth **follows the Amp EG** (or the LPG): each hit folds hard, the tail cleans up. 0 = off |
| | 2 | `fty` | FM FoldType | `sin`: smooth, round folds; `tri`: sharper, edgier |
| | 2 | `sym` | FM Symmetry | Off centre adds even harmonics: richer, buzzier. −63…+64 |
| | 2 | `lev` | FM Body lvl | The level after the folder, against the transient. 100 = unity |
| | 3 | `dtn` | FM Detune | A and B2 sharp against C and B1: 6 cents at 64, 50 at 127 |
| | 3 | `mix` | FM Mix X/Y | Crossfade of the algorithm's outputs: 0 = X, 127 = Y |
| Filter (select 6) | 1 | `frq` `res` `typ` `drv` | Filter … | As on the LXR; `typ` adds **`lpg`**, the low-pass gate, after `off` |
| | 2 | `env` | Filter Env | The Amp EG moves the cutoff: −63…+64, ±63 = ±6 octaves, 0 = off ([filter envelope](#the-filter-envelope-and-the-low-pass-gate)) |
| | 2 | `dcy` `col` `exc` | LPG Decay / Colour / Excite | With `typ` `lpg`, in place of `env` ([low-pass gate](#the-filter-envelope-and-the-low-pass-gate)) |
| Mix (select 8) | 1 | `vol` `pan` `srt` `drv` | Voice … | Level, pan, sample-rate reduction, drive, as on the LXR |

The folder takes the whole FM sound (X and Y), not the sub.

### Randomise on FM Tone

With `rpl` at `all`, randomise works as on FM Drum (algorithms 1–7, A and B1). The other palettes (`bas`, `hrm`,
`met`) are made for bass on `fmt`: they also draw algorithm 8 and B2's ratio and depth. `hrm` and `met` also
draw C's ratio (mostly ×1, sometimes ×0.5 or ×2: an octave switch now and then); `bas` never moves C, so the bass
stays in the octave you set. They keep the depths lower and the feedback mostly off, give A, B1 and B2 longer envelopes
(attacks from instant to a few hundred ms) and mostly no pitch sweep. See [Randomise](#randomise).

---

## mdk — Machinedrum-style drum machines

`eng` = `mdk`. Thirty drum machines after the three synthesis families of the Elektron Machinedrum, from the
[OP-XY-MNMD](https://github.com/mmockett/OP-XY-MNMD) project by mmockett and the OP-XY MNMD contributors (open source, MIT: written from scratch
and fitted by ear and measurement to the Machinedrum's own sound, no Elektron code), with three headers of the
Open-XY SDK (MIT). Each machine keeps its Machinedrum controls (up to eight).

| Family | Letter | How it makes sound | Character |
|---|---|---|---|
| **TRX** | `t` | Classic analog-style drum machine voices: sine and pulse oscillators, noise, pitch ramps, filters | The vintage drum machine: round kicks, snappy snares, claps with a room, metallic hats and cymbals |
| **EFM** | `f` | Two-operator FM: a sine carrier modulated by a sine modulator at a ratio (`MFRQ`), with its own envelope (`MDEC`) and feedback | Digital and punchy: from clean to clangorous, bells and noise as `MOD` goes up |
| **P-I** | `p` | Physical models: a struck body (membrane, shell, bar, plate, seeds) ringing in its modes, set by how hard and where it's hit and what it's made of | Acoustic and alive: toms, snares and metals that vary a little with every hit |

You choose a machine **by sound type** (`typ`), then the machine (`mch`); each type mixes the families. The
machine's short name starts with its family letter. The edit screen shows its full name ("TRX BD"), and the FM page
names its knobs.

**Kick** (`typ` `kik`)

| `mch` | Machine | Sound | Knobs |
|---|---|---|---|
| `tbd` | TRX BD | The classic kick: a click, then one sine with a pitch ramp | PTCH DEC RAMP RDEC · STRT NOIS HARM CLIP |
| `tb2` | TRX B2 | The second classic kick: a sine that swells, holds (`HOLD`), with a tick, noise, dirt and distortion | PTCH DEC RAMP HOLD · TICK NOIS DIRT DIST |
| `fbd` | EFM BD | FM kick: a carrier modulated by one operator; `RAMP` falls from above or rises from below | PTCH DEC RAMP RDEC · MOD MFRQ MDEC MFB |
| `pbd` | P-I BD | A struck membrane: two modes, the pitch settling as it rings | PTCH DEC HARD HAMR · TENS DAMP |

**Snare** (`typ` `snr`)

| `mch` | Machine | Sound | Knobs |
|---|---|---|---|
| `tsd` | TRX SD | The classic snare: a sine body with a small pitch bump, and noise (`SNAP`) | PTCH DEC BUMP BENV · SNAP TONE TUNE CLIP |
| `fsd` | EFM SD | FM snare: a fed-back modulator spreads the tone into a rough band, plus noise | PTCH DEC NOIS NDEC · MOD MFRQ MDEC HPF |
| `psd` | P-I SD | A struck head over snare wires: five head modes and the rattling wires (`RVOL`, `RDEC`) | PTCH DEC HARD RING · TENS RVOL RDEC |

**Clap** (`typ` `clp`)

| `mch` | Machine | Sound | Knobs |
|---|---|---|---|
| `tcp` | TRX CP | The classic clap: a train of noise hands, then a tuned room | CLPY TONE HARD RICH · RATE ROOM RSIZ RTUN |
| `fcp` | EFM CP | FM clap: an FM tone under a train of claps (`CLPS` how many, `CDEC` their spacing) and a tail | PTCH DEC CLPS CDEC · MOD MFRQ MDEC HPF |

**Hat** (`typ` `hat`)

| `mch` | Machine | Sound | Knobs |
|---|---|---|---|
| `tch` | TRX CH | The classic closed hat: six square and pulse oscillators (the cymbal's) and noise, filtered | GAP DEC HPF LPF · MTAL |
| `toh` | TRX OH | The same hat, open: it holds and falls longer | GAP DEC HPF LPF · MTAL |
| `fhh` | EFM HH | FM hat: a fed-back carrier whose phase jumps, so it never settles on a pitch; tremolo (`TREM`, `TFRQ`) | PTCH DEC TREM TFRQ · MOD MFRQ MDEC FB |
| `phh` | P-I HH | A struck metal plate: sixteen inharmonic partials; `CLOS` closes it | PTCH DEC CLSN RING · AG AU BR CLOS |

**Cymbal** (`typ` `cym`)

| `mch` | Machine | Sound | Knobs |
|---|---|---|---|
| `tcy` | TRX CY | The classic cymbal: the hats' oscillators, rich and long, with a top end (`TOP`, `TTUN`) | RICH DEC TOP TTUN · SIZE PEAK |
| `fcy` | EFM CY | FM cymbal: four sines at the Machinedrum's ratios, each with its own modulator | PTCH DEC FB HPF · MOD MFRQ MDEC |
| `prc` | P-I RC | A struck plate, ride: partials from 1.3 to 6 kHz | PTCH DEC HARD RING · AG AU BR GRAB |
| `pcc` | P-I CC | A struck plate, crash: brighter partials, a longer wash | PTCH DEC HARD RING · AG AU BR GRAB |

**Perc** (`typ` `prc`)

| `mch` | Machine | Sound | Knobs |
|---|---|---|---|
| `txt` | TRX XT | The classic tom: a click, then a sine with a pitch ramp | PTCH DEC RAMP RDEC · DAMP DIST DTYP |
| `fxt` | EFM XT | FM tom: the FM kick's voice, two octaves higher and thinner, with a click | PTCH DEC RAMP RDEC · MOD MFRQ MDEC CLIC |
| `pmt` | P-I MT | A struck head over a resonant shell (`TUNE`, `SIZE`), hit at a position (`POS`) | PTCH DEC HARD HAMR · TUNE DAMP SIZE POS |
| `txc` | TRX XC | The classic conga: the tom's model, shorter click, higher | PTCH DEC RAMP RDEC · DAMP DIST DTYP |
| `trs` | TRX RS | The classic rim shot: a burst of filtered saw over a short tone | PTCH DEC DIST |
| `frs` | EFM RS | FM rim shot: two FM tones, the rim and a snare under it | PTCH DEC MOD HPF · SNAR SPTC SDEC SMOD |
| `prs` | P-I RS | A stick across rim and head: nine fixed rim modes over the tuned head | PTCH DEC HARD RING · RVOL RDEC |
| `tcb` | TRX CB | The classic cowbell: a pulse beating against a sine, different every hit | PTCH DEC ENH DAMP · TONE BUMP |
| `fcb` | EFM CB | FM cowbell: two sines, each with its own modulator | PTCH DEC SNAP FB · MOD MFRQ MDEC |
| `tcl` | TRX CL | The classic claves: a pure sine; `DUAL` strikes twice | PTCH DEC DUAL ENH · TUNE CLIC |
| `tma` | TRX MA | The classic maracas: grains of high noise, with attack, sustain and a reverse (`REV`) | ATT SUS REV DAMP · RATL RTYP TONE HARD |
| `pma` | P-I MA | Seeds rattling in a shell: how many (`GRNS`), how long they last (`GLEN`), the shell's size | GRNS DEC GLEN · SIZE HARD |
| `pml` | P-I ML | A struck bar, marimba-like: seven inharmonic modes | PTCH DEC HARD TENS |

**Common knobs:** `PTCH` pitch, `DEC` decay. TRX and EFM: `RAMP` the pitch drop at the start and `RDEC` how fast it
falls; `NOIS` noise; `CLIP` / `DIST` drive. EFM: `MOD` FM depth, `MFRQ` the modulator's ratio (`MFRQ`/16), `MDEC`
its envelope, `FB` / `MFB` feedback, `HPF` a high-pass. P-I: `HARD` how hard it's struck (brighter), `HAMR` the
hammer's softness, `TENS` tension, `DAMP` damping, `RING` how long the body rings, `RVOL` `RDEC` the snare wires' or
rim's level and decay. The P-I metals: `AG` `AU` `BR` blend the alloy (silver, gold, bronze: brighter or darker,
shorter or longer), `GRAB` below 80 grabs (chokes) the cymbal after a moment, and on the hat `CLOS` is the pedal
(127 closed; lower rings open longer, then closes with a chick). Knobs a machine doesn't have are blank and skipped on
the FM page.

**Each machine keeps its own envelopes** (amp, pitch ramps, the clap's bursts): they are its sound. Around it:

```
machine → wavefolder → + transient → filter (+ filter envelope) → drive → mixer
```

- The **Amp EG doesn't set the volume** on this engine; it shapes the **filter envelope** (`env`) and the wavefolder's envelope. No pitch sweep (the machines have their own). The **transient**
  stays, to layer on top: a click on a kick, a tick on hats (`vol` 0 = none).
- **Pitch:** each machine's own pitch knob tunes it; the drum's tune and the step's or MIDI note transpose from there (C4 plays
  the machine as voiced).
- **Changing the machine** (by hand) loads its default knobs, so each starts from a sound that works.
- **Your machine and the others:** the machine your `typ` and `mch` choose is the drum's **home machine**, and the
  knobs belong to it. Any other machine that plays (by `rmc`) reads the knobs
  **relative**: it starts from its own default sound and moves as far as you've moved the knobs from the home
  machine's defaults. So the knobs stay live while machines change: `DEC` up by 20 lengthens every machine's
  decay by 20 from its own.

### Pages

On Drum 1–3:

| Page | Screen | Short | Edit name | What it does |
|---|---|---|---|---|
| Osc (select 1) | 1 | `coa` `fin` | Oscilltr Coarse / Fine | Transpose: C4 plays the machine as voiced |
| | 1 | `eng` | Voice Engine | The engine |
| | 1 | `typ` | MD Type | Sound type: `kik` `snr` `clp` `hat` `cym` `prc` |
| | 2 | `mch` | MD Machine | That type's machines; picking one by hand loads its default knobs |
| Amp EG (select 2) | 1, 2 | `atk` `dec` `slp` `hld` · `sus` `rel` `gat` | Veloc EG … | **Doesn't set the volume** (the machine does): shapes the filter envelope, the folder's depth |
| Mod (select 3) | 1 | `rmc` `rkn` `rsd` `rpl` | Random Machine / Knobs / Seed / Palette | Randomise (below). No pitch sweep: the machines have their own |
| FM (select 4) | 1, 2 | `k1`–`k8` | the machine's own | **The machine's knobs**, 4 and 4, each shown by the machine's own name (`ptc dec rmp rdc`…; the edit screen spells it, "RDEC"); unused knobs are blank and skipped |
| Transient (select 5) | 1 | `wav` `vol` `frq` | Transnt Wave / Volume / Freq | The LXR's transient: a click or sample layered on the machine (a click on a kick, a tick on hats; `vol` 0 = none) |
| | 2 | `fld` | FM Fold | Wavefolder. Its depth **follows the Amp EG** (or the LPG): each hit folds hard, the tail cleans up. 0 = off |
| | 2 | `fty` | FM FoldType | `sin`: smooth, round folds; `tri`: sharper, edgier |
| | 2 | `sym` | FM Symmetry | Off centre adds even harmonics: richer, buzzier. −63…+64 |
| | 2 | `lev` | FM Body lvl | The level after the folder, against the transient. 100 = unity |
| Filter (select 6) | 1 | `frq` `res` `typ` `drv` | Filter … | As on the LXR; `typ` adds **`lpg`**, the low-pass gate, after `off` |
| | 2 | `env` | Filter Env | The Amp EG moves the cutoff: −63…+64, ±63 = ±6 octaves, 0 = off ([filter envelope](#the-filter-envelope-and-the-low-pass-gate)) |
| | 2 | `dcy` `col` `exc` | LPG Decay / Colour / Excite | With `typ` `lpg`, in place of `env` ([low-pass gate](#the-filter-envelope-and-the-low-pass-gate)) |
| Mix (select 8) | 1 | `vol` `pan` `srt` `drv` | Voice … | Level, pan, sample-rate reduction, drive, as on the LXR |

On the snare, cymbal and hi-hat, see [Snare, cymbal and hi-hat](#snare-cymbal-and-hi-hat).

### Randomise on mdk

| Setting | What it does |
|---|---|
| `rmc` Random Machine | Which machine plays, from the palette. It doesn't change your `typ` / `mch` (your home machine); 0 = your machine again |
| `rkn` Random Knobs | Your machine's 8 knobs, re-rolled around its own default sound and written as if you'd turned them (grab any and keep tweaking); 0 restores yours. The pitch stays within about 24 steps of its default; the decay is mostly short (about 70 % in the bottom quarter, 20 % medium, 10 % long, never the 127 hold) |
| `rsd` Random Seed | Another set of draws for both |
| `rpl` Random Palette | Where `rmc` draws from: `typ` your machine's type (any family), `fam` its type and family, `all` any machine |

- `DEC` at 127 holds some machines (the kicks, most FM ones), as on the original: the sound doesn't end by itself.

---

## pla — Plaits

`eng` = `pla`, on **every voice** (Drum 1–3, snare, cymbal, hi-hat). Twelve models from **Mutable Instruments
Plaits** (Emilie Gillet, MIT license), running Mutable's own code: they sound as on Plaits. Each model has
Plaits' three knobs, `hrm` (HARMONICS), `tmb` (TIMBRE) and `mor` (MORPH), plus **`twi`**, the TWIST control from
[Plaits Palette](https://rubato.audio/plaits-palette/) ([TWIST](#twist-twi)). The edit screen names what each
knob does in the model you're on: editing `hrm` shows "Plaits Harmonics" with "Material" on the modal resonator,
the patch's number and name on the 6-op.

### Banks and models

Pick a bank with `bnk`, then the model with `mdl`. A new model starts at the next hit.

| `bnk` | `mdl` | Model | `hrm` | `tmb` | `mor` | `out` / `aux` |
|---|---|---|---|---|---|---|
| `drm` Drums | `bd` | Bass drum | Attack and drive | Brightness | Decay | Analog / Synthetic |
| | `mod` | Modal resonator | Material | Brightness | Decay | Resonator / Exciter |
| | `str` | Inharmonic string | Inharmonicity | Brightness | Decay | String / Exciter |
| `ton` Tonal | `wav` | Wavetable | Bank | Row | Column | Clean / Lo-fi 5-bit |
| | `fm2` | 2-operator FM | Ratio | Mod index | Feedback | FM / Sub oscillator |
| | `dx7` | 6-operator FM (DX7 patches) | Patch | Brightness | Envelope time | — ([6-op FM](#the-6-op-fm-dx7)) |
| | `ter` | Wave terrain | Terrain | Path size | Path move | Terrain / Path sine |
| | `va` | Virtual analog | Detune | Square | Saw | Waves / Sync |
| | `shp` | Waveshaping | Shaper | Fold | Asymmetry | Fold / Fold 2 |
| | `pd` | Phase distortion | Ratio | Amount | Asymmetry | Sync PD / Free PM |
| | `add` | Harmonic (additive) | Peaks | Prominent harmonic | Peak shape | Additive / Drawbars |
| | `chd` | Chords | Chord | Inversion | Waveform | Chord / Root |

`out` picks Plaits' OUT, AUX or a mix of both (`mix`); like the model, it changes at the next hit.

### Pages

| Page | Settings |
|---|---|
| Osc (select 1) | `coa fin eng bnk` \| `mdl out` (the 6-op: `mdl kmd`) \| on `wav` and `ter`: `sub sbs sbo sbf`, the [sub](#the-sub-on-the-wavetable-and-terrain) \| `oct key scl` |
| Mod (select 3) | The LXR's pitch sweep ([below](#drums-and-tonal-models)) |
| FM (select 4) | `hrm tmb mor twi` (the 6-op, the wavetable and the wave terrain: `hrm tmb mor twi` \| `fil`: the 6-op's bank `A`–`C`; on the wavetable and terrain the built-in tables) |
| Transient (select 5) | `wav vol frq` \| `fld fty sym lev`: the transient layered on top, and the wavefolder |

The Amp EG, Filter and Mix pages are the voice's own. All knobs are 0–127 with 64 at noon; `twi` shows −63…+64.

### Drums and tonal models

- **Drum models** (`bd`, `mod`, `str`, the 6-op in `ptc`) shape their own volume, as
  on Plaits: the decay is `mor`, and the Amp EG doesn't change their volume. They make their own pitch sweep, so
  the Mod page's sweep doesn't apply.
- **Tonal models** play through the voice's **Amp EG** (attack, hold, decay, and gate mode for held notes) and take
  the Mod page's pitch sweep. A tonal voice whose Amp EG has closed isn't computed at all.
- Every model stops computing about 40 ms after it falls silent, so a voice only uses the CPU while it sounds.

### The models, one by one

**`bd` Bass drum** — Plaits' analog kick (`out`) and its synthetic kick (`aux`). `hrm` low is a clean sine with a
soft attack; up adds the attack's FM click, then self-FM, then drive. `tmb` brightens the body, `mor` sets the
decay. `out` analog, `aux` synthetic, `mix` both layered.

**`mod` Modal resonator** — a struck object, from a mallet to a bell. `hrm` is the material: low is wood and
plates, up through metal to glass and bells (more inharmonic). `tmb` brightens the strike, `mor` lengthens the ring.
`aux` is the exciter alone (a click).

**`str` Inharmonic string** — a plucked string: `hrm` from a clean string up to inharmonic, sitar-like and metallic
ones; `tmb` brightness, `mor` decay. `aux` the pluck alone.

**`wav` Wavetable** — four banks of 8 × 8 single-cycle waves; `tmb` and `mor` move across the grid, blending
neighbours, `hrm` picks the bank (A, B, C; the fourth is a mix of the other three,). `aux` is a 5-bit lo-fi copy:
gritty and aliased on purpose. For a clean sound keep `out` on `out`.

**`fm2` 2-operator FM** — one sine modulating another. `hrm` steps through musical ratios (sub-octaves to high,
inharmonic ones), `tmb` is the modulation depth (brightness), `mor` the feedback: left of noon the modulator
feeds back into its own phase (rough, chaotic), right of noon into its output (a buzzy, saw-like edge). `aux` is a
sub oscillator an octave down.

**`ter` Wave terrain** — a sine orbit drawn across a 2D surface: the sound is the height along the path. `hrm`
picks the terrain (eight), `tmb` the orbit's size (low = pure, up = richer), `mor` moves it across the surface
(and `twi` up and down). `aux` is the path's own sine.

**`va` Virtual analog** — two oscillators. `tmb` sweeps the first from a narrow pulse through square to a
hard-synced square, `mor` the second from triangle to saw (with a notch), `hrm` detunes them (from unison to
musical intervals). `aux` is a hard-sync sound.

**`shp` Waveshaping** — a triangle through a waveshaper and a wavefolder: `hrm` picks the shaper's curve, `tmb` folds
harder (louder harmonics), `mor` adds asymmetry. `aux` is a second folder with a different character.

**`pd` Phase distortion** — Casio CZ-style. `hrm` sets the modulator's ratio, `tmb` the distortion (filter-sweep-like
brightness), `mor` the asymmetry or pulse width. `out` is synced and smooth, `aux` free-running and wilder.

**`add` Harmonic (additive)** — up to 24 harmonics. `tmb` picks the most prominent one (the spectrum's centre), `mor`
how peaked or flat the spectrum is around it, `hrm` adds more peaks. `aux` is drawbar organ harmonics.

**`chd` Chords** — four-voice chords from one pitch: `hrm` picks the chord (octaves, fifths, minor, major, 7ths,
sus…), `tmb` the inversion (it climbs as you turn), `mor` the waveform: left of noon divide-down organ and string
machine sounds, right of noon wavetables. `aux` is the root note alone. With `scl` on the pitch screen the root
lands in key.

### The sub on the wavetable and terrain

`wav` and `ter` have `fmt`'s [sub oscillator](#the-sub-oscillator), with the same four settings, on a screen of the
Osc page after `mdl out`: `sub` (level, 0 = off, the default), `sbs` (shape `sin` `tri` `saw` `sqr`), `sbo` (1 to 4
octaves below the played pitch) and `sbf` (its own folder: a little saturation, then folds). It restarts with each
hit, follows the pitch, and goes through the voice's filter or LPG and Amp EG with the rest. One sub per voice,
shared by `wav` and `ter`; the other models don't have it. At 127 it's about as loud as the wavetable itself: with
both up, turn the voice's level down a little if it clips. The sub costs some CPU (more with `sqr` and `sbf`): watch the readout with several voices on it.

### The 6-op FM (`dx7`)

Six-operator FM playing **DX7 patches**, as Plaits does. The FM page has `fil`, the patch bank, after `twi`; `hrm`
picks one of its 32 patches (the edit screen shows the patch's number and name). `tmb` is the brightness
(the modulators' levels), `mor` the envelope time. `out` does nothing on this model, so its place on the Osc page
holds the key mode `kmd`.

| `fil` | Bank |
|---|---|
| `A` `B` `C` | Plaits' own three banks (Plaits 1–3), built in |

DX7 banks from the SD card come with a later release.

**`mor`, the envelope time:** 64 plays the patch as programmed. Lower is snappier, shortest around 30–45 (below
that the release grows again); higher slows everything, with longer rings, tails and slower attacks (90 and up).

**`kmd`, the key mode:**

| `kmd` | The key | The volume |
|---|---|---|
| `ptc` (default) | A hit presses the DX7 key briefly; on Drum 1–3 a held MIDI note holds it | The patch's own envelopes, as on Plaits |
| `frz` | None: the sound is frozen at one moment of a held note, and `mor` scrubs through the patch's timeline (left the start, right about 1.5 s in) | The voice's Amp EG |

- **Bells and plucks:** `ptc`, short triggers: they ring out on their own.
- **Bowed, pads and synth sounds:** on Drum 1–3, hold a MIDI note:
  the key stays down and the patch sustains for as long as you hold it, then plays its release. The snare, cymbal
  and hi-hat have no gate: use `frz` there and shape the sound with the Amp EG.

### TWIST (`twi`)

A fourth knob on every model, from **Plaits Palette** (Rubato Audio): it reaches a part of the sound the three
knobs don't. **At 0 (the default) every model sounds exactly as Mutable's Plaits**; turning either way changes it.
The edit screen names it for the model.

| Model | `twi` | Left (−) | Right (+) |
|---|---|---|---|
| `bd` | Punch | A softer attack | More attack FM: a harder, clickier kick |
| `mod` | Hardness | A soft, felt mallet | A hard, bright strike |
| `str` | Pluck size | A short, thin pick | A long burst: a fatter, noisier pluck |
| `wav` | Phase warp | Bends each wave one way | Bends it the other way (new timbres from the same tables) |
| `fm2` | Ratio shift | The modulator up to an octave down, continuously | Up to an octave up: inharmonic in between, bells to clangs |
| `dx7` | Detune / tone | The modulators down (up to an octave) and the sound darker | The modulators up and the sound driven (louder, saturated) |
| `ter` | Path Y | Moves the orbit down the terrain | Moves it up (`mor` moves it sideways) |
| `va` | Saw / square | Only the saw | Only the square |
| `shp` | Symmetry | Folds asymmetric one way | The other way: even harmonics, more grit |
| `pd` | Mod octave | The modulator an octave down | An octave up |
| `add` | Odd / even | Odd harmonics (hollow, square-like) | Even harmonics |
| `chd` | Root level | The chord without its root | The root louder |


### CPU

Plaits' models are heavier than the LXR's own voices. Measured with a busy pattern (the CPU screen, below): one
voice on the bass drum about 18 % of the CPU, the heaviest models (modal resonator, harmonic, phase distortion)
about 26–28 %. Drum models cost CPU only while they ring; a tonal model whose Amp EG has closed costs nothing.

---

## Snare, cymbal and hi-hat

Their `eng`: `lxr` (their own sound, as always), **`mdk`** or **`pla`** (every Plaits model, with the
same pages as on Drum 1–3: [pla — Plaits](#pla--plaits)). The engines work exactly as on Drum 1–3:
the same algorithms and machines, live knob names, relative knobs, randomise, the transient layered on top, the
wavefolder and the filter envelope. They have no hold or gate mode, and no low-pass gate (Drum 1–3 only).

### With `lxr`

Their own sound, as on the stock firmware (see Erica Synths' LXR manual). UMA changes: `eng` on the Osc page,
which moves the snare's settings to `coa fin eng noi` | `mix wav pwm` and the cymbal's and hi-hat's to
`coa fin eng wav` | `pwm`; the [filter envelope](#the-filter-envelope-and-the-low-pass-gate) `env` on Filter
screen 2.

### Pages with `mdk`

The Osc, FM, Mod and Transient pages change to the engine's; the Amp EG, Filter and Mix pages stay the voice's own.

| Page | `mdk` |
|---|---|
| Osc (select 1) | `var p1 p2` |
| Mod (select 3) | `rmc rkn rsd rpl` |
| FM (select 4) | the machine's knobs `k1`–`k8`, named live |
| Transient (select 5) | `fld fty sym lev` |

- **The hi-hat's two triggers:** with mdk the closed trigger plays your machine and the open one its open version: TRX CH plays TRX OH, P-I HH opens its `CLOS`,
  any other machine rings 40 longer (`DEC`). A closed hit cuts an open one, as always.
- **mdk** starts on the voice's own type: snares on the snare, cymbals on the cymbal, hats on the hi-hat.


---

## Randomise

On Drum 1–3 with `fmd` or `fmt` (Mod page: `rpl` on screen 1, `rto rev rsw rsd` on screen 2, `rfn` on screen 3). Each value re-rolls a group of settings; **each number always gives the
same sound** on its drum, so you can return to one you liked. The `mdk` engine has its own randomise, on every
voice: [Randomise on mdk](#randomise-on-mdk). `pla` has no randomise:
each model has only its three knobs and `twi`.

| Setting | Re-rolls | |
|---|---|---|
| `rto` Random Tone | algorithm, ratios, mods, feedback | each ratio: about half harmonic or an interval, a third metallic, the rest anything |
| `rev` Random Env | attack, decay and end of A and B | decays fit the Amp EG's hold and decay |
| `rsw` Random Sweep | the pitch sweep's decay, slope and amount | |
| `rfn` Random Fine | fine ratio A and B | **1–63**: only metallic ratios get detuned; **64–127**: every ratio (abstract). Follows `rto`. With `bas` and `hrm`: metallic ratios only, at any value |
| `rsd` Random Seed | picks the set of random sounds the others use | |
| `rpl` Random Palette | which ratios `rto` draws | see below |

**The ratio palette** `rpl`. The operators that modulate C, or are heard, set the pitch you hear. A ratio
between 1 and 2 that isn't 1 or 2 (×1.2, ×1.41, ×1.73…) puts a tone *below* the root, so the pitch blurs: great
for drums, wrong for a bass. Musical intervals above 2 (×2.25, ×2.67) add a second note.

| `rpl` | Operators that modulate C or are heard | The others (they modulate a modulator) |
|---|---|---|
| `all` | any ratio, as always: gritty drums | the same |
| `bas` | whole numbers ×1–×9 (and ×0.5), or metallic from ×2.82 up (grit above the root) | whole numbers, intervals, metallic from ×2.82 |
| `hrm` | whole numbers ×1–×9 | whole numbers, ×0.5, ×0.25 |
| `met` | metallic only | metallic only |

- It works on `fmd` too (tuned toms, 808-style kicks with `bas` or `hrm`); there it changes only the ratios.
- On `fmt`, `bas` `hrm` `met` also give bass ranges (see [Randomise on FM Tone](#randomise-on-fm-tone)).
- `all` gives exactly the sounds of earlier versions, so every number you noted still sounds the same. Kits from
  before 0.28 load with `all`.
- Changing `rpl`, or `eng` with a palette other than `all`, draws again for the groups not at 0 (settings only
  the old palette set go back to your own).

- **0 restores** the sound from before the re-rolls; after **saving the kit**, 0 restores the saved sound.
- Changing `rsd` re-rolls the groups that aren't at 0. Loading a kit never re-rolls.
- **Not randomised:** the wavefolder.

---

## Modulation

*0.3 test builds.* Each voice has **8 mod slots**. A slot takes a source (an LFO, a Turing Machine, the envelope,
velocity), scales it and moves one of its voice's settings: any UMA setting of the voice, and the stock settings
on its list.

### The mod view: SHIFT + LFO

Select a voice and press **SHIFT + LFO** (select 7) briefly. The display shows the **mod view**:

- The **8 select buttons pick slot 1–8** (Osc = slot 1 … Mix = slot 8). The slot number shows in the top right
  corner of the display: when you see it, the select buttons pick slots.
- The **voice buttons** change the voice; you stay in the mod view.
- **SHIFT + LFO** again leaves it, back to the page you came from. Going to a page that isn't a voice's (global,
  performance, step mode…) leaves it too.

Each slot is one screen:

| Short | What it does | Values |
|---|---|---|
| `src` | Source | see the sources below |
| `amt` | Amount | 0 = off |
| `dst` | Destination | a setting of this voice |
| `pol` | Polarity | `bi` around the knob, `uni` from the knob up |

- With `pol` **`bi`** the modulation moves a setting **up and down around its own value**: the knob is the centre,
  `amt` the range either way. With **`uni`** the knob is the **floor**: the source's lowest value plays the knob,
  its highest the knob + `amt`, nothing goes below.
- **Editing `src` or `dst`, the edit screen names them in full**: "LFO B Drum2", "TM C Cymbl", "Env Drum1";
  "Op A Ratio". A target of another engine stays set but does nothing ("fm only", "mdk only"…), so a slot keeps
  its target when you switch `eng`.
- New slots (kits from before 0.3) start on their voice's LFO A with `amt` 0: nothing moves until you turn `amt` up.
- A few targets are on the list but do nothing on the desktop: `vel`, the operator waveforms `wvc` `wvm`, the
  morph `mrp` and LFO A's fold.

### Sources

The number in a source's name is the voice: 1–3 Drum 1–3, 4 Snare, 5 Cymbal, 6 Hi-hat (both hats). Any slot can use
any voice's sources.

| Source | What it is |
|---|---|
| `LA1`–`LA6` | Each voice's **LFO A**: the stock LFO on its LFO page. Its own target and amount there work as before, separately |
| `LB1`–`LB6` | Each voice's **LFO B** ([LFOs](#lfos)) |
| `T1A`–`T6D` | Each voice's four **Turing Machines** ([Turing Machines](#turing-machines)) |
| `EN1`–`EN6` | Each voice's Amp EG, **adding on top**: the setting's value at rest, up by `amt` at the peak |
| `EI1`–`EI6` | The same, **inverted**: down by `amt` at the peak (ducking) |
| `VL1`–`VL6` | Each voice's **last hit velocity**: the step's velocity, or a MIDI note's. 0 to 1, 127 = 1 |

The list also has the Eurorack's CV inputs, accent jacks and MIDI CC sources (`cv1`–`cv5`, `ac1`–`ac6`,
`MC1`–`MC8`); the desktop has none of them and skips them as you turn `src`. They keep their places so a kit means
the same on both firmwares.

### Velocity

- **To the volume:** the stock switch `vol` on the Mod page's velocity screen (screen 2 with `lxr`; screen 3 with
  `fmd` `fmt` `mdk` `pla`, after randomise). On, the step's velocity sets the hit's level; off, every hit plays at
  full level. It works on every engine.
- **To the sound:** a slot with `VL` as its source: harder hits brighter (`VL2` → `mA`), more fold, a different
  Plaits timbre. Plaits plays every hit with the same accent, so this is how its tone follows velocity.

### Pitch: `v/o`

Every voice's target list has `v/o`: a modulation on it **transposes the note** that plays (the step's, or a MIDI
note's) up to 5 octaves, then the [quantizer](#pitch-octave-key-and-scale) snaps it to the voice's key and scale.
Its knob is 0, so use `pol` **`uni`**: `amt` is then the span above the note. A Turing Machine on `v/o` with the
quantizer on plays melodies; with the quantizer off the pitch moves continuously.

### Modulating a modulation (`a1`–`a8`)

Every voice's list ends with `a1`–`a8`, the **amounts of its own 8 slots**: one slot can scale another's depth. For
example slot 1 `T2A` → `RT` (a pattern of re-rolled tones), slot 2 a slow `LB2` → `a1`: the pattern fades in and out
over bars.

### When a modulation counts

| Target | Counts |
|---|---|
| Randomise settings, LPG `exc` | once, **at the hit** |
| Everything else | continuously (every audio block, about 1.5 ms) |

### Tips

- **LFO → `mA`** (Op A Mod) or `eA`: the FM brightness pulses.
- **`VL` → `mA`, `fb` or `exc`**: harder hits are brighter, grittier or stronger.
- **A locked Turing Machine → `al` or `rA`**: a timbre pattern that repeats with the beat.
- **`EN` → `fld`**: extra fold on the hit. **`EI1` → `vol`** in a Drum 2 slot: Drum 1 ducks Drum 2.

---

## LFOs

Every voice has **two LFOs**. **LFO A** is the stock LFO, as on Erica's firmware (its shapes, sync and target on the
LFO page). **LFO B** is the Eurorack's: 16 shapes you can sweep through, a folder and its own sync. Select the voice,
press **LFO** (select 7), then press it again to step through the screens:

| Press | Screen |
|---|---|
| 1, 2 | LFO A (stock) |
| 3 | LFO B: `frB snB wvB rtB` |
| 4 | LFO B: `ofB fdB` |
| 5 | Turing A \| B ([Turing Machines](#turing-machines)) |
| 6 | Turing C \| D |
| 7 | back to LFO A |

| Short | What it does |
|---|---|
| `frB` | Speed, up to about 200 Hz (while sync is off) |
| `snB` | `off`, or a division of the sequencer's tempo: the LFO's period follows the BPM |
| `wvB` | 0–127: a sweep through the shapes below |
| `rtB` | `off`; `v1`–`v6`: the voice whose hits restart it; `1b` `2b` `4b` `8b`: restart every 1, 2, 4 or 8 bars, counted from when the sequencer starts (free-running in between) |
| `ofB` | Where the wave restarts when retriggered (only matters with `rtB` on) |
| `fdB` | A wavefolder after the shape: 0 = off, 1–24 a light bend, then the wave folds back on itself, more turns the higher |

**Shapes.** `wvB` sweeps through the shapes; each has a small flat zone where it is pure, and between two
neighbours the LFO crossfades from one to the other. The display shows the nearest shape; the edit screen shows the
pure shape's name, or the blend (`sin>tri 40%` = 40 % of the way from sine to triangle). Because it is a sweep, a
Turing Machine or another LFO on `wvB` changes the shape continuously.

| # | Name | Shape | | # | Name | Shape |
|---|---|---|---|---|---|---|
| 0 | `sin` | Sine | | 8 | `scv` | S-curve: smooth rise and fall |
| 1 | `tri` | Triangle | | 9 | `bnc` | Bounce: four bounces, falling |
| 2 | `sup` | Saw up: rises, then drops | | 10 | `hsn` | Half sine, then rest |
| 3 | `sdn` | Saw down: falls, then jumps up | | 11 | `st4` | 4 stairs up |
| 4 | `sqr` | Square | | 12 | `st8` | 8 stairs up |
| 5 | `rnd` | Random, stepped: a new value each cycle | | 13 | `rsm` | Random, gliding to a new value each cycle |
| 6 | `xup` | Exponential up: rises slowly, then fast | | 14 | `trp` | Trapezoid: rise, hold, fall, rest |
| 7 | `xdn` | Exponential down: falls fast, then slowly | | | | |

**Fold.** `fdB` folds whatever shape the LFO plays: low values only bend it, from about 30 the peaks fold over and a
single cycle gets extra turns. A Turing Machine or another LFO can move the fold while the shape stays.

**Sync** divisions, slowest to fastest, in fractions of a bar (one LFO cycle): `4/1` `2/1` `1/1` `1/2`, `4.`
(dotted quarter), `1/3`, `1/4`, `8.` (dotted eighth), `1/6`, `1/8`, `16.` (dotted sixteenth), `12`, `16`, `24`,
`32`. `1/3` `1/6` `12` `24` are triplets. With sync on, `frB` does nothing; retrigger still restarts the LFO.

An LFO B does nothing until a slot uses it (`LB1`–`LB6`). New kits start with a slow sine, free-running.

---

## Turing Machines

Every voice has **four Turing Machines**, A–D: looping random sequences after the Music Thing Modular Turing
Machine. Each moves one step on a hit (or on the sequencer's sixteenths) and gives a value a slot can use
(`T1A`–`T6D`). On the LFO page: the 5th press of **LFO** shows A | B, the 6th C | D (the first four knobs the first
machine, the last four the second).

| Short | Values | What it does |
|---|---|---|
| `lnA` | 1–64 | Loop length in steps |
| `prA` | 0–127 | Chance a step changes when it comes round: 0 = locked (repeats exactly), 127 = random every step |
| `ckA` | `own`, `dr1` `dr2` `dr3` `snr` `cym` `hat`, `seq` | Whose hits move it; `seq`: the sequencer's sixteenths (while it plays) |
| `dvA` | 1–16 | One step every Nth of those (with `seq`: 4 = every beat, 16 = every bar) |

(B, C, D the same with their letter.) **Patterns are saved with the kit**, all 64 steps: save the kit and a locked
loop comes back exactly. To find a new locked pattern, turn `prA` up for a few hits, then back to 0. New kits start
locked, 8 steps long, on their own voice's hits.

Ideas:
- A locked machine on `v/o` (`pol` `uni`) with the quantizer on: an 8-note melody.
- `prA` 20–40: a pattern that slowly evolves.
- Three machines with lengths 3, 5 and 8 on different targets: polyrhythm.
- `ckA` `dr1` on the snare: the snare's timbre follows the kick.
- **A new LFO shape every bar:** sync LFO B (`snB` `1/1`), set a machine to `seq` with `dvA` 16 and route it to
  `wvB`.

---

## Kit morph

The stock **X-fade** (performance page, and the Euklid page) blends the current kit towards the morph kit: load one
with the load menu's morph type, as on Erica's firmware (after a project loads it is the first kit). On LX_UMA the morph
reaches **UMA's settings** too: the engines' sounds, envelopes, LPG, fold, LFO B, the pitch screen, Plaits, mdk and
`v/o`. Modulation moves around the blend.

| How | Settings |
|---|---|
| Blend | all FM, mdk and Plaits knobs, fine ratios, fold, LPG, hold, the filter envelope, LFO B's speed, offset, shape and fold, `v/o` |
| Step through the table | ratios, the octave |
| Step or switch (`lst`) | `eng`, the algorithm, `gat`, the fold type, Plaits' bank, model and output, mdk's type and machine, LFO B's sync and retrigger, key and scale, the sub's shape and octave |
| Don't morph | randomise, the mod slots, the Turing Machines |

- **`lst`** (performance page, after shuffle): `stp` steps list settings through every value in between, `swt`
  switches them once at 50 %. It is saved with the kit (on the Eurorack it is a global setting).
- The knobs keep showing the current kit; the sound moves.
- **Saving a morph** (save menu, morph type) saves the blend of UMA's settings too.

---

## Kits and your projects

- Every kit gets a block of UMA settings (1 KB) **appended** to its kit file. The stock part of the file stays as it
  is, so a project saved by LX_UMA still loads on Erica's firmware, as stock sounds.
- Kits saved by Erica's firmware load with every drum on `lxr` and the FM settings at their starting values.
- **`UMAKITS.DAT`** at the top of the card is LX_UMA's working copy of the project's UMA settings. It is rebuilt
  when a project loads; leave it on the card.
- A pattern that loads its kit (pattern change mode) gets the kit's UMA settings read from the card ahead of time,
  while the pattern before it plays.
- The **morph kit's** UMA settings are read from the card when you load a morph kit (and after a project loads).
- UMA settings are not sent or received as MIDI CC and can't be automated by the sequencer yet (a later release).

## CPU

The global page (SHIFT + step 12, pressed again to step through its screens) shows `pk` in the 4th place of its
2nd screen (`ptm tin tou pk`): the highest load of the last 2 s, in % of the audio's time. Above 100 the audio drops
out. A busy stock pattern runs at about 67 %. Each engine voice adds to it while it sounds (one FM or Plaits drum voice
about 15–18 %, the heaviest Plaits models 26–28 %); a voice whose sound has died away costs little or nothing.
With several heavy voices playing at once, check `pk` on your busiest pattern.

## Not in 0.3 yet

Coming with the next releases: DX7 banks, wavetables and terrains from the card (Phase 4); the sequencer's
automation of UMA settings (UMA lanes). Not on the desktop: the prk engine (its memory is the sequencer's), the FM
operator waveforms, and the Eurorack's CV, accent and MIDI CC sources (no jacks; the desktop isn't meant to be played
from a MIDI controller).
