/*
 * uma_lfo.c - LX_UMA desktop: LFO B per voice, a mod matrix source (uma_lfo.h).
 *
 * The Eurorack's LFO B (LX_UMA a179390) runs on its modified LFO code; on the desktop LFO A stays stock (its 8 shapes,
 * Erica's lfo.c), so LFO B is a small LFO of its own here with the Eurorack's maths: the 16-shape morph and the folder
 * (lfoShapes.c), 16 sync divisions of the sequencer's tempo, restarts on a voice's hits or every 1, 2, 4, 8 bars.
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "main.h"
#include "uma_lfo.h"
#include "uma_mod.h"
#include "Parameters.h"
#include "menu/menu.h"
#include "Soundengine/config.h"
#include "Soundengine/DSPAudio/lfo.h"	// LFO_SR, LFO_MAX_F
#include "Soundengine/DSPAudio/lfoShapes.h"
#include "Soundengine/DSPAudio/random.h"
#include "Soundengine/Sequencer/sequencer.h"	// seq_getBpm

typedef struct
{
	uint32_t phase, phaseInc, phaseOffset;
	float freq, rnd, rndPrev, value;
	uint8_t waveform, retrigger, sync, fold;
} UmaLfoB;

static UmaLfoB umaLfoB[UMA_LFOB_SOURCES];
static uint8_t umaLfoB_set[UMA_LFOB_SOURCES][UMA_LFOB_PARS + 2];	// the settings last applied, then sync, fold

// setting k of voice v's LFO B: FREQ OFS WAVE RTG, then 4: SYNC, 5: FOLD
static uint16_t umaLfoB_parNr(uint8_t v, uint8_t k)
{
	if(k < UMA_LFOB_PARS)
		return (uint16_t)(UMA_LFOB_FIRST_PAR + v * UMA_LFOB_PARS + k);
	return k == UMA_LFOB_PARS ? (uint16_t)(UMA_LFOB_SYNC_PAR + v) : (uint16_t)(UMA_LFOB_FOLD_PAR + v);
}

// synced to the sequencer's tempo; free at freq while sync is off
static uint32_t umaLfoB_inc(const UmaLfoB* l)
{
	const uint32_t synced = lfoShapes_syncInc(l->sync, seq_getBpm(), LFO_SR);
	return synced ? synced : (uint32_t)(l->freq / LFO_SR * 4294967295.f);
}

// a setting as the voice's own LFO gets it (the Eurorack's umaLfoB_apply)
static void umaLfoB_apply(UmaLfoB* l, uint8_t k, uint8_t value)
{
	switch(k)
	{
	case 0:
	{
		float f = value / 127.f;
		f = 0.0078125f + f * (1.f - 0.0078125f);	// as lfo_setFreq: never zero
		l->freq = f * f * f * LFO_MAX_F;
		break;
	}
	case 1: l->phaseOffset = value >= 127 ? 0xffffffffu : (uint32_t)(value / 127.f * 4294967296.f); break;
	case 2: l->waveform = value < 128 ? value : 127; break;	// the shape morph 0-127
	case 3: l->retrigger = value; break;	// 0 off, 1-6 the voice whose hits restart it, 7-10 every 1 2 4 8 bars
	case 4: l->sync = value < LFOS_SYNC_RATES ? value : 0; break;
	case 5: l->fold = value; break;	// the wavefolder after the shape
	}
	l->phaseInc = umaLfoB_inc(l);
}

void umaLfoB_init(void)
{
	for(uint8_t v = 0; v < UMA_LFOB_SOURCES; v++)
	{
		umaLfoB[v].rnd = umaLfoB[v].rndPrev = umaLfoB[v].value = 0.5f;
		for(uint8_t k = 0; k <= UMA_LFOB_PARS + 1; k++)
		{
			umaLfoB_set[v][k] = activeParameters[umaLfoB_parNr(v, k)];
			umaLfoB_apply(&umaLfoB[v], k, umaLfoB_set[v][k]);
		}
	}
}

void umaLfoB_block(void)
{
	for(uint8_t v = 0; v < UMA_LFOB_SOURCES; v++)
	{
		UmaLfoB* l = &umaLfoB[v];
		for(uint8_t k = 0; k <= UMA_LFOB_PARS + 1; k++)
		{
			const uint8_t value = umaMod_get(umaLfoB_parNr(v, k));
			if(value != umaLfoB_set[v][k])
			{
				umaLfoB_set[v][k] = value;
				umaLfoB_apply(l, k, value);
			}
		}
		if(l->sync)
			l->phaseInc = umaLfoB_inc(l);	// the tempo moves
		const uint32_t old = l->phase;
		l->phase += l->phaseInc;
		if(old > l->phase)	// a new cycle: a new random value for the random shapes
		{
			l->rndPrev = l->rnd;
			l->rnd = GetRngValue() / (float)0xffffffff;
		}
		l->value = lfoShapes_fold(lfoShapes_value(l->waveform, l->phase / 4294967296.f, l->rnd, l->rndPrev), l->fold);
	}
}

void umaLfoB_barTick(uint32_t tick)
{
	for(uint8_t v = 0; v < UMA_LFOB_SOURCES; v++)
	{
		const uint8_t r = umaLfoB[v].retrigger;	// 7-10: every 1, 2, 4, 8 bars of 16 sixteenths
		if(r >= 7 && r <= 10 && tick % (16u << (r - 7)) == 0)
			umaLfoB[v].phase = umaLfoB[v].phaseOffset;
	}
}

void umaLfoB_hit(uint8_t voice)
{
	if(voice >= UMA_LFOB_SOURCES)
		voice = UMA_LFOB_SOURCES - 1;	// the open hi-hat is the hi-hat
	for(uint8_t v = 0; v < UMA_LFOB_SOURCES; v++)
		if(umaLfoB[v].retrigger == voice + 1)
			umaLfoB[v].phase = umaLfoB[v].phaseOffset;
}

uint8_t umaLfoA_fold(uint8_t voice)
{
	(void)voice;
	return 0;	// LFO A is the stock LFO on the desktop: no folder
}

float umaLfoB_value(uint8_t voice)
{
	return voice < UMA_LFOB_SOURCES ? umaLfoB[voice].value : 0.5f;
}
