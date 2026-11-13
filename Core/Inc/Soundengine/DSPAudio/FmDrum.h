/*
 * FmDrum.h - UMA FM Drum engine (docs/design/fm-drum-engine.md).
 *
 * Plain C with no hardware code, so the same files build into the firmware and into the
 * Mac test app (tools/fmtest). Settings use the firmware's 0-127 parameter range.
 *
 * The engine is the voice's sound generator: three operators. C is the carrier and plays at the
 * tuned pitch; A and B have no pitch of their own and run at C's frequency times their ratio.
 *
 * Step 2e: the 7 FM DRUM algorithms with feedback on the operator(s) each algorithm puts it on.
 * Operators A and B with ratios, each with an
 * operator envelope: the modulation rises from 0 to MOD over the attack time, then decays to
 * END x MOD. In algorithms 5-7 B is also heard, and MOD B and envelope B set its level. A pitch sweep, the same as the LXR drum voice's pitch envelope (Decay.c), moves all
 * three operators together, updated once per 64-sample block as on the module. The DECAY envelope
 * stands in for the voice's amp envelope.
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef FMDRUM_H_
#define FMDRUM_H_

#include <stdint.h>

#define FMD_NUM_RATIOS	51	// entries of the ratio table (FmDrum.c)

enum FmDrumParam
{
	FMD_ALGO,		// 0-6: algorithm 1-7
	FMD_FEEDBACK,	// 0-127: self-modulation of the algorithm's feedback operator(s)
	FMD_TUNE,		// MIDI note 0-127: the pitch of C
	FMD_SWEEP_DEC,	// 0-127: pitch sweep time, as the LXR's pitch decay (127 = never ends)
	FMD_SWEEP_AMT,	// 0-127: pitch sweep depth, frequency x (1 + amount) at the start, amount = (v/127)^2 x 32
	FMD_SWEEP_SLOPE,// 0-127: sweep curve, 64 = linear
	FMD_RATIO_A,	// 0 .. FMD_NUM_RATIOS-1: index into the ratio table
	FMD_MOD_A,		// 0-127: how much A modulates at the envelope's peak
	FMD_ATK_A,		// 0-127: operator envelope A attack, 0 = instant, else about 0.5 ms to 2 s
	FMD_DEC_A,		// 0-127: operator envelope A decay, about 5 ms to 5 s to -60 dB
	FMD_END_A,		// 0-127: level operator envelope A decays to, as a share of MOD A
	FMD_RATIO_B,
	FMD_MOD_B,
	FMD_ATK_B,
	FMD_DEC_B,
	FMD_END_B,
	FMD_DECAY,		// 0-127: about 5 ms to 5 s to -60 dB (stand-in for the amp envelope)
	FMD_LEVEL,		// 0-127
	FMD_FINE_A,		// 0-127: offset on ratio A, 63 = exact, 0 / 126 = halfway to the table entry below / above
	FMD_FINE_B,
	FMD_FOLD,		// 0-127: wavefolder on the body (C), 0 = off; the drive follows the body envelope (fmDrum_setFoldEnv)
	FMD_FOLD_TYPE,	// 0 sine (smooth), 1 triangle (sharper)
	FMD_FOLD_SYM,	// 0-127: offset before the folder, 63 = symmetric
	FMD_BODY_LEVEL,	// 0-127: the body's level after the folder, 100 = 1 (B's direct output is not affected)
	FMD_WAVE_C,		// 0-127: C's waveform, a sweep sine - triangle - saw - square - pulse - bell (FmWaves.h); 0 = sine
	FMD_WAVE_AB,	// 0-127: A's and B's waveform, the same sweep (B2's too in tone mode)
	// FM Tone (docs/design/fm-tone.md): four operators C, A, B1, B2 in the 8 FM TONE algorithms; B1 is B above
	FMD_TONE,		// 0 FM Drum, 1 FM Tone (FMD_ALGO then 0-7: tone algorithm 1-8)
	FMD_RATIO_C,	// tone: C's ratio, a FMD_RATIO_* table index (6 = x1)
	FMD_RATIO_B2,	// tone: B2's ratio
	FMD_MOD_B2,		// tone: B2's depth (or level where it's heard), on its own envelope (FMD_ATK_B2 ...)
	FMD_DETUNE,		// tone: 0-127, A and B2 detuned up against C and B1 (fmTone_detuneCents)
	FMD_MIX,		// tone: 0-127, crossfade of the outputs X and Y (equal power), 0 = X only
	FMD_SUB_LEVEL,	// tone: 0-127, the sub oscillator's level, 0 = off
	FMD_SUB_SHAPE,	// tone: 0 sine, 1 triangle, 2 saw, 3 square
	FMD_SUB_OCT,	// tone: 0-3, one to four octaves below the played pitch
	FMD_SUB_FOLD,	// tone: 0-127, a sine wavefolder on the sub alone, 0 = off; low values a light saturation
	FMD_ATK_B2,		// tone: B2's own operator envelope (attack, decay, end), as A's and B1's
	FMD_DEC_B2,
	FMD_END_B2,
	FMD_NUM_PARAMS
};

// an operator's waveform for the current block: two neighbouring shapes at the operator's band limit, crossfaded
typedef struct
{
	const int16_t* t0;
	const int16_t* t1;
	float mix;		// 0 = t0, 1 = t1
	uint8_t sine;	// the plain sine table (the setting at 0)
} FmDrumWave;

typedef struct
{
	uint32_t phase;
	uint32_t phaseInc;
	FmDrumWave wave;
} FmDrumOp;

// attack: rises linearly from 0 to 1; then decays towards an end level:
// value = end + (1 - end) * x, with x falling exponentially from 1
typedef struct
{
	float att;		// attack ramp position 0..1
	float attInc;	// per sample; >= 1 means no attack
	uint8_t inAttack;
	float x;
	float coef;
	float end;
} FmDrumOpEnv;

typedef struct
{
	float sampleRate;
	uint8_t params[FMD_NUM_PARAMS];

	FmDrumOp opC, opA, opB;
	float freqC;	// C's pitch in Hz: from TUNE, or set from outside with fmDrum_setPitch
	float baseIncC, baseIncA, baseIncB;	// phase increments without the sweep

	// pitch sweep (the LXR pitch envelope): value falls linearly by sweepDec per block, then is curved
	float sweep;
	float sweepDec;
	float sweepSlope;
	float sweepAmount;
	uint32_t ctrlCount;	// samples left until the next control-rate (block) update
	float modA, modB;	// modulation depth in cycles of the target's phase per unit of modulator output
	float levelB;		// B's output level in algorithms 5-7
	float feedback;		// feedback depth in cycles per unit of the averaged output
	float fbA1, fbA2, fbB1, fbB2;	// the last two enveloped outputs of A and B
	FmDrumOpEnv envA, envB;
	FmDrumOpEnv envB2;	// FM Tone

	// wavefolder (docs/design/wavefolder.md): first-order antiderivative anti-aliasing
	float foldEnv;		// the body envelope the drive follows, 0..1 (fmDrum_setFoldEnv; the own envelope in the test app)
	float foldU1;		// the folder's last input
	float foldDrive1;	// the drive at the end of the last chunk: it glides to the new one, never steps
	uint8_t foldOn;		// folded the last chunk too (else the folder starts from the current input)
	float dcX1, dcY1;	// DC blocker after an asymmetric fold

	// FM Tone
	FmDrumOp opB2;
	float baseIncB2;
	float modB2;		// B2's depth, as modB
	float levelA, levelB2;	// A's and B2's output level where an algorithm hears them, as levelB
	float mixX, mixY;	// the crossfade's gains
	float fbT1, fbT2;	// the last two enveloped outputs of the tone algorithm's feedback operator
	uint32_t subPhase, subInc;
	float subDrive1, subWet1;	// the sub folder's drive and mix at the end of the last chunk: they glide, never step

	uint8_t ampEnvOn;	// 0 on the module, where the voice's amp envelope shapes the volume
	float env;		// current envelope level, 0..1
	float envCoef;	// per-sample decay multiplier
	float level;
	float velocity;
} FmDrum;

void fmDrum_init(FmDrum* d, float sampleRate);
void fmDrum_setParam(FmDrum* d, uint8_t param, uint8_t value);
uint8_t fmDrum_getParam(const FmDrum* d, uint8_t param);
void fmDrum_trigger(FmDrum* d, float velocity);
void fmDrum_render(FmDrum* d, float* out, uint32_t numSamples);

// On the module: the voice sets C's pitch every block (tune, MIDI note, 1V/oct CV, LFO and its own
// pitch envelope), and its amp envelope shapes the volume, so the engine's stand-in is switched off.
void fmDrum_setPitch(FmDrum* d, float freqC);
void fmDrum_setAmpEnv(FmDrum* d, uint8_t on);

// helpers shared with the test app's display
float fmDrum_noteToFreq(float note);
float fmDrum_decayTime(uint8_t value);
float fmDrum_attackTime(uint8_t value);	// 0 for value 0 (instant)
float fmDrum_sweepAmount(uint8_t value);	// pitch multiplier at the start of the sweep is 1 + this
float fmDrum_sweepTime(uint8_t value, float sampleRate);	// seconds; 0 if it never ends

#define FMD_BLOCK_SIZE 64	// control rate: the module's audio block
float fmDrum_ratio(uint8_t value);		// ratio for a FMD_RATIO_* value
float fmDrum_ratioFine(uint8_t value, uint8_t fine);	// ... moved by a FMD_FINE_* value
const char* fmDrum_algoName(uint8_t algo);	// routing as text, e.g. "B -> A -> C"
const char* fmDrum_algoFeedback(uint8_t algo);	// feedback operator(s) as text: "A", "B" or "A+B"
float fmDrum_feedbackIndex(uint8_t value);	// feedback in radians
#define FMD_NUM_ALGOS 7
#define FMT_NUM_ALGOS 8	// FM Tone
#define FMT_NUM_SUB_SHAPES 4
#define FMT_NUM_SUB_OCTS 4	// -1 to -4
float fmTone_subFold(float x, uint8_t value);	// the sub's folder, for tests and the test app
// FM Tone's sub oscillator for other engines (the Plaits wavetable and terrain, 0.34e, docs/design/plaits-sub.md):
// its own state; adds the sub into out (level 0-127 squared, x gain), its folder gliding over the block as on fmt
typedef struct { uint32_t phase; float drive, wet; } FmToneSub;
void fmTone_subReset(FmToneSub* s);
void fmTone_subAdd(FmToneSub* s, float* out, uint32_t numSamples, float cyclesPerSample, uint8_t level, uint8_t shape,
	uint8_t fold, float gain);
const char* fmTone_algoName(uint8_t algo);	// FM Tone routing as text, e.g. "A>C B2>B1>C  X C  Y B1"
const char* fmTone_algoFeedback(uint8_t algo);	// its feedback operator: "A", "B1" or "B2"
float fmTone_detuneCents(uint8_t value);	// FMD_DETUNE: how far A and B2 go up, in cents
float fmDrum_modIndex(uint8_t value);	// modulation index in radians for a FMD_MOD_* value

// --- randomise (docs/design/fm-drum-engine.md section 5) -------------------------------------------
// The random values for one group are a hash of (seed, group value, drum), so the same numbers always give
// the same sound. Shared by the test app and the firmware so both randomise identically.
enum FmDrumRandomGroup
{
	FMD_RND_TONE,	// algorithm, ratio A and B, mod A and B, feedback
	FMD_RND_ENV,	// operator envelopes A and B: attack, decay, end
	FMD_RND_SWEEP,	// sweep amount, decay, slope
	FMD_RND_FINE,	// fine ratio A and B (full range); with values 1-63 only for metallic ratios, see below
	FMD_NUM_RND_GROUPS
};
#define FMD_RND_MAX_VALUES 9	// FM Tone with a palette: tone and envelopes 9 each

// the ratio palettes of the tone draw (rpl, UMA 0.28, docs/design/fm-tone.md section 5)
enum FmDrumRandomPalette
{
	FMD_PAL_ALL,	// any ratio (the FM Drum's draw): gritty drums
	FMD_PAL_BAS,	// ratios that keep the root: whole numbers, metallic from x2.82 up
	FMD_PAL_HRM,	// whole numbers only
	FMD_PAL_MET,	// metallic only
	FMD_NUM_PALETTES
};

typedef struct
{
	uint8_t param;	// FMD_* parameter
	uint8_t value;	// 0-127, as for fmDrum_setParam
} FmDrumRandomValue;

// fills out[] with the group's new values and returns how many; value 1-127 (0 = restore, not random).
// length: the sound's length in seconds (the voice's amp decay), or 0 if unknown. The envelope group then
// draws decays as a share of it (mostly 30-100 %, sometimes up to 150 %) and keeps attacks under a third.
uint8_t fmDrum_random(uint8_t group, uint8_t seed, uint8_t value, uint8_t drum, float length,
	FmDrumRandomValue out[FMD_RND_MAX_VALUES]);
// ... with a palette (FMD_PAL_*) and the engine (tone 1: FM Tone). FMD_PAL_ALL is fmDrum_random on both
// engines. Other palettes: the tone group draws the ratios by palette, from a strict list for the operators
// that modulate C or are heard; on FM Tone the tone group also draws algorithm 1-8, B2's ratio and depth and
// (hrm, met: not bas) C's ratio, and every group bass ranges (envelopes of A, B1 and B2)
uint8_t fmDrum_randomPal(uint8_t group, uint8_t seed, uint8_t value, uint8_t drum, float length,
	uint8_t palette, uint8_t tone, FmDrumRandomValue out[FMD_RND_MAX_VALUES]);

// the envelope the wavefolder's drive follows (the module: the voice's amp EG, or the LPG's vactrol), 0..1
void fmDrum_setFoldEnv(FmDrum* d, float env);
// one sample of the folders, for tests: type 0 sine, 1 triangle, u the folder's input
float fmDrum_fold(uint8_t type, float u);
// the wavefolder and body level over a block of another engine's sound (prk), up to FMD_BLOCK_SIZE samples, with
// this drum's fold settings and fold envelope
void fmDrum_foldBlock(FmDrum* d, float* buf, uint32_t n);

// the random fine group: 1 if a ratio takes the random fine at this group value: 1-63 only metallic ratios
// (harmonic ones keep their own fine), 64-127 every ratio
uint8_t fmDrum_randomFineApplies(uint8_t value, uint8_t ratio);
// ... with a palette: bas and hrm only metallic ratios, whatever the value
uint8_t fmDrum_randomFineAppliesPal(uint8_t value, uint8_t ratio, uint8_t palette);
uint8_t fmDrum_ratioIsMetallic(uint8_t ratio);	// a FMD_RATIO_* table index: one of the inharmonic entries

#endif /* FMDRUM_H_ */
