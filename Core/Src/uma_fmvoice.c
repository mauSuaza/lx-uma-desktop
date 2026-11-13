/*
 * uma_fmvoice.c - LX_UMA: FM Drum and FM Tone on Drum 1-3 (uma_fmvoice.h). From the Eurorack's DrumVoice.c
 * (umaFm_init, umaFm_isOn, umaFm_syncParams, umaFm_syncFold, umaVoice_closed; LX_UMA a179390).
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "main.h"
#include "uma_fmvoice.h"
#include "Parameters.h"
#include "menu/menu.h"	// activeParameters
#include "Soundengine/config.h"
#include "Soundengine/DSPAudio/DrumVoice.h"
#include "Soundengine/DSPAudio/FmDrum.h"
#include "uma_env.h"
#include "uma_random.h"
#include "uma_voice.h"
#include "uma_mod.h"
#include "Soundengine/DSPAudio/MdDrum.h"
#include <math.h>

#define UMA_FMV_DRUMS 3
static FmDrum umaFm[UMA_FMV_DRUMS];

// a setting's value (Phase 3: the modulated one, umaMod_get)
static inline uint8_t umaPar(uint16_t p)
{
	return activeParameters[p];
}

static uint8_t umaFm_isOn(uint8_t voiceNr)
{
	if(voiceNr >= UMA_FMV_DRUMS)
		return 0;
	const uint8_t e = umaVoice_engine(voiceNr);
	return e == UMA_ENG_FMD || e == UMA_ENG_FMT;
}

// the drum-machine engine (0.2c): its own amp and pitch envelopes, then the wavefolder; the voice's transient, filter,
// drive and mixer stay, the amp EG doesn't scale the volume
static volatile uint8_t umaMd_hitPending[UMA_FMV_DRUMS];
static uint8_t umaMd_isOn(uint8_t voiceNr)
{
	return voiceNr < UMA_FMV_DRUMS && umaVoice_engine(voiceNr) == UMA_ENG_MDK;
}

// the voice's amp EG is shut: the engine would be silent, so it isn't run (the next hit starts it as always)
static uint8_t umaFm_closed(uint8_t voiceNr)
{
	if(umaEnv_lpgOn(voiceNr))
		return umaEnv_foldEnv(voiceNr) < 1e-5f;
	return voiceArray[voiceNr].ampFilterInput <= 0.f && voiceArray[voiceNr].lastGain <= 0.f;
}

__attribute__((noinline)) void umaFmV_init(void)
{
	for(uint8_t v = 0; v < UMA_FMV_DRUMS; v++)
	{
		fmDrum_init(&umaFm[v], REAL_FS);
		fmDrum_setAmpEnv(&umaFm[v], 0);					// the voice's amp EG shapes the volume
		fmDrum_setParam(&umaFm[v], FMD_SWEEP_AMT, 0);	// the voice's pitch EG does the sweep
		fmDrum_setParam(&umaFm[v], FMD_LEVEL, 127);		// the voice's mixer sets the volume
		mdDrum_init(v);
	}
	umaVoice_init();	// the snare, cymbal and hi-hat's engines
}

__attribute__((noinline)) void umaFmV_trigger(uint8_t voiceNr)
{
	if(voiceNr < UMA_FMV_DRUMS)
		fmDrum_trigger(&umaFm[voiceNr], 1.f);	// velocity is applied by the voice
	if(voiceNr < UMA_FMV_DRUMS)
		umaMd_hitPending[voiceNr] = 1;	// started in umaFmV_block, after randomise drew this block's machine
	umaRnd_hit(voiceNr);	// a modulated randomise setting re-rolls in this block (Phase 3), before the voice renders
}

// wavefolder and body level; the fold's drive follows the amp EG, or the LPG's vactrol
static void umaFm_syncFold(uint8_t voiceNr)
{
	FmDrum* d = &umaFm[voiceNr];
	static const uint8_t foldParam[UMA_FOLD_PARS] = {FMD_FOLD, FMD_FOLD_TYPE, FMD_FOLD_SYM, FMD_BODY_LEVEL};
	for(uint8_t j = 0; j < UMA_FOLD_PARS; j++)
	{
		uint8_t v = umaPar((uint16_t)(UMA_FOLD_FIRST_PAR + voiceNr * UMA_FOLD_PARS + j));
		if(j == 1) v = v ? 1 : 0;
		if(fmDrum_getParam(d, foldParam[j]) != v)
			fmDrum_setParam(d, foldParam[j], v);
	}
	fmDrum_setFoldEnv(d, umaEnv_foldEnv(voiceNr));
}

_Static_assert(UMA_FM_RATIO_STEPS == FMD_NUM_RATIOS, "the kit's ratio steps are the engine's ratio table");
static void umaFm_syncParams(uint8_t voiceNr)
{
	static const uint8_t engineParam[UMA_FM_PARS_PER_DRUM] =
	{
		0xff,	// engine setting: not an engine parameter
		FMD_ALGO, FMD_FEEDBACK, FMD_RATIO_A, FMD_RATIO_B,
		FMD_MOD_A, FMD_ATK_A, FMD_DEC_A, FMD_END_A,
		FMD_MOD_B, FMD_ATK_B, FMD_DEC_B, FMD_END_B,
	};
	const uint16_t kit = (uint16_t)(PAR_UMA_ENG1 + voiceNr * UMA_FM_PARS_PER_DRUM);
	FmDrum* d = &umaFm[voiceNr];
	const uint8_t tone = umaPar(kit) == 2;	// fmt
	if(fmDrum_getParam(d, FMD_TONE) != tone)
		fmDrum_setParam(d, FMD_TONE, tone);
	for(uint8_t i = 1; i < UMA_FM_PARS_PER_DRUM; i++)
	{
		uint8_t v = umaPar((uint16_t)(kit + i));
		if(engineParam[i] == FMD_RATIO_A || engineParam[i] == FMD_RATIO_B)
			v = (uint8_t)(v < UMA_FM_RATIO_STEPS ? v : UMA_FM_RATIO_STEPS - 1);
		if(fmDrum_getParam(d, engineParam[i]) != v)
			fmDrum_setParam(d, engineParam[i], v);
	}
	// fine ratio A and B
	for(uint8_t j = 0; j < 2; j++)
	{
		const uint8_t v = umaPar((uint16_t)(PAR_UMA_FINE_A1 + voiceNr * 2 + j));
		if(fmDrum_getParam(d, (uint8_t)(FMD_FINE_A + j)) != v)
			fmDrum_setParam(d, (uint8_t)(FMD_FINE_A + j), v);
	}
	umaFm_syncFold(voiceNr);
	// (operator waveforms: none on the desktop, FMD_NO_WAVES; the kit's wvc / wvm are kept but not played)
	// FM Tone: ratio C and B2, mod B2, detune, mix X/Y, the sub
	if(!tone)
		return;
	static const uint8_t toneParam[UMA_TONE_PARS] = {FMD_RATIO_C, FMD_RATIO_B2, FMD_MOD_B2, FMD_DETUNE, FMD_MIX,
		FMD_SUB_LEVEL, FMD_SUB_SHAPE, FMD_SUB_OCT, FMD_SUB_FOLD};
	for(uint8_t j = 0; j < UMA_TONE_PARS; j++)
	{
		uint8_t v = umaPar((uint16_t)(UMA_TONE_FIRST_PAR + voiceNr * UMA_TONE_PARS + j));
		if(j < 2 && v >= UMA_FM_RATIO_STEPS)
			v = UMA_FM_RATIO_STEPS - 1;
		if(fmDrum_getParam(d, toneParam[j]) != v)
			fmDrum_setParam(d, toneParam[j], v);
	}
	// B2's own operator envelope
	for(uint8_t j = 0; j < UMA_B2ENV_PARS; j++)
	{
		const uint8_t v = umaPar((uint16_t)(UMA_B2ENV_FIRST_PAR + voiceNr * UMA_B2ENV_PARS + j));
		if(fmDrum_getParam(d, (uint8_t)(FMD_ATK_B2 + j)) != v)
			fmDrum_setParam(d, (uint8_t)(FMD_ATK_B2 + j), v);
	}
}


// the machine: the one rmc drew, else the type and machine settings. The knobs belong to the machine the settings
// choose (the home machine); any other machine reads them relative to its own defaults (the Eurorack's DrumVoice.c
// umaMd_syncParams)
static void umaMd_syncParams(uint8_t voiceNr)
{
	const uint16_t kit = (uint16_t)(UMA_MD_FIRST_PAR + voiceNr * UMA_MD_PARS);
	const uint8_t home = mdDrum_machineOf(activeParameters[kit], activeParameters[kit + 1]);
	const uint8_t drawn = umaRnd_mdMachine(voiceNr);
	const uint8_t played = drawn < MD_MACHINES ? drawn : mdDrum_machineOf(umaMod_get(kit), umaMod_get((uint16_t)(kit + 1)));
	mdDrum_setMachine(voiceNr, played);
	uint8_t defaults[MD_CONTROLS], homeDefaults[MD_CONTROLS];
	if(played != home)
	{
		mdDrum_defaults(played, defaults);
		mdDrum_defaults(home, homeDefaults);
	}
	for(uint8_t c = 0; c < MD_CONTROLS; c++)
	{
		int v = umaMod_get((uint16_t)(kit + 2 + c));
		if(played != home)
			v = defaults[c] + v - homeDefaults[c];
		mdDrum_setControl(voiceNr, c, (uint8_t)(v < 0 ? 0 : v > 127 ? 127 : v));
	}
	umaFm_syncFold(voiceNr);
	// the voice's pitch without the pitch EG (the machine has its own): C4 plays the machine as voiced
	const OscInfo* osc = &voiceArray[voiceNr].osc;
	const float f = osc->freq * osc->modNodeValue * (44100.f / REAL_FS);
	mdDrum_setTranspose(voiceNr, f > 1.f ? 12.f * log2f(f / 261.6256f) : 0.f);
}

__attribute__((noinline)) void umaFmV_block(uint8_t voiceNr, float freqHz)
{
	if(umaMd_isOn(voiceNr))
	{
		umaMd_syncParams(voiceNr);
		if(umaMd_hitPending[voiceNr])
		{
			umaMd_hitPending[voiceNr] = 0;
			mdDrum_trigger(voiceNr);	// with this block's machine and knobs
		}
		return;
	}
	if(voiceNr < UMA_FMV_DRUMS)
		umaMd_hitPending[voiceNr] = 0;	// a hit on another engine doesn't wait for mdk
	if(!umaFm_isOn(voiceNr))
		return;
	umaFm_syncParams(voiceNr);
	fmDrum_setPitch(&umaFm[voiceNr], freqHz);	// C at the voice's pitch
}

__attribute__((noinline)) uint8_t umaFmV_render(uint8_t voiceNr, int16_t* buf, uint8_t size)
{
	if(umaMd_isOn(voiceNr))
	{
		// the drum machine, through the wavefolder
		float mdBuf[MD_BLOCK];
		if(size == MD_BLOCK)
			mdDrum_render(voiceNr, mdBuf);
		else
			for(uint8_t i = 0; i < MD_BLOCK; i++) mdBuf[i] = 0.f;
		fmDrum_foldBlock(&umaFm[voiceNr], mdBuf, size);
		for(uint8_t i = 0; i < size; i++)
		{
			float x = mdBuf[i] * 32767.f;
			buf[i] = (int16_t)(x > 32767.f ? 32767.f : x < -32767.f ? -32767.f : x);
		}
		return 1;
	}
	if(!umaFm_isOn(voiceNr))
		return 0;
	if(umaFm_closed(voiceNr) || size > FMD_BLOCK_SIZE)
	{
		for(uint8_t i = 0; i < size; i++) buf[i] = 0;
		return 1;
	}
	float fmBuf[FMD_BLOCK_SIZE];
	fmDrum_render(&umaFm[voiceNr], fmBuf, size);
	for(uint8_t i = 0; i < size; i++)
	{
		float s = fmBuf[i] * 32767.f;
		if(s > 32767.f) s = 32767.f;
		if(s < -32767.f) s = -32767.f;
		buf[i] = (int16_t)s;
	}
	return 1;
}

__attribute__((noinline)) uint8_t umaFmV_ownEnv(uint8_t voiceNr)
{
	return umaMd_isOn(voiceNr);
}
