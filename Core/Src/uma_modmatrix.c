/*
 * uma_modmatrix.c - LX_UMA desktop: the mod matrix (uma_modmatrix.h). From the Eurorack's ModMatrix_Processor.c
 * (LX_UMA a179390).
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "main.h"
#include "uma_modmatrix.h"
#include "uma_mod.h"
#include "uma_turing.h"
#include "uma_lfo.h"
#include "Soundengine/DSPAudio/random.h"
#include "Parameters.h"
#include "menu/menu.h"
#include "menu/umaModTargets.h"
#include "Soundengine/DSPAudio/DrumVoice.h"
#include "Soundengine/DSPAudio/Snare.h"
#include "Soundengine/DSPAudio/CymbalVoice.h"
#include "Soundengine/DSPAudio/HiHat.h"

#ifndef UMA_TURING
#define UMA_TURING 1	// the Turing Machines (0.3c)
#endif
#ifndef UMA_LFOB
#define UMA_LFOB 1		// LFO B (0.3b)
#endif

INDTCMZ ModulationNode cvModulators[NUM_MOD_SLOTS];

#define UMA_SRC_LFOA_FIRST	5	// lf1-lf6
#define UMA_SRC_ACCENT_FIRST	11	// ac1-ac6 (the Eurorack's accent jacks)

uint8_t umaModMx_sourceExists(uint8_t src)
{
	if(src < UMA_SRC_LFOA_FIRST) return 0;		// cv1-cv5
	if(src < UMA_SRC_ACCENT_FIRST) return 1;	// LFO A
	if(src < UMA_TM_FIRST_SOURCE) return 0;	// the accent jacks
	if(src < UMA_TM_FIRST_SOURCE + UMA_TM_SOURCES) return UMA_TURING;
	if(src < UMA_ENV_FIRST_SOURCE) return UMA_LFOB;
	return src < UMA_MC_FIRST_SOURCE;			// EN, EI, VL; not the MIDI CC sources
}

// a voice's body envelope 0..1 (the open hi-hat shares voice 6's), from the last block
static float umaModMx_env(uint8_t voice)
{
	switch(voice)
	{
	case 0: case 1: case 2:	return (float)voiceArray[voice].ampFilterInput;
	case 3:					return (float)snareVoice.egValueOscVol;
	case 4:					return (float)cymbalVoice.egValueOscVol;
	default:				return (float)hatVoice.egValueOscVol;
	}
}

// a voice's last hit velocity 0..1 (the step's, or a MIDI note's)
static float umaModMx_vel(uint8_t voice)
{
	switch(voice)
	{
	case 0: case 1: case 2:	return voiceArray[voice].velo;
	case 3:					return snareVoice.velo;
	case 4:					return cymbalVoice.velo;
	default:				return hatVoice.velo;
	}
}

static float umaModMx_lfoA(uint8_t voice)
{
	switch(voice)
	{
	case 0: case 1: case 2:	return voiceArray[voice].lfo.value;
	case 3:					return snareVoice.lfo.value;
	case 4:					return cymbalVoice.lfo.value;
	default:				return hatVoice.lfo.value;
	}
}

void umaModMx_init(void)
{
	for(uint8_t i = 0; i < NUM_MOD_SLOTS; i++)
		modNode_init(&cvModulators[i]);
	umaLfoB_init();
	umaTm_init(GetRngValue());	// the Turing Machines start from random patterns (a kit brings its own)
}

// the sequencer's sixteenths since it started: LFO B's bar restarts; the Turing Machines' clock (CLK `seq`)
static uint32_t umaModMx_tick;
void umaModMx_seqStart(void)
{
	umaModMx_tick = 0;
}
void umaModMx_seqStep(void)
{
	umaLfoB_barTick(umaModMx_tick);
	umaTm_clockTick();
	umaModMx_tick++;
}
void umaModMx_hit(uint8_t voice)
{
	umaLfoB_hit(voice);
	umaTm_hit(voice);
}

// v/o of each voice (0-127 = 0-60 semitones up, modulated) into its pitched oscillators, as the quantizer's settings
static void umaModMx_voct(void)
{
	float st[6];
	for(uint8_t v = 0; v < 6; v++)
		st[v] = umaMod_get((uint16_t)(UMA_DSK_VOCT_PAR + v)) * (60.f / 127.f);
	for(uint8_t v = 0; v < 3; v++)
	{
		voiceArray[v].osc.umaVoct = st[v];
		voiceArray[v].modOsc.umaVoct = st[v];
	}
	snareVoice.osc.umaVoct = st[3];
	cymbalVoice.osc.umaVoct = st[4];
	hatVoice.osc.umaVoct = st[5];
}

void umaModMx_process(void)
{
	umaLfoB_block();	// LFO B's next value, before the slots read it
	umaMod_newPass();	// modulated UMA settings not set again in this pass go stale
	for(uint8_t i = 0; i < NUM_MOD_SLOTS; i++)
	{
		const uint8_t amt = umaMod_get(modSlot_amt(i));	// the knob amount, or modulated by another slot
		if(amt == 0) continue;
		const uint8_t src = activeParameters[modSlot_src(i)];
		if(!umaModMx_sourceExists(src)) continue;
		cvModulators[i].amount = amt / 100.f;
		float cv;
		if(src < UMA_SRC_ACCENT_FIRST)
			cv = umaModMx_lfoA((uint8_t)(src - UMA_SRC_LFOA_FIRST));
		else if(src >= UMA_VEL_FIRST_SOURCE)
			cv = umaModMx_vel((uint8_t)(src - UMA_VEL_FIRST_SOURCE));
		else if(src >= UMA_ENV_FIRST_SOURCE)
		{
			// 0.5 is "no change" for every target: EN adds up to +amt with the envelope, EI pulls down to -amt
			const uint8_t k = (uint8_t)(src - UMA_ENV_FIRST_SOURCE);
			const float e = umaModMx_env((uint8_t)(k % 6));
			cv = k < 6 ? 0.5f + 0.5f * e : 0.5f - 0.5f * e;
		}
#if UMA_LFOB
		else if(src >= UMA_LFOB_FIRST_SOURCE)
			cv = umaLfoB_value((uint8_t)(src - UMA_LFOB_FIRST_SOURCE));
#endif
#if UMA_TURING
		else
			cv = umaTm_value((uint8_t)(src - UMA_TM_FIRST_SOURCE));
#else
		else
			cv = 0.5f;
#endif
		if(activeParameters[modSlot_pol(i)])	// unipolar, from the knob up
			cv = umaMod_unipolar(src, cv);
		modNode_updateValue(&cvModulators[i], cv);
	}
	umaModMx_voct();	// after the slots: this block's v/o
}
