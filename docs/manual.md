# LX_UMA for the LXR-02 desktop: user manual

Custom firmware for the Erica Synths LXR-02 drum machine, by the makers of LX_UMA for the LXR Eurorack module
([github.com/mauSuaza/LX_UMA](https://github.com/mauSuaza/LX_UMA)). It is not official Erica Synths firmware and
Erica Synths provides no support for it (see the README).

This manual covers release **0.1**: the FM engines on Drum 1–3. Everything else works as on Erica's firmware
v1.7; see the LXR-02 manual for it.

## Start here

### Installing

1. Back up your SD card (copy everything on it to your computer). LX_UMA keeps your projects, but adds to their kit
   files (below).
2. Copy `LXR_V2_UMA_V0.1.img` to the top folder of the card and delete any other `LXR*.img` there.
3. Put the card in the LXR, hold down the encoder button and switch the LXR on. Keep holding until "LXR Updater"
   appears. Wait for "Update OK!".
4. The start-up screen says **"LX_UMA v0.1"** and "base fw V 2.00".

To go back to Erica's firmware, install its update file the same way. Your projects still load there (the kits
play as stock sounds).

### What 0.1 adds

| | |
|---|---|
| **Drum 1–3** | An engine setting, `eng`, on the Osc page: `lxr` (the LXR's own oscillators, as before), `fmd` (FM Drum) or `fmt` (FM Tone) |
| **FM Drum** | Three operators after the Digitone II's FM DRUM: 7 algorithms, 51 ratios, operator envelopes, feedback, waveforms |
| **FM Tone** | Four operators after the Digitone's FM TONE: 8 algorithms, detune, X/Y mix, a sub oscillator |
| **Kits** | Every kit keeps its UMA settings (below) |
| **CPU readout** | `pk` on the global page (below) |

### Getting around

As on the stock firmware: a voice's select buttons choose its pages (Osc, Amp EG, Mod, FM, Transient, Filter, LFO,
Mix). Pressing the same select button again shows the page's 2nd screen. Some FM pages have **more screens**:
press the button again after the 2nd (they're listed below, screen 3 and 4). The pots edit the four settings on
screen.

With a drum on `lxr` its Osc page is `coa fin eng wav`, `pwm` on screen 2.

---

## fmd — FM Drum

`eng` = `fmd`, Drum 1–3. Three operators, after the Digitone II's FM DRUM.

### How it works

Three operators, after the Digitone II's FM DRUM: **C** is the carrier you hear; **A** and **B** modulate it
(in algorithms 5–7, B is also heard on its own). C plays at the drum's pitch (coarse, fine, the step's note, MIDI note); A and B run at C's pitch times their **ratio**, so the tone stays the same at every pitch. The
pitch sweep (Mod page) moves all three together. A and B each have an envelope that shapes how much they
modulate over the hit.

The FM sound is mixed with B's direct output (5–7) and the transient, and runs through the **filter**, the
**amp envelope**, drive and the mixer.

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
| Mod (select 3) | 1 | `dec` `slp` `mod` | Mod EG Decay / Slope / PitchMod Amount | The pitch sweep (moves all three operators) |
| | 2 | `vdt` `vam` `vvo` | Velocity Dest / Amount / Vol | As on the LXR: velocity modulation of one setting, and of the volume |
| FM (select 4) | 1 | `mod` | Op A Mod | How much A modulates at its envelope's peak |
| | 1 | `atk` `dec` `end` | Op A Attack / Decay / End lvl | A's envelope: rise (0 instant … 2 s), fall (5 ms–5 s), the level it falls to and holds (share of `mod`) |
| | 2 | `mod` `atk` `dec` `end` | Op B … | B's, the same (in algorithms 5–7, B's volume) |
| | 3 | `wvc` `wvm` | FM Wave C / AB | The carrier's and the modulators' waveform: 0 sine … 127 bell |
| Transient (select 5) | 1 | `wav` `vol` `frq` | Transnt Wave / Volume / Freq | The LXR's transient, layered on the hit |
| | 1 | `fdb` | FM Feedback | Self-modulation of the algorithm's feedback operator(s): brighter, gritty, then noise |
| Filter (select 6) | 1 | `frq` `res` `typ` `drv` | Filter … | As on the LXR; |
| Mix (select 8) | 1 | `vol` `pan` `srt` `drv` | Voice … | Level, pan, sample-rate reduction, drive, as on the LXR |

The waveforms sweep smoothly: 0 sine, ~25 triangle, ~51 saw, ~76 square, ~102 pulse, 127 bell (harmonics 1, 2,
3, 5, 8, 13). A non-sine **modulator** adds sidebands for each of its harmonics: dense and gritty, best low; a non-sine **carrier** colours the sound more gently. They're band-limited (no aliasing at high
notes) 


---

---

## fmt — FM Tone

`eng` = `fmt`. A melodic FM engine for **bass** (and drums too: the transient and the pitch sweep stay), after
the Digitone's FM TONE machine, built on the FM Drum engine: four operators, eight algorithms, and a **sub
oscillator**. Everything around it works as for `fmd`. The FM Drum settings (ratios A and B, mods, envelopes, feedback,
waveforms) are shared: switching `fmd` ↔ `fmt` keeps them.

### How it works

Four operators: **C** (the carrier), **A**, **B1** and **B2**. Each runs at its own **ratio** of the played
pitch: `rtc` for C (×1 plays the note; ×0.5 drops the whole tone an octave), `rta`, `rtb` (B1) and `rb2`. A, B1
and B2 each have their own depth and operator envelope (`mod atk dec end` on the FM page: A, B1, B2), so one
modulator can snap while another lingers. Each algorithm has two outputs, **X** and **Y**,
and `mix` crossfades them. An operator that is heard (on X or Y) plays at the level its depth setting gives it.

**Pitch:** C follows the drum's pitch every audio block: tune, the step's note, MIDI notes.

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
up to a hard fold at 127; smooth when modulated). The sub restarts with each hit and goes through the filter and the Amp EG like the rest. `sbo` `-3` and
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
| Mod (select 3) | 1 | `dec` `slp` `mod` | Mod EG Decay / Slope / PitchMod Amount | The pitch sweep |
| | 2 | `vdt` `vam` `vvo` | Velocity Dest / Amount / Vol | As on the LXR: velocity modulation of one setting, and of the volume |
| FM (select 4) | 1 | `mod` `atk` `dec` `end` | Op A … | A's depth and envelope |
| | 2 | `mod` `atk` `dec` `end` | Op B … | B1's |
| | 3 | `mod` `atk` `dec` `end` | Op B2 … | B2's: depth (where it modulates) or level (where it's heard), its own envelope |
| | 4 | `wvc` `wvm` | FM Wave C / AB | Waveforms (`wvm` also sets B2's) |
| | 4 | `fna` `fnb` | Op A / Op B Fine rt | Fine ratios |
| Transient (select 5) | 1 | `wav` `vol` `frq` | Transnt Wave / Volume / Freq | The LXR's transient |
| | 1 | `fdb` | FM Feedback | The algorithm's feedback |
| | 3 | `dtn` | FM Detune | A and B2 sharp against C and B1: 6 cents at 64, 50 at 127 |
| | 3 | `mix` | FM Mix X/Y | Crossfade of the algorithm's outputs: 0 = X, 127 = Y |
| Filter (select 6) | 1 | `frq` `res` `typ` `drv` | Filter … | As on the LXR; |
| Mix (select 8) | 1 | `vol` `pan` `srt` `drv` | Voice … | Level, pan, sample-rate reduction, drive, as on the LXR |


---

## Kits and your projects

- Every kit gets a block of UMA settings (1 KB) **appended** to its kit file. The stock part of the file stays as it
  is, so a project saved by LX_UMA still loads on Erica's firmware, as stock sounds.
- Kits saved by Erica's firmware load with every drum on `lxr` and the FM settings at their starting values.
- **`UMAKITS.DAT`** at the top of the card is LX_UMA's working copy of the project's UMA settings. It is rebuilt
  when a project loads; leave it on the card.
- A pattern that loads its kit (pattern change mode) gets the kit's UMA settings read from the card ahead of time,
  while the pattern before it plays.
- UMA settings are not sent or received as MIDI CC and can't be automated by the sequencer yet (a later release).

## CPU

The global page (SHIFT + step 12, pressed again to step through its screens) shows `pk` in the 4th place of its
2nd screen (`ptm tin tou pk`): the highest load of the last 2 s, in % of the audio's time. Above 100 the audio drops
out. A stock pattern runs at about 67 %; each FM voice adds a few %, and a voice whose amp envelope has died away
costs nothing. With three FM Tone voices playing at once, check `pk` on your busiest pattern.

## Not in 0.1 yet

Coming with the next releases, as on the Eurorack firmware: the hold and gate modes, the filter envelope, the
low-pass gate and the wavefolder; randomise; the mdk and Plaits engines on every voice and the quantizer; the mod
matrix, LFO B, Turing Machines, velocity and MIDI CC sources, kit morph; the sequencer's automation and MIDI for UMA
settings; DX7 banks, wavetables and terrains from the card.
