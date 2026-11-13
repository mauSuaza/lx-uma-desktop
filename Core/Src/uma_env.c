/*
 * uma_env.c - UMA: hold, gate mode and the filter envelope of Drum 1-3 (see uma_env.h).
 *
* LX_UMA desktop (0.2a): the Eurorack's file (LX_UMA a179390) without the trigger jacks (the desktop's gate is a held
 * MIDI note), the desktop's track mute, and a setting's value from the kit (umaMod_get: Phase 3).
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "uma_env.h"
#include "uma.h"

#if UMA_FM
#include <math.h>
#include "main.h"
#include "Parameters.h"
#include "menu/menu.h"
#include "Soundengine/Sequencer/sequencer.h"	// seq_isTrackMuted
#include "Soundengine/config.h"
#include "Soundengine/DSPAudio/DrumVoice.h"
#include "Soundengine/DSPAudio/Lpg.h"

#define UMA_ENV_DRUMS		3
#define UMA_ENV_HOLD_MAX	2.f		// seconds at 127
#define UMA_ENV_OCTAVES		6.f		// filter envelope at full
#define UMA_NO_NOTE			0xff

// a setting's value (Phase 3: the modulated one)
static inline uint8_t umaMod_get(uint16_t p) { return activeParameters[p]; }

float fastTan(float x);	// ResonantFilter.c
float softClipTwo(float in);	// ResonantFilter.c

static Lpg umaLpg[UMA_ENV_DRUMS];
static uint8_t umaLpg_struck[UMA_ENV_DRUMS];	// hit in this block: excite taken at render, after the mod matrix

static uint8_t umaEnv_midiHeld[UMA_ENV_DRUMS] = {UMA_NO_NOTE, UMA_NO_NOTE, UMA_NO_NOTE};	// the held note
static uint8_t umaEnv_lastRel[UMA_ENV_DRUMS] = {0xff, 0xff, 0xff};

static uint16_t umaEnv_par(uint8_t drum, uint8_t k) { return (uint16_t)(UMA_ENV_FIRST_PAR + drum * UMA_ENV_PARS + k); }
#define ENV_HOLD	0
#define ENV_SUS		1
#define ENV_REL		2
#define ENV_GATE	3
#define ENV_FENV	4

uint8_t umaEnv_gateHeld(uint8_t drum)
{
	if(drum >= UMA_ENV_DRUMS || seq_isTrackMuted(drum))
		return 0;	// a muted drum's held gate ends
	return umaEnv_midiHeld[drum] != UMA_NO_NOTE;
}

void umaEnv_midiNote(uint8_t voice, uint8_t note, uint8_t on)
{
	if(voice >= UMA_ENV_DRUMS)
		return;
	if(on)
		umaEnv_midiHeld[voice] = note;
	else if(umaEnv_midiHeld[voice] == note)	// a newer note keeps the gate
		umaEnv_midiHeld[voice] = UMA_NO_NOTE;
}

void umaEnv_block(uint8_t drum)
{
	SlopeEg2* eg = &voiceArray[drum].oscVolEg;
	const float h = umaMod_get(umaEnv_par(drum, ENV_HOLD)) / 127.f;
	slopeEg2_setHold(eg, (uint16_t)(h * h * UMA_ENV_HOLD_MAX * REAL_FS / OUTPUT_DMA_SIZE + 0.5f));
	slopeEg2_setSustain(eg, umaMod_get(umaEnv_par(drum, ENV_SUS)) / 127.f);	// on the current slope's curve
	const uint8_t rel = umaMod_get(umaEnv_par(drum, ENV_REL));
	if(rel != umaEnv_lastRel[drum])
	{
		umaEnv_lastRel[drum] = rel;
		slopeEg2_setRelease(eg, rel);
	}
	if(eg->gated && !umaEnv_gateHeld(drum))
		slopeEg2_gateEnd(eg);
}

uint8_t umaEnv_lpgOn(uint8_t drum)
{
	return drum < UMA_ENV_DRUMS && voiceArray[drum].filterType == UMA_FILTER_LPG;
}

float umaEnv_foldEnv(uint8_t drum)
{
	if(drum >= UMA_ENV_DRUMS)
		return 1.f;
	return umaEnv_lpgOn(drum) ? lpg_level(&umaLpg[drum]) : (float)voiceArray[drum].ampFilterInput;
}

void umaEnv_trigger(uint8_t drum)
{
	if(drum >= UMA_ENV_DRUMS)
		return;
	umaLpg_struck[drum] = 1;
	// gate mode, and a gate is behind this hit; otherwise it plays as trg (decays) so it never hangs
	voiceArray[drum].oscVolEg.gated = activeParameters[umaEnv_par(drum, ENV_GATE)] && umaEnv_gateHeld(drum);
}

// the low-pass gate: filter and volume from its vactrol, struck by the hit (Lpg.c, after Plaits)
static void umaLpg_block(uint8_t drum, int16_t* buf, uint8_t size)
{
	ResonantFilter* f = &voiceArray[drum].filter;
	Lpg* l = &umaLpg[drum];
	const uint16_t p = (uint16_t)(UMA_LPG_FIRST_PAR + drum * UMA_LPG_PARS);
	if(umaLpg_struck[drum])
	{
		umaLpg_struck[drum] = 0;
		lpg_trigger(l, umaMod_get(p + 2) / 127.f);	// exc: the modulated value at this hit
	}
	// dcy: 35 ms to 9 s to -40 dB (Plaits' decay -0.4 .. 0.9); open: frq (knob and matrix); res: 0 = Plaits
	const float decay = -0.4f + 1.3f * (umaMod_get(p) / 127.f);
	const float colour = umaMod_get((uint16_t)(p + 1)) / 127.f;
	float open = f->f / 0.45f;
	open = open < 0.f ? 0.f : open > 1.f ? 1.f : open;
	const float resonance = 1.f - f->q;
	float x[LPG_BLOCK];
	for(uint8_t i = 0; i < size; i += LPG_BLOCK)
	{
		const uint8_t n = (uint8_t)(size - i < LPG_BLOCK ? size - i : LPG_BLOCK);
		lpg_control(l, decay, colour, open, resonance, REAL_FS);
		for(uint8_t j = 0; j < n; j++)
			x[j] = softClipTwo((buf[i + j] / 32767.f) * f->drive);	// drv as the LXR's filter input
		lpg_process(l, x, n);
		for(uint8_t j = 0; j < n; j++)
		{
			float y = x[j] * 32767.f;
			buf[i + j] = (int16_t)(y > 32767.f ? 32767.f : y < -32767.f ? -32767.f : y);
		}
	}
}

void umaEnv_filterBlock(uint8_t drum, int16_t* buf, uint8_t size)
{
	ResonantFilter* f = &voiceArray[drum].filter;
	if(voiceArray[drum].filterType == UMA_FILTER_LPG)
	{
		umaLpg_block(drum, buf, size);
		return;
	}
	umaLpg_struck[drum] = 0;
	const int8_t env = (int8_t)(umaMod_get(umaEnv_par(drum, ENV_FENV)) - 63);
	if(env == 0)
	{
		SVF_calcBlockZDF(f, voiceArray[drum].filterType, buf, size);
		return;
	}
	// the cutoff (knob and mod matrix) moved in octaves by the body envelope, for this block only
	const float amount = (env > 63 ? 63 : env) / 63.f;
	float fc = f->f * exp2f(amount * UMA_ENV_OCTAVES * (float)voiceArray[drum].ampFilterInput);
	if(fc > 0.449f) fc = 0.449f;
	if(fc < 0.0002f) fc = 0.0002f;
	const float keepF = f->f, keepG = f->g;
	f->f = fc;
	f->g = fastTan((float)M_PI * fc);
	SVF_calcBlockZDF(f, voiceArray[drum].filterType, buf, size);
	f->f = keepF;
	f->g = keepG;
}

#else
void umaEnv_block(uint8_t drum) { (void)drum; }
void umaEnv_trigger(uint8_t drum) { (void)drum; }
void umaEnv_midiNote(uint8_t voice, uint8_t note, uint8_t on) { (void)voice; (void)note; (void)on; }
uint8_t umaEnv_gateHeld(uint8_t drum) { (void)drum; return 0; }
#endif
