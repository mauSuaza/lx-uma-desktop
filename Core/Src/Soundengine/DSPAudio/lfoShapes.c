/*
 * lfoShapes.c - UMA: the LFOs' shapes, their morph and the sync divisions (see lfoShapes.h).
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "Soundengine/DSPAudio/lfoShapes.h"
#include <math.h>

#define LFOS_PI			3.14159265f
#define LFOS_DETENT		0.18f	// the share of the way between two shapes that stays on each pure shape

static const char lfoShapes_names[LFOS_SHAPES][4] =
	{"sin", "tri", "sup", "sdn", "sqr", "rnd", "xup", "xdn", "scv", "bnc", "hsn", "st4", "st8", "rsm", "trp"};
static const char* const lfoShapes_long[LFOS_SHAPES] =
	{"sine", "tri", "saw up", "saw dn", "square", "random", "exp up", "exp dn",
	 "s-curve", "bounce", "halfsin", "4 steps", "8 steps", "rnd smth", "trapez"};

float lfoShapes_shape(uint8_t shape, float t, float rnd, float rndPrev)
{
	switch(shape)
	{
	default:
	case 0: return 0.5f + 0.5f * sinf(2.f * LFOS_PI * t);	// sine, from the middle upwards (as the LXR)
	case 1: return 1.f - fabsf(t * 2.f - 1.f);				// triangle, from 0
	case 2: return t;										// saw up
	case 3: return 1.f - t;									// saw down
	case 4: return t > 0.5f ? 1.f : 0.f;					// square
	case 5: return rnd;										// random, a new value each cycle
	case 6: return t * t * t;								// exponential up
	case 7: { const float x = 1.f - t; return x * x * x; }	// exponential down
	case 8: { const float x = 1.f - fabsf(t * 2.f - 1.f); return x * x * (3.f - 2.f * x); }	// S-curve
	case 9: return fabsf(sinf(4.f * LFOS_PI * t)) * (1.f - t);	// bounce: four, falling
	case 10: return t < 0.5f ? sinf(2.f * LFOS_PI * t) : 0.f;	// half sine, then rest
	case 11: { int s = (int)(t * 4.f); if(s > 3) s = 3; return s / 3.f; }	// 4 stairs up
	case 12: { int s = (int)(t * 8.f); if(s > 7) s = 7; return s / 7.f; }	// 8 stairs up
	case 13: return rndPrev + (rnd - rndPrev) * (0.5f - 0.5f * cosf(LFOS_PI * t));	// random, gliding
	case 14:												// trapezoid: rise, hold, fall, rest
		if(t < 0.25f) return t * 4.f;
		if(t < 0.5f) return 1.f;
		if(t < 0.75f) return (0.75f - t) * 4.f;
		return 0.f;
	}
}

// the morph on a list of n shapes
static void lfoShapes_blendN(uint8_t morph, uint8_t n, uint8_t* first, uint8_t* second, float* amount)
{
	const float x = (morph > 127 ? 127 : morph) * ((n - 1) / 127.f);
	uint8_t k = (uint8_t)x;
	if(k > n - 2) k = (uint8_t)(n - 2);
	float f = (x - k - LFOS_DETENT) / (1.f - 2.f * LFOS_DETENT);	// flat on each pure shape, a crossfade between
	f = f < 0.f ? 0.f : f > 1.f ? 1.f : f;
	if(f >= 1.f) { k++; f = 0.f; }
	*first = k;
	*second = (uint8_t)(k + 1 < n ? k + 1 : k);
	*amount = f;
}

void lfoShapes_blend(uint8_t morph, uint8_t* first, uint8_t* second, float* amount)
{
	lfoShapes_blendN(morph, LFOS_SHAPES, first, second, amount);
}

float lfoShapes_value(uint8_t morph, float t, float rnd, float rndPrev)
{
	uint8_t a, b;
	float f;
	lfoShapes_blend(morph, &a, &b, &f);
	const float va = lfoShapes_shape(a, t, rnd, rndPrev);
	if(f <= 0.f)
		return va;
	return va + (lfoShapes_shape(b, t, rnd, rndPrev) - va) * f;
}

const char* lfoShapes_name(uint8_t shape)
{
	return lfoShapes_names[shape < LFOS_SHAPES ? shape : 0];
}

const char* lfoShapes_longName(uint8_t shape)
{
	return lfoShapes_long[shape < LFOS_SHAPES ? shape : 0];
}

// off, 4/1 2/1 1/1 1/2, dotted quarter, 1/3, 1/4, dotted eighth, 1/6, 1/8, dotted sixteenth, 1/12, 1/16, 1/24, 1/32
// (fractions of a bar: quarter notes per cycle)
static const float lfoShapes_beats[LFOS_SYNC_RATES] =
	{0.f, 16.f, 8.f, 4.f, 2.f, 1.5f, 4.f / 3.f, 1.f, 0.75f, 2.f / 3.f, 0.5f, 0.375f, 1.f / 3.f, 0.25f, 1.f / 6.f, 0.125f};

float lfoShapes_syncBeats(uint8_t rate)
{
	return rate < LFOS_SYNC_RATES ? lfoShapes_beats[rate] : 0.f;
}

uint32_t lfoShapes_syncInc(uint8_t rate, float bpm, float controlRate)
{
	const float beats = lfoShapes_syncBeats(rate);
	if(beats <= 0.f || bpm <= 0.f || controlRate <= 0.f)
		return 0;
	const float cyclesPerSecond = bpm / 60.f / beats;
	return (uint32_t)(cyclesPerSecond / controlRate * 4294967295.f);
}

uint8_t lfoShapes_fromOldWave(uint8_t wave)
{
	if(wave > 7) wave = 7;
	return (uint8_t)((wave * 127 + 7) / 15);	// the shape's detent
}

uint8_t lfoShapes_from16(uint8_t morph)
{
	// the place on the old list: shape + crossfade. Pulse (13) was between 8 stairs and gliding random: its
	// stretch 12..14 is squeezed into 12..13; the shapes after it move down one
	uint8_t a, b;
	float f;
	lfoShapes_blendN(morph, 16, &a, &b, &f);
	const float p = a + f;
	const float want = p <= 12.f ? p : p < 14.f ? 12.f + (p - 12.f) * 0.5f : p - 1.f;
	// the new value that lands closest
	uint8_t best = 0;
	float bestErr = 1e9f;
	for(uint8_t v = 0; v < 128; v++)
	{
		lfoShapes_blendN(v, LFOS_SHAPES, &a, &b, &f);
		const float e = fabsf(a + f - want);
		if(e < bestErr - 1e-6f) { bestErr = e; best = v; }
	}
	return best;
}

float lfoShapes_fold(float value, uint8_t amount)
{
	if(amount == 0)
		return value;
	const float a = (amount > 127 ? 127 : amount) / 127.f;
	const float drive = 1.f + 4.f * a * a;
	const float wet = amount < 24 ? amount / 24.f : 1.f;
	const float x = value * 2.f - 1.f;	// -1..1
	const float y = x + (sinf(0.5f * LFOS_PI * drive * x) - x) * wet;
	return 0.5f + 0.5f * y;
}

uint8_t lfoShapes_fromOldSync(uint8_t sync)
{
	// the LXR's list: off 4/1 2/1 1/1 1/2 1/3 1/4 1/6 1/8 12 16 32
	static const uint8_t map[12] = {0, 1, 2, 3, 4, 6, 7, 9, 10, 12, 13, 15};
	return sync < 12 ? map[sync] : 0;
}
