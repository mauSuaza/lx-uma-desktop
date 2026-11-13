/*
 * uma_quant.c - UMA: the scale and chord quantizer (docs/design/quantizer.md). The tables and the snap are plain C
 * (the Mac test uses them as they are); umaQuant_block reads the voice's settings on the module.
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "uma_quant.h"
#include <math.h>

typedef struct
{
	uint16_t mask;
	char shortName[4];
	const char* name;
} UmaScale;

#define N(a) (1u << (a))

// docs/design/quantizer.md section 3, in its order
static const UmaScale umaQuant_scales[UMA_QUANT_SCALES] =
{
	{0xfff, "off", "Off (chromatic)"},
	// scales
	{N(0)|N(2)|N(4)|N(5)|N(7)|N(9)|N(11), "maj", "Major"},
	{N(0)|N(2)|N(3)|N(5)|N(7)|N(8)|N(10), "min", "Minor"},
	{N(0)|N(2)|N(3)|N(5)|N(7)|N(8)|N(11), "hmi", "Harmonic minor"},
	{N(0)|N(2)|N(3)|N(5)|N(7)|N(9)|N(11), "mmi", "Melodic minor"},
	{N(0)|N(2)|N(3)|N(5)|N(7)|N(9)|N(10), "dor", "Dorian"},
	{N(0)|N(1)|N(3)|N(5)|N(7)|N(8)|N(10), "phr", "Phrygian"},
	{N(0)|N(2)|N(4)|N(6)|N(7)|N(9)|N(11), "lyd", "Lydian"},
	{N(0)|N(2)|N(4)|N(5)|N(7)|N(9)|N(10), "mix", "Mixolydian"},
	{N(0)|N(2)|N(4)|N(7)|N(9), "pnM", "Pentatonic maj"},
	{N(0)|N(3)|N(5)|N(7)|N(10), "pnm", "Pentatonic min"},
	// major family
	{N(0)|N(4)|N(7), "M", "Major"},
	{N(0)|N(4)|N(7)|N(9), "M6", "Major 6"},
	{N(0)|N(4)|N(7)|N(11), "M7", "Major 7"},
	{N(0)|N(4)|N(7)|N(10), "7", "Dominant 7"},
	{N(0)|N(2)|N(4)|N(7), "ad9", "Add 9"},
	{N(0)|N(2)|N(4)|N(7)|N(11), "M9", "Major 9"},
	{N(0)|N(2)|N(4)|N(7)|N(10), "9", "Dominant 9"},
	{N(0)|N(2)|N(4)|N(5)|N(7)|N(11), "M11", "Major 11"},
	{N(0)|N(2)|N(4)|N(5)|N(7)|N(10), "11", "Dominant 11"},
	{N(0)|N(4)|N(6)|N(7)|N(11), "#11", "Major 7 #11"},
	{N(0)|N(2)|N(4)|N(7)|N(9)|N(11), "M13", "Major 13"},
	{N(0)|N(2)|N(4)|N(7)|N(9)|N(10), "13", "Dominant 13"},
	// minor, with alterations
	{N(0)|N(3)|N(7), "m", "Minor"},
	{N(0)|N(3)|N(7)|N(9), "m6", "Minor 6"},
	{N(0)|N(3)|N(7)|N(10), "m7", "Minor 7"},
	{N(0)|N(3)|N(7)|N(11), "mM7", "Minor major 7"},
	{N(0)|N(2)|N(3)|N(7), "ma9", "Minor add 9"},
	{N(0)|N(2)|N(3)|N(7)|N(10), "m9", "Minor 9"},
	{N(0)|N(2)|N(3)|N(5)|N(7)|N(10), "m11", "Minor 11"},
	{N(0)|N(2)|N(3)|N(7)|N(9)|N(10), "m13", "Minor 13"},
	{N(0)|N(3)|N(7)|N(8), "mb6", "Minor b6"},
	{N(0)|N(3)|N(6)|N(10), "m7-", "Half-dim m7b5"},
	{N(0)|N(3)|N(6), "dim", "Diminished"},
	{N(0)|N(3)|N(6)|N(9), "dm7", "Diminished 7"},
	// suspended, augmented, altered dominants
	{N(0)|N(2)|N(7), "su2", "Sus 2"},
	{N(0)|N(5)|N(7), "su4", "Sus 4"},
	{N(0)|N(5)|N(7)|N(10), "7s4", "7 sus 4"},
	{N(0)|N(4)|N(8), "aug", "Augmented"},
	{N(0)|N(1)|N(4)|N(7)|N(10), "7b9", "7 b9"},
	{N(0)|N(3)|N(4)|N(7)|N(10), "7#9", "7 #9"},
	{N(0)|N(4)|N(8)|N(10), "7#5", "7 #5"},
	{N(0)|N(7), "5", "Fifth"},
	{N(0), "oct", "Octaves"},
	// the harmonic minor's seven chords, from the key (the scale's root): 0 2 3 5 7 8 11
	{N(0)|N(3)|N(7)|N(11), "h1", "Harm min i mM7"},
	{N(2)|N(5)|N(8)|N(0), "h2", "Harm min ii m7b5"},
	{N(3)|N(7)|N(11)|N(2), "h3", "Harm min III+M7"},
	{N(5)|N(8)|N(0)|N(3), "h4", "Harm min iv m7"},
	{N(7)|N(11)|N(2)|N(5)|N(8), "h5", "Harm min V7b9"},
	{N(8)|N(0)|N(3)|N(7), "h6", "Harm min VI M7"},
	{N(11)|N(2)|N(5)|N(8), "h7", "Harm min vii dim"},
};

static const char umaQuant_keys[UMA_QUANT_KEYS][3] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

uint16_t umaQuant_mask(uint8_t scale)
{
	return umaQuant_scales[scale < UMA_QUANT_SCALES ? scale : 0].mask;
}
const char* umaQuant_scaleShort(uint8_t scale) { return umaQuant_scales[scale < UMA_QUANT_SCALES ? scale : 0].shortName; }
const char* umaQuant_scaleName(uint8_t scale) { return umaQuant_scales[scale < UMA_QUANT_SCALES ? scale : 0].name; }
const char* umaQuant_keyName(uint8_t key) { return umaQuant_keys[key % UMA_QUANT_KEYS]; }

static uint8_t umaQuant_allowed(int n, uint8_t key, uint16_t mask)
{
	const int pc = ((n - key) % 12 + 12) % 12;
	return (mask >> pc) & 1u;
}

int16_t umaQuant_snap(float note, uint8_t key, uint8_t scale, int16_t* held)
{
	const uint16_t mask = umaQuant_mask(scale);
	key %= UMA_QUANT_KEYS;
	if(note < -200.f) note = -200.f;
	if(note > 300.f) note = 300.f;
	// the nearest allowed note: below and above, at most an octave away (every mask has its root); ties go down
	const int lo0 = (int)floorf(note);
	int below = lo0, above = lo0 + 1;
	while(!umaQuant_allowed(below, key, mask)) below--;
	while(!umaQuant_allowed(above, key, mask)) above++;
	const float dBelow = note - (float)below, dAbove = (float)above - note;
	int best = dAbove < dBelow ? above : below;
	const float dBest = dAbove < dBelow ? dAbove : dBelow;
	if(held)
	{
		const int h = *held;
		if(h != UMA_QUANT_NONE && h != best && umaQuant_allowed(h, key, mask)
			&& fabsf(note - (float)h) <= dBest + 2.f * UMA_QUANT_HYST)	// within HYST past the midpoint
			best = h;
		*held = (int16_t)best;
	}
	return (int16_t)best;
}

#ifndef UMA_QUANT_HOST_TEST	// the Mac test has only the tables and the snap
#include "Parameters.h"
#include "uma_mod.h"
#include "Soundengine/DSPAudio/DrumVoice.h"
#include "Soundengine/DSPAudio/Snare.h"
#include "Soundengine/DSPAudio/CymbalVoice.h"
#include "Soundengine/DSPAudio/HiHat.h"

static void umaQuant_set(OscInfo* osc, uint8_t oct, uint8_t key, uint8_t scale)
{
	osc->umaOct = (int8_t)((oct > UMA_QUANT_OCT_MAX ? UMA_QUANT_OCT_MAX : oct) - UMA_QUANT_OCT_ZERO);
	osc->umaKey = (uint8_t)(key % UMA_QUANT_KEYS);
	osc->umaScale = scale < UMA_QUANT_SCALES ? scale : 0;
}

void umaQuant_block(uint8_t voice)
{
	if(voice >= UMA_QUANT_VOICES) return;
	const uint16_t p = (uint16_t)(UMA_QUANT_FIRST_PAR + voice * UMA_QUANT_PARS);
	const uint8_t oct = umaMod_get(p), key = umaMod_get((uint16_t)(p + 1)), scale = umaMod_get((uint16_t)(p + 2));
	// the voice's pitch oscillator; Drum 1-3's mod oscillator too, so the lxr engine's FM keeps its ratio
	if(voice < 3)
	{
		umaQuant_set(&voiceArray[voice].osc, oct, key, scale);
		umaQuant_set(&voiceArray[voice].modOsc, oct, key, scale);
	}
	else if(voice == 3)
		umaQuant_set(&snareVoice.osc, oct, key, scale);
	else if(voice == 4)
		umaQuant_set(&cymbalVoice.osc, oct, key, scale);
	else
		umaQuant_set(&hatVoice.osc, oct, key, scale);
}
#endif
