/*
 * FmDrum.c - UMA FM Drum engine (docs/design/fm-drum-engine.md).
 *
 * Step 2e: the 7 algorithms with feedback, operators A and B with ratios and operator envelopes (attack, decay, end),
 * and the LXR pitch sweep on all three operators.
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "Soundengine/DSPAudio/FmDrum.h"
#include "Soundengine/DSPAudio/FmWaves.h"
#include <math.h>

// on the module, the sine table goes in the fast data memory (DTCM)
#ifdef STM32F765xx
#include "Soundengine/config.h"
#define FMD_FAST_DATA INDTCMZ
#else
#define FMD_FAST_DATA
#endif

#define FMD_SINE_BITS	10
#define FMD_SINE_SIZE	(1 << FMD_SINE_BITS)
#define FMD_FRAC_BITS	(32 - FMD_SINE_BITS)

#define FMD_TWO_PI			6.283185307f
#define FMD_CYCLES_PER_RAD	0.1591549431f	// 1 / 2 pi
#define FMD_PHASE_MAX		4294967040.f	// largest float below 2^32
#define FMD_MAX_MOD_INDEX	8.f				// radians at mod 127
#define FMD_MAX_FB_INDEX	4.f				// radians at feedback 127: saw-like around 1.5, noise above about 2.5

// Ratios for A and B, 0.25 to 16: whole numbers, just intervals (thirds to sevenths, and the 9th, 11th and
// 13th an octave up) for harmonic and chordal sounds, and inharmonic ones (square roots, pi) for metallic sounds
// (listed in fmd_metallicRatios).
// The menu's umaRatioNames show the same list.
static const float fmd_ratios[FMD_NUM_RATIOS] =
{
	0.25f, 1.f / 3.f, 0.5f, 2.f / 3.f, 0.7071f, 0.75f,
	1.f, 1.2f, 1.25f, 4.f / 3.f, 1.41f, 1.5f,
	1.6f, 5.f / 3.f, 1.73f, 1.75f, 1.8f, 1.875f,
	2.f, 2.25f, 2.4142f, 2.5f, 8.f / 3.f, 2.82f,
	3.f, 3.14f, 10.f / 3.f, 3.46f, 3.5f, 4.f,
	4.24f, 4.5f, 4.7124f, 5.f, 5.19f, 5.6569f,
	6.f, 6.2832f, 6.92f, 7.f, 7.854f, 8.f,
	8.4853f, 9.f, 9.42f, 10.f, 11.f, 12.f,
	13.f, 14.f, 16.f,
};

// --- algorithms (docs/design/fm-drum-engine.md, read from the Digitone II's FM DRUM screens) ---
// C is the carrier. fbA / fbB: which operators get the feedback.
enum
{
	FMD_ROUTE_AC_BC,	// A -> C, B -> C
	FMD_ROUTE_BA_BC_AC,	// B -> A, B -> C, A -> C
	FMD_ROUTE_BAC,		// B -> A -> C
	FMD_ROUTE_AC_B_OUT,	// A -> C; B is heard
};

typedef struct
{
	uint8_t route;
	uint8_t fbA, fbB;
	const char* name;
} FmdAlgo;

static const FmdAlgo fmd_algos[FMD_NUM_ALGOS] =
{
	{FMD_ROUTE_AC_BC,    0, 1, "A -> C, B -> C"},
	{FMD_ROUTE_AC_BC,    1, 1, "A -> C, B -> C"},
	{FMD_ROUTE_BA_BC_AC, 0, 1, "B -> A, B -> C, A -> C"},
	{FMD_ROUTE_BAC,      0, 1, "B -> A -> C"},
	{FMD_ROUTE_AC_B_OUT, 0, 1, "A -> C, B heard"},
	{FMD_ROUTE_AC_B_OUT, 1, 0, "A -> C, B heard"},
	{FMD_ROUTE_AC_B_OUT, 1, 1, "A -> C, B heard"},
};

// --- FM Tone algorithms (docs/design/fm-tone.md, read from the Digitone's FM TONE screens) ---
// Each has two outputs X and Y (crossfaded by FMD_MIX) and feedback on one operator.
enum { FMT_FB_A, FMT_FB_B1, FMT_FB_B2 };

typedef struct
{
	uint8_t fb;
	const char* name;
	const char* fbName;
} FmtAlgo;

static const FmtAlgo fmt_algos[FMT_NUM_ALGOS] =
{
	{FMT_FB_A,  "A>C B2>B1>C  X C  Y B1", "A"},
	{FMT_FB_B2, "A>C B2>B1  X C  Y B1", "B2"},
	{FMT_FB_A,  "A>C,B1,B2  X C+B2  Y B1", "A"},
	{FMT_FB_B2, "B2>B1>A>C  X C  Y B1", "B2"},
	{FMT_FB_B1, "B1>B2 B1+B2>A>C  X C  Y A", "B1"},
	{FMT_FB_A,  "A+B2>C,B1  X C  Y B1", "A"},
	{FMT_FB_A,  "A>C B2>B1  X C+A  Y B1+B2", "A"},
	{FMT_FB_B1, "A>C  X C+B2  Y B1", "B1"},
};

const char* fmTone_algoName(uint8_t algo)
{
	return fmt_algos[algo < FMT_NUM_ALGOS ? algo : 0].name;
}

const char* fmTone_algoFeedback(uint8_t algo)
{
	return fmt_algos[algo < FMT_NUM_ALGOS ? algo : 0].fbName;
}

float fmTone_detuneCents(uint8_t value)
{
	// subtle below the middle (6 cents at 64), wide at the top (50 cents)
	const float v = (value > 127 ? 127 : value) / 127.f;
	return 50.f * v * v * v;
}

const char* fmDrum_algoName(uint8_t algo)
{
	return fmd_algos[algo < FMD_NUM_ALGOS ? algo : 0].name;
}

const char* fmDrum_algoFeedback(uint8_t algo)
{
	const FmdAlgo* a = &fmd_algos[algo < FMD_NUM_ALGOS ? algo : 0];
	return a->fbA && a->fbB ? "A+B" : a->fbA ? "A" : "B";
}

float fmDrum_feedbackIndex(uint8_t value)
{
	const float v = (value > 127 ? 127 : value) / 127.f;
	return v * v * FMD_MAX_FB_INDEX;
}

// one extra entry so the interpolation can read table[i+1] without wrapping
FMD_FAST_DATA static float fmd_sine[FMD_SINE_SIZE + 1];
static uint8_t fmd_sineReady;

static void fmd_initSine(void)
{
	if(fmd_sineReady)
		return;
	for(uint32_t i = 0; i <= FMD_SINE_SIZE; i++)
		fmd_sine[i] = sinf(FMD_TWO_PI * (float)i / FMD_SINE_SIZE);
	fmd_sineReady = 1;
}

static inline float fmd_sineLookup(uint32_t phase)
{
	const uint32_t i = phase >> FMD_FRAC_BITS;
	const float frac = (float)(phase & ((1u << FMD_FRAC_BITS) - 1)) * (1.f / (float)(1u << FMD_FRAC_BITS));
	const float a = fmd_sine[i];
	return a + (fmd_sine[i + 1] - a) * frac;
}

float fmDrum_noteToFreq(float note)
{
	return 440.f * powf(2.f, (note - 69.f) / 12.f);
}

float fmDrum_decayTime(uint8_t value)
{
	// exponential knob: 5 ms at 0, 5 s at 127
	return 0.005f * powf(1000.f, value / 127.f);
}

float fmDrum_attackTime(uint8_t value)
{
	// 0 = instant, then exponential: 0.5 ms at 1 to 2 s at 127
	if(value == 0)
		return 0.f;
	return 0.0005f * powf(4000.f, (value - 1) / 126.f);
}

// --- pitch sweep: the LXR drum voice's pitch envelope (Decay.c, PITCH_AMOUNT_FACTOR 32) ---
#define FMD_SWEEP_TIME_K	(2.f * 0.99f / (1.f - 0.99f))

// amount the sweep value falls per block (once per FMD_BLOCK_SIZE samples), as DecayEg_calcTime
static float fmd_sweepDec(uint8_t value)
{
	const float v = value / 127.f;
	const float ret = (1.f - ((1.f + FMD_SWEEP_TIME_K) * v / (1.f + FMD_SWEEP_TIME_K * fabsf(v)))) * 2.f;
	return ret > 1.f ? 1.f : ret;
}

// curve, as DecayEg_setSlope; limited inside +-1, where the LXR formula goes flat (0) or divides by 0 (127)
static float fmd_sweepSlope(uint8_t value)
{
	float a = ((value / 127.f) - 0.5f) * 2.f;
	if(a > 0.99f) a = 0.99f;
	if(a < -0.99f) a = -0.99f;
	return 2.f * a / (1.f - a);
}

static inline float fmd_curve(float v, float slope)
{
	return (1.f + slope) * v / (1.f + slope * fabsf(v));
}

float fmDrum_sweepAmount(uint8_t value)
{
	const float v = value / 127.f;
	return v * v * 32.f;
}

float fmDrum_sweepTime(uint8_t value, float sampleRate)
{
	const float dec = fmd_sweepDec(value);
	return dec > 0.f ? (1.f / dec) * FMD_BLOCK_SIZE / sampleRate : 0.f;
}

float fmDrum_ratio(uint8_t value)
{
	return fmd_ratios[value < FMD_NUM_RATIOS ? value : FMD_NUM_RATIOS - 1];
}

float fmDrum_ratioFine(uint8_t value, uint8_t fine)
{
	const uint8_t i = value < FMD_NUM_RATIOS ? value : FMD_NUM_RATIOS - 1;
	const float r = fmd_ratios[i];
	float f = ((int)fine - 63) / 63.f;	// -1..1, as the drum's fine tune (63 shows as 0)
	if(f > 1.f) f = 1.f;
	// up to halfway to the neighbouring entry; at the table's ends, the gap on the other side
	const float gap = f > 0.f ? (i < FMD_NUM_RATIOS - 1 ? fmd_ratios[i + 1] - r : r - fmd_ratios[FMD_NUM_RATIOS - 2])
	                          : (i > 0 ? r - fmd_ratios[i - 1] : fmd_ratios[1] - r);
	return r + f * 0.5f * gap;
}

float fmDrum_modIndex(uint8_t value)
{
	// squared, so low settings give fine control
	const float v = (value > 127 ? 127 : value) / 127.f;
	return v * v * FMD_MAX_MOD_INDEX;
}

// C plays at the tuned pitch; A and B follow at their ratios. FM Tone: every operator at its own ratio of the
// played pitch (C's too), A and B2 detuned; the sub one or two octaves below the played pitch
static void fmd_updatePitch(FmDrum* d)
{
	const float inc = d->freqC / d->sampleRate;	// cycles per sample
	if(d->params[FMD_TONE])
	{
		const float detune = powf(2.f, fmTone_detuneCents(d->params[FMD_DETUNE]) * (1.f / 1200.f));
		d->baseIncC = inc * fmDrum_ratio(d->params[FMD_RATIO_C]);
		d->baseIncA = inc * fmDrum_ratioFine(d->params[FMD_RATIO_A], d->params[FMD_FINE_A]) * detune;
		d->baseIncB = inc * fmDrum_ratioFine(d->params[FMD_RATIO_B], d->params[FMD_FINE_B]);
		d->baseIncB2 = inc * fmDrum_ratio(d->params[FMD_RATIO_B2]) * detune;
		return;
	}
	d->baseIncC = inc;
	d->baseIncA = inc * fmDrum_ratioFine(d->params[FMD_RATIO_A], d->params[FMD_FINE_A]);
	d->baseIncB = inc * fmDrum_ratioFine(d->params[FMD_RATIO_B], d->params[FMD_FINE_B]);
}

// cycles per sample to a phase increment, kept below Nyquist so a deep sweep can't overflow
static inline uint32_t fmd_cyclesToInc(float cycles)
{
	if(cycles > 0.499f) cycles = 0.499f;
	return (uint32_t)(cycles * 4294967296.f);
}

// once per block: advance the sweep and set all three operators' pitch from it
// the waveform an operator plays this block: the sweep position (0 = sine) at the band limit its frequency allows
static const uint16_t fmd_waveHarmonics[FMW_LEVELS] = {511, 256, 128, 64, 32, 16, 8, 4, 2, 1};
static void fmd_setWave(FmDrumWave* w, uint8_t value, uint32_t phaseInc)
{
#ifdef FMD_NO_WAVES
	// LX_UMA desktop: no operator waveforms (the user's choice, 2026-11-12): sines only, and FmWaves.c's 120 KB of
	// tables aren't linked
	(void)value; (void)phaseInc;
	w->sine = 1;
	return;
#endif
	if(value == 0)
	{
		w->sine = 1;
		return;
	}
	w->sine = 0;
	// harmonics below Nyquist (with a margin): 0.45 / (cycles per sample)
	const float cycles = (float)phaseInc * (1.f / 4294967296.f);
	const float allowed = cycles > 0.f ? 0.45f / cycles : 1e9f;
	uint8_t level = 0;
	while(level < FMW_LEVELS - 1 && fmd_waveHarmonics[level] > allowed)
		level++;
	const float x = (value > 127 ? 127 : value) * ((FMW_SHAPES - 1) / 127.f);
	uint8_t s = (uint8_t)x;
	if(s > FMW_SHAPES - 2) s = FMW_SHAPES - 2;
	w->mix = x - s;
	w->t0 = fmw_tables[s][level];
	w->t1 = fmw_tables[s + 1][level];
}

static inline float fmd_waveLookup(const FmDrumWave* w, uint32_t phase)
{
	if(w->sine)
		return fmd_sineLookup(phase);
	const uint32_t i = phase >> (32 - FMW_BITS);
	const float frac = (float)(phase & ((1u << (32 - FMW_BITS)) - 1)) * (1.f / (float)(1u << (32 - FMW_BITS)));
	const float a0 = w->t0[i], a1 = w->t1[i];
	const float v0 = a0 + (w->t0[i + 1] - a0) * frac;
	const float v1 = a1 + (w->t1[i + 1] - a1) * frac;
	return (v0 + (v1 - v0) * w->mix) * (1.f / 32767.f);
}

static void fmd_controlUpdate(FmDrum* d)
{
	float v = d->sweep - d->sweepDec;	// as DecayEg_calc: step first, then use
	if(v < 0.f) v = 0.f;
	d->sweep = v;
	const float pitch = 1.f + fmd_curve(v, d->sweepSlope) * d->sweepAmount;
	d->opC.phaseInc = fmd_cyclesToInc(d->baseIncC * pitch);
	d->opA.phaseInc = fmd_cyclesToInc(d->baseIncA * pitch);
	d->opB.phaseInc = fmd_cyclesToInc(d->baseIncB * pitch);
	fmd_setWave(&d->opC.wave, d->params[FMD_WAVE_C], d->opC.phaseInc);
	fmd_setWave(&d->opA.wave, d->params[FMD_WAVE_AB], d->opA.phaseInc);
	fmd_setWave(&d->opB.wave, d->params[FMD_WAVE_AB], d->opB.phaseInc);
	if(d->params[FMD_TONE])
	{
		d->opB2.phaseInc = fmd_cyclesToInc(d->baseIncB2 * pitch);
		fmd_setWave(&d->opB2.wave, d->params[FMD_WAVE_AB], d->opB2.phaseInc);
		const float sub = d->freqC / d->sampleRate / (float)(2u << d->params[FMD_SUB_OCT]);	// / 2, 4, 8, 16
		d->subInc = fmd_cyclesToInc(sub * pitch);
	}
}

// per-sample multiplier that reaches -60 dB (x 0.001) after the decay time
static float fmd_decayCoef(const FmDrum* d, uint8_t value)
{
	return expf(logf(0.001f) / (fmDrum_decayTime(value) * d->sampleRate));
}

static float fmd_attackInc(const FmDrum* d, uint8_t value)
{
	const float t = fmDrum_attackTime(value);
	return t > 0.f ? 1.f / (t * d->sampleRate) : 1.f;
}

static void fmd_envTrigger(FmDrumOpEnv* e)
{
	e->x = 1.f;
	e->att = 0.f;
	e->inAttack = e->attInc < 1.f;
}

// one sample of an operator envelope
static inline float fmd_envNext(FmDrumOpEnv* e)
{
	if(e->inAttack)
	{
		e->att += e->attInc;
		if(e->att >= 1.f)
		{
			e->att = 1.f;
			e->inAttack = 0;
		}
		return e->att;
	}
	const float v = e->end + (1.f - e->end) * e->x;
	e->x *= e->coef;
	return v;
}

static void fmd_update(FmDrum* d, uint8_t param)
{
	switch(param)
	{
	case FMD_TUNE:
		d->freqC = fmDrum_noteToFreq(d->params[FMD_TUNE]);
		fmd_updatePitch(d);
		break;
	case FMD_TONE:
	case FMD_RATIO_C:
	case FMD_RATIO_B2:
	case FMD_DETUNE:
	case FMD_RATIO_A:
	case FMD_RATIO_B:
	case FMD_FINE_A:
	case FMD_FINE_B:
		fmd_updatePitch(d);
		break;
	case FMD_SWEEP_DEC:
		d->sweepDec = fmd_sweepDec(d->params[FMD_SWEEP_DEC]);
		break;
	case FMD_SWEEP_AMT:
		d->sweepAmount = fmDrum_sweepAmount(d->params[FMD_SWEEP_AMT]);
		break;
	case FMD_SWEEP_SLOPE:
		d->sweepSlope = fmd_sweepSlope(d->params[FMD_SWEEP_SLOPE]);
		break;
	case FMD_MOD_A:
	{
		d->modA = fmDrum_modIndex(d->params[FMD_MOD_A]) * FMD_CYCLES_PER_RAD;
		const float l = d->params[FMD_MOD_A] / 127.f;
		d->levelA = l * l;
		break;
	}
	case FMD_MOD_B2:
	{
		d->modB2 = fmDrum_modIndex(d->params[FMD_MOD_B2]) * FMD_CYCLES_PER_RAD;
		const float l = d->params[FMD_MOD_B2] / 127.f;
		d->levelB2 = l * l;
		break;
	}
	case FMD_MIX:
	{
		const float m = d->params[FMD_MIX] / 127.f * (FMD_TWO_PI * 0.25f);	// equal power: cos and sin
		d->mixX = cosf(m);
		d->mixY = sinf(m);
		if(d->params[FMD_MIX] == 0) { d->mixX = 1.f; d->mixY = 0.f; }
		if(d->params[FMD_MIX] == 127) { d->mixX = 0.f; d->mixY = 1.f; }
		break;
	}
	case FMD_MOD_B:
	{
		d->modB = fmDrum_modIndex(d->params[FMD_MOD_B]) * FMD_CYCLES_PER_RAD;
		const float l = d->params[FMD_MOD_B] / 127.f;
		d->levelB = l * l;
		break;
	}
	case FMD_ATK_A:
		d->envA.attInc = fmd_attackInc(d, d->params[FMD_ATK_A]);
		break;
	case FMD_ATK_B:
		d->envB.attInc = fmd_attackInc(d, d->params[FMD_ATK_B]);
		break;
	case FMD_ATK_B2:
		d->envB2.attInc = fmd_attackInc(d, d->params[FMD_ATK_B2]);
		break;
	case FMD_DEC_B2:
		d->envB2.coef = fmd_decayCoef(d, d->params[FMD_DEC_B2]);
		break;
	case FMD_END_B2:
		d->envB2.end = d->params[FMD_END_B2] / 127.f;
		break;
	case FMD_FEEDBACK:
		d->feedback = fmDrum_feedbackIndex(d->params[FMD_FEEDBACK]) * FMD_CYCLES_PER_RAD;
		break;
	case FMD_DEC_A:
		d->envA.coef = fmd_decayCoef(d, d->params[FMD_DEC_A]);
		break;
	case FMD_END_A:
		d->envA.end = d->params[FMD_END_A] / 127.f;
		break;
	case FMD_DEC_B:
		d->envB.coef = fmd_decayCoef(d, d->params[FMD_DEC_B]);
		break;
	case FMD_END_B:
		d->envB.end = d->params[FMD_END_B] / 127.f;
		break;
	case FMD_DECAY:
		d->envCoef = d->ampEnvOn ? fmd_decayCoef(d, d->params[FMD_DECAY]) : 1.f;
		break;
	case FMD_LEVEL:
	{
		const float l = d->params[FMD_LEVEL] / 127.f;
		d->level = l * l;
		break;
	}
	default:
		break;
	}
}

void fmDrum_init(FmDrum* d, float sampleRate)
{
	fmd_initSine();
	d->sampleRate = sampleRate;
	d->ampEnvOn = 1;
	d->opC.phase = d->opA.phase = d->opB.phase = 0;
	d->env = 0.f;
	d->envA.x = d->envB.x = 0.f;
	d->envA.inAttack = d->envB.inAttack = 0;
	d->sweep = 0.f;
	d->ctrlCount = 0;
	d->fbA1 = d->fbA2 = d->fbB1 = d->fbB2 = 0.f;
	d->velocity = 1.f;

	d->params[FMD_ALGO] = 0;		// algorithm 1
	d->params[FMD_FEEDBACK] = 0;
	d->params[FMD_TUNE] = 45;		// A2, 110 Hz
	d->params[FMD_SWEEP_DEC] = 50;
	d->params[FMD_SWEEP_AMT] = 0;	// off
	d->params[FMD_SWEEP_SLOPE] = 64;	// linear
	d->params[FMD_RATIO_A] = 6;		// ratio 1
	d->params[FMD_MOD_A] = 0;
	d->params[FMD_ATK_A] = 0;
	d->params[FMD_DEC_A] = 50;
	d->params[FMD_END_A] = 0;
	d->params[FMD_RATIO_B] = 18;	// ratio 2
	d->params[FMD_MOD_B] = 0;
	d->params[FMD_ATK_B] = 0;
	d->params[FMD_DEC_B] = 40;
	d->params[FMD_END_B] = 0;
	d->params[FMD_DECAY] = 70;
	d->params[FMD_LEVEL] = 100;
	d->params[FMD_FINE_A] = 63;	// exact ratios
	d->params[FMD_FINE_B] = 63;
	d->params[FMD_FOLD] = 0;	// off
	d->params[FMD_FOLD_TYPE] = 0;
	d->params[FMD_FOLD_SYM] = 63;
	d->params[FMD_BODY_LEVEL] = 100;
	d->params[FMD_WAVE_C] = 0;	// sines
	d->params[FMD_WAVE_AB] = 0;
	d->params[FMD_TONE] = 0;	// FM Drum
	d->params[FMD_RATIO_C] = 6;	// x1
	d->params[FMD_RATIO_B2] = 6;
	d->params[FMD_MOD_B2] = 0;
	d->params[FMD_DETUNE] = 0;
	d->params[FMD_MIX] = 0;	// X only
	d->params[FMD_SUB_LEVEL] = 0;	// off
	d->params[FMD_SUB_SHAPE] = 0;	// sine
	d->params[FMD_SUB_OCT] = 0;	// -1
	d->params[FMD_SUB_FOLD] = 0;	// off
	d->params[FMD_ATK_B2] = 0;
	d->params[FMD_DEC_B2] = 40;
	d->params[FMD_END_B2] = 0;
	d->envB2.x = 0.f;
	d->envB2.inAttack = 0;
	d->subDrive1 = 1.f; d->subWet1 = 0.f;
	d->opC.wave.sine = d->opA.wave.sine = d->opB.wave.sine = d->opB2.wave.sine = 1;
	d->opB2.phase = d->subPhase = 0;
	d->opB2.phaseInc = d->subInc = 0;
	d->fbT1 = d->fbT2 = 0.f;
	d->foldEnv = 1.f;
	d->foldU1 = 0.f;
	d->foldDrive1 = 1.f;
	d->foldOn = 0;
	d->dcX1 = d->dcY1 = 0.f;
	for(uint8_t p = 0; p < FMD_NUM_PARAMS; p++)
		fmd_update(d, p);
}

void fmDrum_setParam(FmDrum* d, uint8_t param, uint8_t value)
{
	if(param >= FMD_NUM_PARAMS)
		return;
	// the algorithm keeps 0-7 whichever mode (FM Drum plays 7 as 6), so the order settings arrive in doesn't matter
	const uint8_t max = param == FMD_ALGO ? FMT_NUM_ALGOS - 1 : param == FMD_TONE ? 1
		: param == FMD_SUB_SHAPE ? FMT_NUM_SUB_SHAPES - 1 : param == FMD_SUB_OCT ? FMT_NUM_SUB_OCTS - 1 : 127;
	d->params[param] = value > max ? max : value;
	fmd_update(d, param);
}

uint8_t fmDrum_getParam(const FmDrum* d, uint8_t param)
{
	return param < FMD_NUM_PARAMS ? d->params[param] : 0;
}

void fmDrum_setPitch(FmDrum* d, float freqC)
{
	d->freqC = freqC;
	fmd_updatePitch(d);
}

void fmDrum_setAmpEnv(FmDrum* d, uint8_t on)
{
	d->ampEnvOn = on;
	fmd_update(d, FMD_DECAY);
	if(!on)
		d->env = 1.f;
}

void fmDrum_trigger(FmDrum* d, float velocity)
{
	d->opC.phase = d->opA.phase = d->opB.phase = 0;
	d->env = 1.f;
	fmd_envTrigger(&d->envA);
	fmd_envTrigger(&d->envB);
	d->sweep = 1.f;
	d->ctrlCount = 0;	// start a new control block, so the sweep starts with the hit
	d->fbA1 = d->fbA2 = d->fbB1 = d->fbB2 = 0.f;
	d->opB2.phase = d->subPhase = 0;
	d->fbT1 = d->fbT2 = 0.f;
	fmd_envTrigger(&d->envB2);
	d->velocity = velocity;
}

// phase offset in cycles to a phase increment: the offset can be several cycles, so keep the fraction
// (floorf is one VRINTM on the M7)
static inline uint32_t fmd_pm(float cycles)
{
	return (uint32_t)((cycles - floorf(cycles)) * FMD_PHASE_MAX);
}

// renders up to one control block with constant pitch. One loop per routing, so the routing
// isn't decided per sample. Feedback: an operator's phase is pushed by the average of its last two
// enveloped outputs (the DX7 approach, which keeps it stable); fbA / fbB are 0 for operators
// without feedback in the current algorithm.
void fmDrum_setFoldEnv(FmDrum* d, float env)
{
	d->foldEnv = env < 0.f ? 0.f : env > 1.f ? 1.f : env;
}

// sin(pi/2 u) from the table: a quarter cycle per unit of u. Wrapped to one cycle first: a hard fold takes u
// well past +-2, where u * 2^30 no longer fits a 32-bit phase (it clipped there, and clicked)
static inline float fmd_sinHalfPi(float u)
{
	const float cycles = u * 0.25f;
	const float frac = cycles - floorf(cycles);	// 0..1
	return fmd_sineLookup((uint32_t)(frac * 4294967295.f));
}

// triangle with period 4: 0 at 0, 1 at 1, -1 at -1, straight between
static inline float fmd_tri(float u)
{
	const float p = (u + 1.f) * 0.25f;
	const float t = p - floorf(p);
	return 1.f - 4.f * fabsf(t - 0.5f);
}

// its antiderivative, periodic (each period integrates to 0)
static inline float fmd_triInt(float u)
{
	const float p = (u + 1.f) * 0.25f;
	const float t = p - floorf(p);
	return t < 0.5f ? 4.f * (2.f * t * t - t) : 4.f * (3.f * t - 2.f * t * t - 1.f);
}

float fmDrum_fold(uint8_t type, float u)
{
	return type ? fmd_tri(u) : fmd_sinHalfPi(u);
}

// the folder over one chunk of the body, in place. First-order antiderivative anti-aliasing: the output is
// (F(u) - F(u1)) / (u - u1), the folder averaged over the step from the last input
static void fmd_foldChunk(FmDrum* d, float* body, uint32_t n)
{
	const float f = d->params[FMD_FOLD] / 127.f;
	const float target = 1.f + 7.f * f * f * d->foldEnv;
	const float offset = (d->params[FMD_FOLD_SYM] - 63) / 63.f * 0.5f;
	const float level = d->params[FMD_BODY_LEVEL] / 100.f;
	const uint8_t tri = d->params[FMD_FOLD_TYPE] != 0;
	float u1 = d->foldU1, x1 = d->dcX1, y1 = d->dcY1;
	// the drive glides over the chunk (the envelope changes once per block; a step would click on a hard fold)
	float drive = d->foldOn ? d->foldDrive1 : target;
	const float driveStep = n ? (target - drive) / (float)n : 0.f;
	if(!d->foldOn)	// switched on: start from the current input, not a stale one
	{
		u1 = body[0] * drive + offset;
		x1 = y1 = 0.f;
	}
	for(uint32_t i = 0; i < n; i++)
	{
		drive += driveStep;
		const float u = body[i] * drive + offset;
		const float du = u - u1;
		float y;
		if(tri)
			y = fabsf(du) > 1e-3f ? (fmd_triInt(u) - fmd_triInt(u1)) / du : fmd_tri(0.5f * (u + u1));
		else
		{
			// F(u) = -2/pi cos(pi/2 u): (F(u) - F(u1)) / du = 4/pi sin(pi/4 (u + u1)) sin(pi/4 du) / du, exact for any step.
			// sin(pi/4 du) / du is even: taken from |du|, so a tiny negative step never lands just below a full table
			// cycle (float can't place it finely there, and the division magnified that into a one-sample spike)
			const float adu = fabsf(du);
			const float x = 0.78539816f * adu;	// pi/4 |du|
			const float s = x < 0.25f ? 1.f - x * x * (1.f / 6.f) + x * x * x * x * (1.f / 120.f)	// sin(x)/x, exact to 1e-7
			                          : fmd_sinHalfPi(0.5f * adu) / adu * (4.f / 3.14159265f);
			y = fmd_sinHalfPi(0.5f * (u + u1)) * s;
		}
		u1 = u;
		// DC blocker (about 10 Hz): an asymmetric fold leaves an offset
		const float hp = y - x1 + 0.9986f * y1;
		x1 = y; y1 = hp;
		body[i] = hp * level;
	}
	d->foldU1 = u1; d->dcX1 = x1; d->dcY1 = y1;
	d->foldDrive1 = target;
	d->foldOn = 1;
}

void fmDrum_foldBlock(FmDrum* d, float* buf, uint32_t n)
{
	if(d->params[FMD_FOLD])
		fmd_foldChunk(d, buf, n);
	else
	{
		d->foldOn = 0;
		if(d->params[FMD_BODY_LEVEL] != 100)
		{
			const float level = d->params[FMD_BODY_LEVEL] / 100.f;
			for(uint32_t i = 0; i < n; i++) buf[i] *= level;
		}
	}
}

static void fmd_renderChunk(FmDrum* d, float* out, uint32_t numSamples)
{
	uint32_t phC = d->opC.phase, phA = d->opA.phase, phB = d->opB.phase;
	const uint32_t incC = d->opC.phaseInc, incA = d->opA.phaseInc, incB = d->opB.phaseInc;
	const float modA = d->modA, modB = d->modB, levelB = d->levelB;
	float env = d->env;
	const float coef = d->envCoef;
	const float gain = d->level * d->velocity;
	const FmdAlgo* algo = &fmd_algos[d->params[FMD_ALGO] < FMD_NUM_ALGOS ? d->params[FMD_ALGO] : FMD_NUM_ALGOS - 1];
	const uint8_t route = algo->route;
	const float fbA = algo->fbA ? d->feedback * 0.5f : 0.f;	// x 0.5: average of the two last outputs
	const float fbB = algo->fbB ? d->feedback * 0.5f : 0.f;
	float a1 = d->fbA1, a2 = d->fbA2, b1 = d->fbB1, b2 = d->fbB2;
	// the wavefolder or a body level: the body is processed on its own (with 0 and 100, exactly as before)
	const uint8_t bodyOnly = d->params[FMD_FOLD] != 0 || d->params[FMD_BODY_LEVEL] != 100;
	float bDirect[FMD_BLOCK_SIZE];

	// B, with its feedback; then its enveloped output is stored for the next sample
#define FMD_OP_B \
		const float b = fmd_waveLookup(&d->opB.wave, phB + fmd_pm(fbB * (b1 + b2))); \
		b2 = b1; b1 = b * eB;
	// A, with its feedback plus an extra modulation input (0 when nothing modulates A)
#define FMD_OP_A(IN) \
		const float a = fmd_waveLookup(&d->opA.wave, phA + fmd_pm(fbA * (a1 + a2) + (IN))); \
		a2 = a1; a1 = a * eA;

#define FMD_LOOP(BODY) \
	for(uint32_t i = 0; i < numSamples; i++) \
	{ \
		const float eA = fmd_envNext(&d->envA); \
		const float eB = fmd_envNext(&d->envB); \
		BODY \
		phC += incC; phA += incA; phB += incB; \
		env *= coef; \
	}

	switch(route)
	{
	default:
	case FMD_ROUTE_AC_BC:
		FMD_LOOP(
			FMD_OP_B
			FMD_OP_A(0.f)
			out[i] = fmd_waveLookup(&d->opC.wave, phC + fmd_pm(a * modA * eA + b * modB * eB)) * env * gain;
		)
		break;
	case FMD_ROUTE_BA_BC_AC:
		FMD_LOOP(
			FMD_OP_B
			const float bm = b * modB * eB;
			FMD_OP_A(bm)
			out[i] = fmd_waveLookup(&d->opC.wave, phC + fmd_pm(a * modA * eA + bm)) * env * gain;
		)
		break;
	case FMD_ROUTE_BAC:
		FMD_LOOP(
			FMD_OP_B
			FMD_OP_A(b * modB * eB)
			out[i] = fmd_waveLookup(&d->opC.wave, phC + fmd_pm(a * modA * eA)) * env * gain;
		)
		break;
	case FMD_ROUTE_AC_B_OUT:
		if(bodyOnly)
		{
			// the body and B's direct output apart, so only the body is folded
			FMD_LOOP(
				FMD_OP_B
				FMD_OP_A(0.f)
				const float c = fmd_waveLookup(&d->opC.wave, phC + fmd_pm(a * modA * eA));
				out[i] = c * env * gain;
				bDirect[i] = b * levelB * eB * env * gain;
			)
		}
		else
		{
			FMD_LOOP(
				FMD_OP_B
				FMD_OP_A(0.f)
				const float c = fmd_waveLookup(&d->opC.wave, phC + fmd_pm(a * modA * eA));
				out[i] = (c + b * levelB * eB) * env * gain;
			)
		}
		break;
	}
#undef FMD_LOOP
#undef FMD_OP_A
#undef FMD_OP_B

	d->opC.phase = phC;
	d->opA.phase = phA;
	d->opB.phase = phB;
	d->env = env;
	d->fbA1 = a1; d->fbA2 = a2; d->fbB1 = b1; d->fbB2 = b2;

	if(bodyOnly)
	{
		if(d->params[FMD_FOLD])
			fmd_foldChunk(d, out, numSamples);
		else
		{
			d->foldOn = 0;
			const float level = d->params[FMD_BODY_LEVEL] / 100.f;
			for(uint32_t i = 0; i < numSamples; i++) out[i] *= level;
		}
		if(route == FMD_ROUTE_AC_B_OUT)
			for(uint32_t i = 0; i < numSamples; i++) out[i] += bDirect[i];
	}
}

// polyBLEP: the step's correction near a jump of a naive saw or square, t the phase 0..1, dt the phase step
static inline float fmt_blep(float t, float dt)
{
	if(t < dt)
	{
		t /= dt;
		return t + t - t * t - 1.f;
	}
	if(t > 1.f - dt)
	{
		t = (t - 1.f) / dt;
		return t * t + t + t + 1.f;
	}
	return 0.f;
}

// one sample of the sub oscillator, each shape starting at 0 or its edge at phase 0
static inline float fmt_sub(uint8_t shape, uint32_t phase, float dt)
{
	const float t = (float)phase * (1.f / 4294967296.f);
	switch(shape)
	{
	default:
	case 0:
		return fmd_sineLookup(phase);
	case 1:	// triangle rising from 0
	{
		float u = t + 0.25f;
		u -= floorf(u);
		return 1.f - 4.f * fabsf(u - 0.5f);
	}
	case 2:	// saw rising from 0: its jump at half a cycle
	{
		float u = t + 0.5f;
		u -= floorf(u);
		return 2.f * u - 1.f - fmt_blep(u, dt);
	}
	case 3:	// square
	{
		float u = t + 0.5f;
		u -= floorf(u);
		return (t < 0.5f ? 1.f : -1.f) + fmt_blep(t, dt) - fmt_blep(u, dt);
	}
	}
}

// the sub's folder: sin(pi/2 x drive), faded in over the first values so a little is only a little (a sine
// folder at drive 1 already bends a sine). Drive 1 to 5. The sub is low, so no anti-aliasing is needed
static void fmt_subFoldAmount(uint8_t value, float* drive, float* wet)
{
	const float v = (value > 127 ? 127 : value) / 127.f;
	*drive = 1.f + 4.f * v * v;
	*wet = value < 24 ? value / 24.f : 1.f;
}

static inline float fmt_subFold(float x, float drive, float wet)
{
	return x + (fmd_sinHalfPi(x * drive) - x) * wet;
}

void fmTone_subReset(FmToneSub* s)
{
	fmd_initSine();	// (built by the first fmDrum_init on the module; here in case no FM voice was set up)
	s->phase = 0;
	s->drive = 1.f;
	s->wet = 0.f;
}

void fmTone_subAdd(FmToneSub* s, float* out, uint32_t numSamples, float cyclesPerSample, uint8_t level, uint8_t shape,
	uint8_t fold, float gain)
{
	const float l = (level > 127 ? 127 : level) / 127.f;
	const float subLevel = l * l * gain;
	if(subLevel == 0.f || numSamples == 0) return;
	float driveTo, wetTo;
	fmt_subFoldAmount(fold, &driveTo, &wetTo);
	if(fold == 0) wetTo = 0.f;
	float drive = s->drive, wet = s->wet;
	const float driveStep = (driveTo - drive) / (float)numSamples, wetStep = (wetTo - wet) / (float)numSamples;
	const uint32_t inc = fmd_cyclesToInc(cyclesPerSample);
	const float dt = (float)inc * (1.f / 4294967296.f);
	uint32_t ph = s->phase;
	for(uint32_t i = 0; i < numSamples; i++)
	{
		drive += driveStep; wet += wetStep;
		out[i] += fmt_subFold(fmt_sub(shape, ph, dt), drive, wet) * subLevel;
		ph += inc;
	}
	s->phase = ph;
	s->drive = driveTo; s->wet = wetTo;
}

float fmTone_subFold(float x, uint8_t value)
{
	if(value == 0)
		return x;
	float drive, wet;
	fmt_subFoldAmount(value, &drive, &wet);
	return fmt_subFold(x, drive, wet);
}

// FM Tone: renders up to one control block, one loop per algorithm. X and Y crossfaded, then the wavefolder and
// body level on the whole FM sound, then the sub (clean low end)
static void fmt_renderChunk(FmDrum* d, float* out, uint32_t numSamples)
{
	uint32_t phC = d->opC.phase, phA = d->opA.phase, phB = d->opB.phase, phB2 = d->opB2.phase, phS = d->subPhase;
	const uint32_t incC = d->opC.phaseInc, incA = d->opA.phaseInc, incB = d->opB.phaseInc, incB2 = d->opB2.phaseInc;
	const uint32_t incS = d->subInc;
	const float mA = d->modA, mB = d->modB, mB2 = d->modB2;
	const float lA = d->levelA, lB = d->levelB, lB2 = d->levelB2;
	const float gx = d->mixX, gy = d->mixY;
	float env = d->env;
	const float coef = d->envCoef;
	const float gain = d->level * d->velocity;
	const uint8_t al = d->params[FMD_ALGO] < FMT_NUM_ALGOS ? d->params[FMD_ALGO] : 0;
	const float fb = d->feedback * 0.5f;	// x 0.5: average of the two last outputs
	float t1 = d->fbT1, t2 = d->fbT2;
	const float subL = d->params[FMD_SUB_LEVEL] / 127.f;
	const float subLevel = subL * subL;
	const uint8_t subShape = d->params[FMD_SUB_SHAPE];
	// the sub folder's amount glides over the chunk to its new value (modulated, it changes once per block)
	float subDrive, subWet, subDriveTo, subWetTo;
	fmt_subFoldAmount(d->params[FMD_SUB_FOLD], &subDriveTo, &subWetTo);
	if(d->params[FMD_SUB_FOLD] == 0) subWetTo = 0.f;
	subDrive = d->subDrive1; subWet = d->subWet1;
	const float subDriveStep = numSamples ? (subDriveTo - subDrive) / (float)numSamples : 0.f;
	const float subWetStep = numSamples ? (subWetTo - subWet) / (float)numSamples : 0.f;
	const float subDt = (float)incS * (1.f / 4294967296.f);
	const uint8_t bodyOnly = d->params[FMD_FOLD] != 0 || d->params[FMD_BODY_LEVEL] != 100;
	float subBuf[FMD_BLOCK_SIZE];

#define FMT_OP(OP, PH, PM) fmd_waveLookup(&d->OP.wave, (PH) + fmd_pm(PM))
	// FB: the feedback operator's enveloped output, kept for the next two samples
#define FMT_LOOP(BODY) \
	for(uint32_t i = 0; i < numSamples; i++) \
	{ \
		const float eA = fmd_envNext(&d->envA); \
		const float eB = fmd_envNext(&d->envB); \
		const float eB2 = fmd_envNext(&d->envB2); \
		const float f = fb * (t1 + t2); \
		float x, y, fo; \
		BODY \
		t2 = t1; t1 = fo; \
		out[i] = (x * gx + y * gy) * env * gain; \
		subDrive += subDriveStep; subWet += subWetStep; \
		subBuf[i] = subLevel != 0.f ? fmt_subFold(fmt_sub(subShape, phS, subDt), subDrive, subWet) * subLevel * env * gain : 0.f; \
		phC += incC; phA += incA; phB += incB; phB2 += incB2; phS += incS; \
		env *= coef; \
	}

	switch(al)
	{
	default:
	case 0:	// A > C; B2 > B1 > C. Feedback A. X C, Y B1
		FMT_LOOP(
			const float a = FMT_OP(opA, phA, f); fo = a * eA;
			const float b2 = FMT_OP(opB2, phB2, 0.f);
			const float b1 = FMT_OP(opB, phB, b2 * mB2 * eB2);
			x = FMT_OP(opC, phC, a * mA * eA + b1 * mB * eB);
			y = b1 * lB * eB;
		)
		break;
	case 1:	// A > C; B2 > B1. Feedback B2. X C, Y B1
		FMT_LOOP(
			const float b2 = FMT_OP(opB2, phB2, f); fo = b2 * eB2;
			const float b1 = FMT_OP(opB, phB, b2 * mB2 * eB2);
			const float a = FMT_OP(opA, phA, 0.f);
			x = FMT_OP(opC, phC, a * mA * eA);
			y = b1 * lB * eB;
		)
		break;
	case 2:	// A > C, B1 and B2. Feedback A. X C + B2, Y B1
		FMT_LOOP(
			const float a = FMT_OP(opA, phA, f); fo = a * eA;
			const float am = a * mA * eA;
			const float b2 = FMT_OP(opB2, phB2, am);
			const float b1 = FMT_OP(opB, phB, am);
			x = FMT_OP(opC, phC, am) + b2 * lB2 * eB2;
			y = b1 * lB * eB;
		)
		break;
	case 3:	// B2 > B1 > A > C. Feedback B2. X C, Y B1
		FMT_LOOP(
			const float b2 = FMT_OP(opB2, phB2, f); fo = b2 * eB2;
			const float b1 = FMT_OP(opB, phB, b2 * mB2 * eB2);
			const float a = FMT_OP(opA, phA, b1 * mB * eB);
			x = FMT_OP(opC, phC, a * mA * eA);
			y = b1 * lB * eB;
		)
		break;
	case 4:	// B1 > B2; B1 and B2 > A > C. Feedback B1. X C, Y A
		FMT_LOOP(
			const float b1 = FMT_OP(opB, phB, f); fo = b1 * eB;
			const float b1m = b1 * mB * eB;
			const float b2 = FMT_OP(opB2, phB2, b1m);
			const float a = FMT_OP(opA, phA, b1m + b2 * mB2 * eB2);
			x = FMT_OP(opC, phC, a * mA * eA);
			y = a * lA * eA;
		)
		break;
	case 5:	// A and B2 > C and B1. Feedback A. X C, Y B1
		FMT_LOOP(
			const float a = FMT_OP(opA, phA, f); fo = a * eA;
			const float b2 = FMT_OP(opB2, phB2, 0.f);
			const float m = a * mA * eA + b2 * mB2 * eB2;
			const float b1 = FMT_OP(opB, phB, m);
			x = FMT_OP(opC, phC, m);
			y = b1 * lB * eB;
		)
		break;
	case 6:	// A > C; B2 > B1. Feedback A. X C + A, Y B1 + B2
		FMT_LOOP(
			const float a = FMT_OP(opA, phA, f); fo = a * eA;
			const float b2 = FMT_OP(opB2, phB2, 0.f);
			const float b1 = FMT_OP(opB, phB, b2 * mB2 * eB2);
			x = FMT_OP(opC, phC, a * mA * eA) + a * lA * eA;
			y = b1 * lB * eB + b2 * lB2 * eB2;
		)
		break;
	case 7:	// A > C; B1 and B2 alone. Feedback B1. X C + B2, Y B1
		FMT_LOOP(
			const float b1 = FMT_OP(opB, phB, f); fo = b1 * eB;
			const float b2 = FMT_OP(opB2, phB2, 0.f);
			const float a = FMT_OP(opA, phA, 0.f);
			x = FMT_OP(opC, phC, a * mA * eA) + b2 * lB2 * eB2;
			y = b1 * lB * eB;
		)
		break;
	}
#undef FMT_LOOP
#undef FMT_OP

	d->opC.phase = phC; d->opA.phase = phA; d->opB.phase = phB; d->opB2.phase = phB2; d->subPhase = phS;
	d->env = env;
	d->fbT1 = t1; d->fbT2 = t2;
	d->subDrive1 = subDriveTo; d->subWet1 = subWetTo;

	if(bodyOnly)
	{
		if(d->params[FMD_FOLD])
			fmd_foldChunk(d, out, numSamples);
		else
		{
			d->foldOn = 0;
			const float level = d->params[FMD_BODY_LEVEL] / 100.f;
			for(uint32_t i = 0; i < numSamples; i++) out[i] *= level;
		}
	}
	if(subLevel != 0.f)
		for(uint32_t i = 0; i < numSamples; i++) out[i] += subBuf[i];
}

void fmDrum_render(FmDrum* d, float* out, uint32_t numSamples)
{
	while(numSamples)
	{
		if(d->ctrlCount == 0)
		{
			fmd_controlUpdate(d);
			d->ctrlCount = FMD_BLOCK_SIZE;
		}
		const uint32_t n = numSamples < d->ctrlCount ? numSamples : d->ctrlCount;
		if(d->params[FMD_TONE])
			fmt_renderChunk(d, out, n);
		else
			fmd_renderChunk(d, out, n);
		out += n;
		numSamples -= n;
		d->ctrlCount -= n;
	}
}

// --- randomise --------------------------------------------------------------------------------------------

// 32-bit integer hash (lowbias32): every input bit changes about half the output bits, so neighbouring
// group values give unrelated sounds
static uint32_t fmd_hash(uint32_t x)
{
	x ^= x >> 16;
	x *= 0x7feb352du;
	x ^= x >> 15;
	x *= 0x846ca68bu;
	x ^= x >> 16;
	return x;
}

typedef struct { uint32_t state; } FmdRng;

static uint32_t fmd_rngNext(FmdRng* r)
{
	r->state = fmd_hash(r->state + 0x9e3779b9u);
	return r->state;
}

// whole number in lo..hi
static uint8_t fmd_rngRange(FmdRng* r, uint8_t lo, uint8_t hi)
{
	return (uint8_t)(lo + fmd_rngNext(r) % (uint32_t)(hi - lo + 1));
}

// centre-weighted in lo..hi (average of two draws)
static uint8_t fmd_rngMid(FmdRng* r, uint8_t lo, uint8_t hi)
{
	return (uint8_t)(((uint32_t)fmd_rngRange(r, lo, hi) + fmd_rngRange(r, lo, hi) + 1) / 2);
}

// true with the given chance in percent
static uint8_t fmd_rngChance(FmdRng* r, uint8_t percent)
{
	return fmd_rngNext(r) % 100u < percent;
}

// harmonic and chordal entries of fmd_ratios: 1/2, 2/3, 1, the intervals 6/5 to 9/5 (not 15/8), 2, 9/4, 5/2, 8/3,
// 3, 10/3, 7/2, 4, 5, 6, 7, 8, 9
static const uint8_t fmd_harmonicRatios[] = {2, 3, 6, 7, 8, 9, 11, 12, 13, 15, 16, 18, 19, 21, 22, 24, 26, 28, 29, 33, 36, 39, 41, 43};
// metallic (inharmonic) entries: the square roots and pi and their multiples, 1/sqrt 2 to 3 pi
static const uint8_t fmd_metallicRatios[] = {4, 10, 14, 20, 23, 25, 27, 30, 32, 34, 35, 37, 38, 40, 42, 44};

static uint8_t fmd_isMetallic(uint8_t index)
{
	for(uint8_t i = 0; i < sizeof(fmd_metallicRatios); i++)
		if(fmd_metallicRatios[i] == index)
			return 1;
	return 0;
}

uint8_t fmDrum_ratioIsMetallic(uint8_t ratio)
{
	return fmd_isMetallic(ratio);
}

uint8_t fmDrum_randomFineApplies(uint8_t value, uint8_t ratio)
{
	return value >= 64 || (value > 0 && fmd_isMetallic(ratio));
}

uint8_t fmDrum_randomFineAppliesPal(uint8_t value, uint8_t ratio, uint8_t palette)
{
	if(palette == FMD_PAL_BAS || palette == FMD_PAL_HRM)
		return value > 0 && fmd_isMetallic(ratio);	// a fine on a whole number would detune the root
	return fmDrum_randomFineApplies(value, ratio);
}

// a ratio: 55 % harmonic or an interval, 30 % metallic, 15 % any entry. (The random fine is the FMD_RND_FINE
// group now; the draw it used is still made, so each tone value gives the same ratios, mods and feedback as
// in UMA 0.18.)
static void fmd_rndRatio(FmdRng* r, uint8_t* ratio)
{
	const uint32_t pick = fmd_rngNext(r) % 100u;
	if(pick < 55)
		*ratio = fmd_harmonicRatios[fmd_rngNext(r) % sizeof(fmd_harmonicRatios)];
	else if(pick < 85)
		*ratio = fmd_metallicRatios[fmd_rngNext(r) % sizeof(fmd_metallicRatios)];
	else
		*ratio = fmd_rngRange(r, 0, FMD_NUM_RATIOS - 1);
	if(fmd_isMetallic(*ratio))
		(void)fmd_rngRange(r, 0, 126);	// was the fine offset
}

// 'off' chance, then low or high: for feedback and end levels
static uint8_t fmd_rndMostlyLow(FmdRng* r, uint8_t offPercent, uint8_t lowPercent, uint8_t lowMax, uint8_t highMax)
{
	const uint32_t roll = fmd_rngNext(r) % 100u;
	if(roll < offPercent) return 0;
	if(roll < (uint32_t)offPercent + lowPercent) return fmd_rngRange(r, 1, lowMax);
	return fmd_rngRange(r, (uint8_t)(lowMax + 1), highMax);
}

static uint8_t fmd_rndAttack(FmdRng* r)
{
	return fmd_rndMostlyLow(r, 70, 20, 60, 110);	// mostly instant, sometimes short, rarely slow
}

// FMD_DEC_* / FMD_ATK_* value for a time, the inverse of fmDrum_decayTime / fmDrum_attackTime
static uint8_t fmd_decayValue(float t)
{
	const float v = 127.f * logf(t / 0.005f) / logf(1000.f);
	return v < 0.f ? 0 : v > 127.f ? 127 : (uint8_t)(v + 0.5f);
}
static uint8_t fmd_attackValue(float t)
{
	if(t < 0.0005f) return 0;
	const float v = 1.f + 126.f * logf(t / 0.0005f) / logf(4000.f);
	return v > 127.f ? 127 : (uint8_t)v;	// rounded down: stays under the time
}

// operator decay as a share of the sound's length: mostly 30-100 %, sometimes up to 150 %
static uint8_t fmd_rndDecayFor(FmdRng* r, float length)
{
	const float share = fmd_rngChance(r, 75) ? 0.3f + 0.7f * (fmd_rngNext(r) % 1001u) / 1000.f
	                                         : 1.f + 0.5f * (fmd_rngNext(r) % 1001u) / 1000.f;
	return fmd_decayValue(length * share);
}

// --- the ratio palettes (docs/design/fm-tone.md section 5) ---
// An operator that modulates C directly, or is heard, sets the pitch the ear hears: a ratio between 1 and 2
// that isn't 1 or 2 puts a tone below the root, an interval above 2 a second note. Those get the strict list.
static const uint8_t fmd_wholeRatios[] = {6, 18, 24, 29, 33, 36, 39, 41, 43};	// x1 to x9
// the metallic ones from x2.82 up: their tones land well above the root, grit
static const uint8_t fmd_highMetallicRatios[] = {23, 25, 27, 30, 32, 34, 35, 37, 38, 40, 42, 44};
// the just intervals: 1/3, 2/3, 3/4, 6/5 to 15/8, 9/4, 5/2, 8/3, 10/3, 7/2, 9/2
static const uint8_t fmd_intervalRatios[] = {1, 3, 5, 7, 8, 9, 11, 12, 13, 15, 16, 17, 19, 21, 22, 26, 28, 31};
#define FMD_RND_PICK(r, list)	((list)[fmd_rngNext(r) % sizeof(list)])

// FM Tone: per algorithm, the operators (bit 0 A, 1 B1, 2 B2) that only modulate another modulator
static const uint8_t fmt_rndLoose[FMT_NUM_ALGOS] = {4, 4, 0, 4, 6, 0, 0, 0};

// 1 if operator op (0 A, 1 B / B1, 2 B2) modulates C or is heard in this algorithm
static uint8_t fmd_rndStrict(uint8_t tone, uint8_t algo, uint8_t op)
{
	if(tone)
		return !((fmt_rndLoose[algo] >> op) & 1);
	return !(op == 1 && fmd_algos[algo].route == FMD_ROUTE_BAC);	// FM Drum: only B -> A -> C has B further away
}

static uint8_t fmd_rndPaletteRatio(FmdRng* r, uint8_t palette, uint8_t strict)
{
	const uint32_t pick = fmd_rngNext(r) % 100u;
	switch(palette)
	{
	case FMD_PAL_HRM:	// whole numbers; further away also an octave or two down
		if(strict || pick < 70) return FMD_RND_PICK(r, fmd_wholeRatios);
		return pick < 90 ? 2 : 0;
	case FMD_PAL_MET:
		return FMD_RND_PICK(r, fmd_metallicRatios);
	default:	// FMD_PAL_BAS
		if(strict)
			return pick < 60 ? FMD_RND_PICK(r, fmd_wholeRatios) : pick < 65 ? 2 : FMD_RND_PICK(r, fmd_highMetallicRatios);
		return pick < 35 ? FMD_RND_PICK(r, fmd_wholeRatios) : pick < 40 ? 2
		     : pick < 70 ? FMD_RND_PICK(r, fmd_intervalRatios) : FMD_RND_PICK(r, fmd_highMetallicRatios);
	}
}

uint8_t fmDrum_random(uint8_t group, uint8_t seed, uint8_t value, uint8_t drum, float length,
	FmDrumRandomValue out[FMD_RND_MAX_VALUES])
{
	return fmDrum_randomPal(group, seed, value, drum, length, FMD_PAL_ALL, 0, out);
}

// with a palette other than all: the tone draw by palette (FM Drum and FM Tone); on FM Tone also bass ranges
// for every group and B2, C's ratio. all: exactly the FM Drum's draw, on both engines
static uint8_t fmd_randomBass(FmdRng* r, uint8_t group, uint8_t palette, uint8_t tone, float length,
	FmDrumRandomValue out[FMD_RND_MAX_VALUES])
{
	uint8_t n = 0;
	switch(group)
	{
	case FMD_RND_TONE:
	{
		const uint8_t algo = (uint8_t)(fmd_rngNext(r) % (tone ? FMT_NUM_ALGOS : FMD_NUM_ALGOS));
		out[n++] = (FmDrumRandomValue){FMD_ALGO, algo};
		out[n++] = (FmDrumRandomValue){FMD_RATIO_A, fmd_rndPaletteRatio(r, palette, fmd_rndStrict(tone, algo, 0))};
		out[n++] = (FmDrumRandomValue){FMD_RATIO_B, fmd_rndPaletteRatio(r, palette, fmd_rndStrict(tone, algo, 1))};
		if(!tone)
		{
			// the drum's depths and feedback
			out[n++] = (FmDrumRandomValue){FMD_MOD_A, fmd_rngMid(r, 15, 105)};
			out[n++] = (FmDrumRandomValue){FMD_MOD_B, fmd_rngChance(r, 25) ? 0 : fmd_rngMid(r, 10, 100)};
			out[n++] = (FmDrumRandomValue){FMD_FEEDBACK, fmd_rndMostlyLow(r, 55, 30, 40, 100)};
			break;
		}
		// bass: lower depths (it stays focused), feedback mostly off or low
		out[n++] = (FmDrumRandomValue){FMD_MOD_A, fmd_rngMid(r, 10, 80)};
		out[n++] = (FmDrumRandomValue){FMD_MOD_B, fmd_rngChance(r, 20) ? 0 : fmd_rngMid(r, 10, 80)};
		out[n++] = (FmDrumRandomValue){FMD_FEEDBACK, fmd_rndMostlyLow(r, 65, 25, 35, 70)};
		out[n++] = (FmDrumRandomValue){FMD_RATIO_B2, fmd_rndPaletteRatio(r, palette, fmd_rndStrict(tone, algo, 2))};
		if(palette != FMD_PAL_BAS)	// bas keeps C where it is: the bass's octave is the player's
		{
			const uint32_t c = fmd_rngNext(r) % 100u;
			out[n++] = (FmDrumRandomValue){FMD_RATIO_C, c < 80 ? 6 : c < 90 ? 2 : 18};	// x1, sometimes x0.5 or x2
		}
		out[n++] = (FmDrumRandomValue){FMD_MOD_B2, fmd_rngChance(r, 30) ? 0 : fmd_rngMid(r, 10, 80)};
		break;
	}
	case FMD_RND_ENV:
		// A, B1, B2: longer than for drums; attacks from instant to a few hundred ms (plucks to swells)
		for(uint8_t op = 0; op < 3; op++)
		{
			const uint32_t roll = fmd_rngNext(r) % 100u;
			uint8_t atk = roll < 55 ? 0 : roll < 85 ? fmd_rngRange(r, 1, 60) : fmd_rngRange(r, 61, 98);
			uint8_t dec;
			if(length > 0.f)
			{
				const float share = fmd_rngChance(r, 70) ? 0.6f + 0.9f * (fmd_rngNext(r) % 1001u) / 1000.f
				                                         : 1.5f + 1.f * (fmd_rngNext(r) % 1001u) / 1000.f;
				dec = fmd_decayValue(length * share);
				if(fmDrum_attackTime(atk) > length / 2.f)
					atk = fmd_attackValue(length / 2.f);
			}
			else
				dec = fmd_rngMid(r, 40, 115);
			const uint8_t end = fmd_rngChance(r, 40) ? 0 : fmd_rngRange(r, 1, 40);	// some brightness kept
			static const uint8_t first[3] = {FMD_ATK_A, FMD_ATK_B, FMD_ATK_B2};
			static const uint8_t dist[3][2] = {{FMD_DEC_A, FMD_END_A}, {FMD_DEC_B, FMD_END_B}, {FMD_DEC_B2, FMD_END_B2}};
			out[n++] = (FmDrumRandomValue){first[op], atk};
			out[n++] = (FmDrumRandomValue){dist[op][0], dec};
			out[n++] = (FmDrumRandomValue){dist[op][1], end};
		}
		break;
	case FMD_RND_SWEEP:
		// mostly none, else a small blip at the attack
		out[n++] = (FmDrumRandomValue){FMD_SWEEP_AMT, fmd_rngChance(r, 60) ? 0 : fmd_rngRange(r, 6, 24)};
		out[n++] = (FmDrumRandomValue){FMD_SWEEP_DEC, fmd_rngRange(r, 10, 45)};
		out[n++] = (FmDrumRandomValue){FMD_SWEEP_SLOPE, fmd_rngRange(r, 20, 100)};
		break;
	default:	// FMD_RND_FINE: as all (which ratios take it: fmDrum_randomFineAppliesPal)
		out[n++] = (FmDrumRandomValue){FMD_FINE_A, fmd_rngRange(r, 0, 126)};
		out[n++] = (FmDrumRandomValue){FMD_FINE_B, fmd_rngRange(r, 0, 126)};
		break;
	}
	return n;
}

uint8_t fmDrum_randomPal(uint8_t group, uint8_t seed, uint8_t value, uint8_t drum, float length,
	uint8_t palette, uint8_t tone, FmDrumRandomValue out[FMD_RND_MAX_VALUES])
{
	if(length > 5.f)
		length = 5.f;	// a decay that never ends: treat as 5 s
	FmdRng r = { fmd_hash((uint32_t)seed | ((uint32_t)value << 8) | ((uint32_t)drum << 16) | ((uint32_t)group << 24)) };
	if(palette != FMD_PAL_ALL && palette < FMD_NUM_PALETTES && (tone || group == FMD_RND_TONE))
		return fmd_randomBass(&r, group, palette, tone, length, out);
	uint8_t n = 0;
	switch(group)
	{
	case FMD_RND_TONE:
	{
		uint8_t ratioA, ratioB;
		out[n++] = (FmDrumRandomValue){FMD_ALGO, (uint8_t)(fmd_rngNext(&r) % FMD_NUM_ALGOS)};
		fmd_rndRatio(&r, &ratioA);
		fmd_rndRatio(&r, &ratioB);
		out[n++] = (FmDrumRandomValue){FMD_RATIO_A, ratioA};
		out[n++] = (FmDrumRandomValue){FMD_RATIO_B, ratioB};
		out[n++] = (FmDrumRandomValue){FMD_MOD_A, fmd_rngMid(&r, 15, 105)};
		out[n++] = (FmDrumRandomValue){FMD_MOD_B, fmd_rngChance(&r, 25) ? 0 : fmd_rngMid(&r, 10, 100)};
		out[n++] = (FmDrumRandomValue){FMD_FEEDBACK, fmd_rndMostlyLow(&r, 55, 30, 40, 100)};
		break;
	}
	case FMD_RND_ENV:
		for(uint8_t op = 0; op < 2; op++)
		{
			uint8_t atk = fmd_rndAttack(&r);
			const uint8_t dec = length > 0.f ? fmd_rndDecayFor(&r, length) : fmd_rngMid(&r, 25, 110);
			// end levels change only a little: 40 % none, else up to 16 (about 13 % of mod)
			const uint8_t end = fmd_rngChance(&r, 40) ? 0 : fmd_rngRange(&r, 1, 16);
			if(length > 0.f && fmDrum_attackTime(atk) > length / 3.f)
				atk = fmd_attackValue(length / 3.f);
			out[n++] = (FmDrumRandomValue){op ? FMD_ATK_B : FMD_ATK_A, atk};
			out[n++] = (FmDrumRandomValue){op ? FMD_DEC_B : FMD_DEC_A, dec};
			out[n++] = (FmDrumRandomValue){op ? FMD_END_B : FMD_END_A, end};
		}
		break;
	case FMD_RND_SWEEP:
	{
		// smaller sweeps more often: the lower of two draws
		const uint8_t a1 = fmd_rngRange(&r, 20, 100), a2 = fmd_rngRange(&r, 20, 100);
		out[n++] = (FmDrumRandomValue){FMD_SWEEP_AMT, fmd_rngChance(&r, 15) ? 0 : (a1 < a2 ? a1 : a2)};
		out[n++] = (FmDrumRandomValue){FMD_SWEEP_DEC, fmd_rngRange(&r, 25, 85)};
		out[n++] = (FmDrumRandomValue){FMD_SWEEP_SLOPE, fmd_rngRange(&r, 20, 100)};
		break;
	}
	case FMD_RND_FINE:
		// anywhere between halfway to the ratio below and halfway to the one above; which ratios take it is
		// up to the caller (fmDrum_randomFineApplies)
		out[n++] = (FmDrumRandomValue){FMD_FINE_A, fmd_rngRange(&r, 0, 126)};
		out[n++] = (FmDrumRandomValue){FMD_FINE_B, fmd_rngRange(&r, 0, 126)};
		break;
	default:
		break;
	}
	return n;
}
