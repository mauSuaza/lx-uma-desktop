/*
 * uma_mod.c - UMA: modulation of the UMA settings (see uma_mod.h).
 *
 * LX_UMA desktop (0.3a): the Eurorack's file (LX_UMA a179390). The UMA settings are one block here, PAR_UMA_ENG1 up
 * to PAR_UMA_END (the record), with the slots' amounts in it (slots 1-3: PAR_UMA_DSK_MS*); kit morph of UMA settings
 * comes in 0.3e (umaMod_knob: the knob value).
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "uma_mod.h"
#include "uma_quant.h"	// the pitch screen's ranges
#include "Soundengine/DSPAudio/PlaitsDrum.h"	// the Plaits engine's banks
#include "uma.h"

#if UMA_FM
#include "Parameters.h"
#include "menu/menu.h"
#include "menu/umaModTargets.h"
#include "uma_modmatrix.h"	// cvModulators
#include "Soundengine/modSlots.h"
#include "Soundengine/DSPAudio/modulationNode.h"
#include "Soundengine/DSPAudio/lfoShapes.h"
#include "Soundengine/DSPAudio/PrkDrum.h"
#include "Soundengine/DSPAudio/MdDrum.h"
#include "uma_turing.h"	// UMA_ENV_FIRST_SOURCE
#include "uma_voice.h"	// the snare's, cymbal's and hi-hat's engine settings

// every UMA setting of the record, PAR_UMA_ENG1 .. PAR_UMA_END
#define UMA_MOD_COUNT		(PAR_UMA_END - PAR_UMA_ENG1)
#define UMA_MOD_FRESH		2	// passes a modulated value stays valid without being set again


static volatile uint8_t umaMod_pass;
static uint8_t umaMod_value[UMA_MOD_COUNT];
static uint8_t umaMod_stamp[UMA_MOD_COUNT];	// pass the value was last set in
static uint8_t umaMod_valid[UMA_MOD_COUNT];	// 1 while a slot modulates it. Cleared when stale, so the
												// wrapping pass counter can never make an old stamp look new
static uint8_t umaMod_lastEngine[3] = {0xff, 0xff, 0xff};

void umaMod_newPass(void)
{
	umaMod_pass++;
	for(uint16_t i = 0; i < UMA_MOD_COUNT; i++)
		if(umaMod_valid[i] && (uint8_t)(umaMod_pass - umaMod_stamp[i]) > UMA_MOD_FRESH)
			umaMod_valid[i] = 0;
}

// a slot's amount (UMA 0.21: a target, so one slot can scale another's depth)
static uint8_t umaMod_isSlotAmount(uint16_t param)
{
	uint8_t field;
	return modSlot_of(param, &field) != 0xff && field == 1;
}

uint8_t umaMod_isTarget(uint16_t param)
{
	if(param < PAR_UMA_ENG1 || param >= PAR_UMA_END)
		return 0;
	uint8_t field;
	return modSlot_of(param, &field) == 0xff || field == 1;	// a slot's source and destination aren't targets
}

// where a target's modulated value is kept
static uint16_t umaMod_index(uint16_t param)
{
	return (uint16_t)(param - PAR_UMA_ENG1);
}

// the largest value of an FM Drum setting: the algorithm and ratios are lists, the rest 0-127
// the snare's, cymbal's and hi-hat's engine settings: the offset in the voice's block (UMA_VX_*), 0xff if not one
static uint8_t umaMod_vx(uint16_t p)
{
	return p >= UMA_VX_FIRST_PAR && p < UMA_VX_FIRST_PAR + 3 * UMA_VX_PARS ? (uint8_t)((p - UMA_VX_FIRST_PAR) % UMA_VX_PARS) : 0xff;
}

// `fil` (0.34d): one mod target per voice (the DX bank's place) for the file of the model the voice plays: the 6-op's
// bank, the wavetable's or the terrain's file. Its value is kept as the target's; its knob and range are the model's
static uint16_t umaMod_fil(uint16_t param)
{
	if(param < UMA_PLA_DXB_PAR || param >= UMA_PLA_DXB_PAR + 6) return param;
	const uint8_t v = (uint8_t)(param - UMA_PLA_DXB_PAR);
	const uint16_t bnk = (uint16_t)(UMA_PLA_FIRST_PAR + v * UMA_PLA_PARS);	// the voice's bank and model
	const uint8_t model = plaitsDrum_modelOf(activeParameters[bnk], activeParameters[bnk + 1]);
	return model == PL_MODEL_WAVETABLE ? (uint16_t)(UMA_PLA_WVF_PAR + v)
		: model == PL_MODEL_TERRAIN ? (uint16_t)(UMA_PLA_TRF_PAR + v) : param;
}

static uint8_t umaMod_max(uint16_t param)
{
	param = umaMod_fil(param);
	switch(umaMod_vx(param))
	{
	case UMA_VX_ENG:		return UMA_PLAITS ? 3 : 2;	// lxr prk mdk (pla)
	case UMA_VX_PRK_ALG:	return PRK_NUM_ALGS - 1;
	case UMA_VX_PRK_VAR:	return (uint8_t)(prkDrum_varCount(activeParameters[param - 1]) - 1);
	case UMA_VX_PRK_PAL:	return PRK_NUM_PALETTES - 1;
	case UMA_VX_MD_TYP:		return MD_NUM_TYPES - 1;
	case UMA_VX_MD_MCH:		return (uint8_t)(mdDrum_typeCount(activeParameters[param - 1]) - 1);
	case UMA_VX_MD_PAL:		return MD_NUM_PALETTES - 1;
	case UMA_VX_FOLD_TYPE:	return 1;
	case 0xff:				break;
	default:				return 127;
	}
	if(umaMod_isSlotAmount(param))
		return 100;	// a slot's amount is 0-100
	if(param >= UMA_PLA_DXB_PAR && param < UMA_PLA_DXB_PAR + 6)
		return (uint8_t)(plaitsDrum_dxBankCount() - 1);	// the 6-op's banks
	if(param >= UMA_PLA_KMD_PAR && param < UMA_PLA_KMD_PAR + 6)
		return PL_NUM_DXK - 1;	// the 6-op's key mode
	if(param >= UMA_PLA_WVF_PAR && param < UMA_PLA_WVF_PAR + 6)
		return (uint8_t)(plaitsDrum_userFileCount(PL_MODEL_WAVETABLE) - 1);	// built in, the card's wavetables
	if(param >= UMA_PLA_TRF_PAR && param < UMA_PLA_TRF_PAR + 6)
		return (uint8_t)(plaitsDrum_userFileCount(PL_MODEL_TERRAIN) - 1);	// built in, the card's terrains
	if(param >= UMA_PLA_SBS_PAR && param < UMA_PLA_SBO_PAR + 6)
		return 3;	// the Plaits sub's 4 shapes, 4 octaves
	if(param >= UMA_PLA_FIRST_PAR && param < UMA_PLA_FIRST_PAR + 6 * UMA_PLA_PARS)
	{
		const uint16_t k = (uint16_t)((param - UMA_PLA_FIRST_PAR) % UMA_PLA_PARS);	// Plaits: BNK MDL HRM TMB MOR OUT
		if(k == 0) return PL_NUM_BANKS - 1;
		if(k == 1) { const uint8_t n = plaitsDrum_bankCount(activeParameters[param - 1]); return n ? (uint8_t)(n - 1) : 0; }
		if(k == 5) return PL_NUM_OUTPUTS - 1;
		return 127;
	}
	if(param >= UMA_QUANT_FIRST_PAR && param < UMA_QUANT_FIRST_PAR + UMA_QUANT_VOICES * UMA_QUANT_PARS)
	{
		const uint16_t q = (uint16_t)((param - UMA_QUANT_FIRST_PAR) % UMA_QUANT_PARS);	// the pitch screen
		return q == 0 ? UMA_QUANT_OCT_MAX : q == 1 ? UMA_QUANT_KEYS - 1 : UMA_QUANT_SCALES - 1;
	}
	if(param >= UMA_LFOB_SYNC_PAR && param < UMA_LFOB_SYNC_PAR + 6)
		return LFOS_SYNC_RATES - 1;	// LFO B sync: off and 15 divisions
	if(param >= UMA_LFOB_FIRST_PAR && param < UMA_LFOB_FIRST_PAR + 6 * UMA_LFOB_PARS
		&& (param - UMA_LFOB_FIRST_PAR) % UMA_LFOB_PARS == 3)
		return 10;	// LFO B retrigger: off, v1-v6, every 1 2 4 8 bars
	if(param >= UMA_TONE_FIRST_PAR && param < UMA_TONE_FIRST_PAR + 3 * UMA_TONE_PARS)
	{
		const uint16_t t = (uint16_t)((param - UMA_TONE_FIRST_PAR) % UMA_TONE_PARS);
		if(t == PAR_UMA_RTC1 - PAR_UMA_RTC1 || t == PAR_UMA_RB2_1 - PAR_UMA_RTC1) return UMA_FM_RATIO_STEPS - 1;
		if(t == PAR_UMA_SBS1 - PAR_UMA_RTC1 || t == PAR_UMA_SBO1 - PAR_UMA_RTC1) return 3;	// 4 shapes, 4 octaves
		return 127;
	}
	if(param >= UMA_RPL_PAR && param < UMA_RPL_PAR + 3)
		return 3;	// all bas hrm met (FMD_NUM_PALETTES - 1)
	if(param >= UMA_PRK_PAL_PAR && param < UMA_PRK_PAL_PAR + 3)
		return PRK_NUM_PALETTES - 1;	// all prc drm
	if(param >= UMA_MD_PAL_PAR && param < UMA_MD_PAL_PAR + 3)
		return MD_NUM_PALETTES - 1;	// typ fam all
	if(param >= UMA_MD_FIRST_PAR && param < UMA_MD_FIRST_PAR + 3 * UMA_MD_PARS)
	{
		const uint16_t k = (uint16_t)((param - UMA_MD_FIRST_PAR) % UMA_MD_PARS);
		if(k == 0) return MD_NUM_TYPES - 1;
		if(k == 1) return (uint8_t)(mdDrum_typeCount(activeParameters[param - 1]) - 1);	// the type's machines
		return 127;
	}
	if(param >= UMA_PRK_FIRST_PAR && param < UMA_PRK_FIRST_PAR + 3 * UMA_PRK_PARS)
	{
		const uint16_t k = (uint16_t)((param - UMA_PRK_FIRST_PAR) % UMA_PRK_PARS);
		if(k == 0) return PRK_NUM_ALGS - 1;
		if(k == 1) return (uint8_t)(prkDrum_varCount(activeParameters[param - 1]) - 1);
		return 127;
	}
	const uint16_t k = (uint16_t)((param - PAR_UMA_ENG1) % UMA_FM_PARS_PER_DRUM);
	if(param >= PAR_UMA_ENG1 && param < UMA_RND_FIRST_PAR)
	{
		if(k == PAR_UMA_ENG1 - PAR_UMA_ENG1) return UMA_PLAITS ? 5 : 4;	// lxr fmd fmt prk mdk (pla)
		if(k == PAR_UMA_ALGO1 - PAR_UMA_ENG1)	// FM Drum 7 algorithms, FM Tone 8
			return activeParameters[param - (PAR_UMA_ALGO1 - PAR_UMA_ENG1)] == 2 ? 7 : 6;
		if(k == PAR_UMA_RATIO_A1 - PAR_UMA_ENG1 || k == PAR_UMA_RATIO_B1 - PAR_UMA_ENG1) return UMA_FM_RATIO_STEPS - 1;
	}
	return 127;
}

float umaMod_unipolar(uint8_t src, float cv)
{
	float u;
	if(src <= 4)
		u = 2.f * cv - 1.f;	// cv1-5: 0 V no change, +5 V the whole amount; negative voltages nothing
	else if(src >= UMA_ENV_FIRST_SOURCE && src < UMA_VEL_FIRST_SOURCE)
		u = src < UMA_ENV_FIRST_SOURCE + 6 ? 2.f * cv - 1.f : 2.f * cv;	// EN: the envelope; EI: 1 - envelope
	else
		u = cv;
	u = u < 0.f ? 0.f : u > 1.f ? 1.f : u;
	return 0.5f + 0.5f * u;
}

void umaMod_apply(uint16_t param, float amount, float val)
{
	if(!umaMod_isTarget(param))
		return;
	const float max = umaMod_max(param);
	float v = umaMod_knob(param) / max + amount * (val * 2.f - 1.f);	// around the (morphed) knob value
	v = v < 0.f ? 0.f : v > 1.f ? 1.f : v;
	const uint16_t i = umaMod_index(param);
	umaMod_value[i] = (uint8_t)(v * max + 0.5f);
	umaMod_stamp[i] = umaMod_pass;
	umaMod_valid[i] = 1;
}

uint8_t umaMod_knob(uint16_t param)
{
	return activeParameters[umaMod_fil(param)];	// `fil`: the file setting of the voice's model (morph: 0.3e)
}

uint8_t umaMod_isModulated(uint16_t param)
{
	if(!umaMod_isTarget(param))
		return 0;
	const uint16_t i = umaMod_index(param);
	return umaMod_valid[i] && (uint8_t)(umaMod_pass - umaMod_stamp[i]) <= UMA_MOD_FRESH;
}

uint8_t umaMod_get(uint16_t param)
{
	if(!umaMod_isTarget(param))
		return activeParameters[param];
	const uint16_t i = umaMod_index(param);
	if(umaMod_valid[i] && (uint8_t)(umaMod_pass - umaMod_stamp[i]) <= UMA_MOD_FRESH)
		return umaMod_value[i];
	return umaMod_knob(param);
}

void umaMod_kitLoaded(void)
{
	for(uint8_t s = 0; s < NUM_MOD_SLOTS; s++)
		modNode_setDestination(&cvModulators[s], umaTgt_slotParam(s));
	for(uint8_t d = 0; d < 3; d++)
		umaMod_lastEngine[d] = activeParameters[PAR_UMA_ENG1 + d * UMA_FM_PARS_PER_DRUM];
}

void umaMod_tick(void)
{
	for(uint8_t d = 0; d < 3; d++)
	{
		const uint8_t engine = activeParameters[PAR_UMA_ENG1 + d * UMA_FM_PARS_PER_DRUM];
		if(engine == umaMod_lastEngine[d])
			continue;
		umaMod_lastEngine[d] = engine;
		// the drum's slots keep their targets: the other engine's ones go quiet, this engine's come back
		for(uint8_t s = d * MOD_SLOTS_PER_VOICE; s < (d + 1) * MOD_SLOTS_PER_VOICE; s++)
			modNode_setDestination(&cvModulators[s], umaTgt_slotParam(s));
	}
}

#else
void umaMod_newPass(void) {}
uint8_t umaMod_isTarget(uint16_t param) { (void)param; return 0; }
void umaMod_apply(uint16_t param, float amount, float val) { (void)param; (void)amount; (void)val; }
float umaMod_unipolar(uint8_t src, float cv) { (void)src; return cv; }
uint8_t umaMod_get(uint16_t param) { (void)param; return 0; }
uint8_t umaMod_knob(uint16_t param) { (void)param; return 0; }
uint8_t umaMod_isModulated(uint16_t param) { (void)param; return 0; }
void umaMod_kitLoaded(void)
{
	for(uint8_t s = 0; s < NUM_MOD_SLOTS; s++)
		modNode_setDestination(&cvModulators[s], umaTgt_slotParam(s));
	for(uint8_t d = 0; d < 3; d++)
		umaMod_lastEngine[d] = activeParameters[PAR_UMA_ENG1 + d * UMA_FM_PARS_PER_DRUM];
}

void umaMod_tick(void) {}
void umaMod_kitLoaded(void) {}
#endif
