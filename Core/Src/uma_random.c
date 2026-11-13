/*
 * uma_random.c - UMA randomise for the FM Drum engine (docs/design/fm-drum-engine.md section 5).
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "uma_random.h"
#include "uma.h"

#if UMA_FM
#include "Parameters.h"
#include "menu/menu.h"
#include "frontPanelParser_ui.h"
#include "Soundengine/MIDI/MidiMessages.h"
#include "Soundengine/DSPAudio/FmDrum.h"
#include "Soundengine/DSPAudio/PrkDrum.h"
#include "Soundengine/DSPAudio/MdDrum.h"
#include "uma_voice.h"
#include "uma_mod.h"

#define UMA_RND_DRUMS	3	// Drum 1-3 (the FM engines)
#define UMA_RND_VOICES	6	// every voice: prk and mdk randomise on the snare, cymbal and hi-hat too (UMA 0.29)
#define UMA_RND_SEED	FMD_NUM_RND_GROUPS	// index of the seed among a drum's randomise settings
#define UMA_RND_PAL	(FMD_NUM_RND_GROUPS + 1)	// ... and of the ratio palette (UMA 0.28)
#define UMA_RND_PPAL	(FMD_NUM_RND_GROUPS + 2)	// ... and of prk's palette (UMA 0.29)
#define UMA_RND_MPAL	(FMD_NUM_RND_GROUPS + 3)	// ... and of the drum-machine engine's palette (UMA 0.29)
#define UMA_RND_INDICES	(FMD_NUM_RND_GROUPS + 4)	// the groups (tone env sweep fine), the seed, the palettes
#define UMA_RND_COPY_MAX	(FMD_RND_MAX_VALUES + PRK_RND_MAX_VALUES + MD_CONTROLS)	// what any engine's draw sets

// a value of a kit parameter
typedef struct
{
	uint16_t param;
	uint8_t value;
} UmaRndValue;

// restore copy of each group per drum, and the randomise values last seen
static UmaRndValue umaRnd_copy[UMA_RND_VOICES][FMD_NUM_RND_GROUPS][UMA_RND_COPY_MAX];
static uint8_t umaRnd_copyCount[UMA_RND_VOICES][FMD_NUM_RND_GROUPS];
static uint8_t umaRnd_last[UMA_RND_VOICES][UMA_RND_INDICES];
// the randomise values in effect: the knob value, or while modulated the modulated value taken at the last hit
static uint8_t umaRnd_held[UMA_RND_VOICES][UMA_RND_INDICES];
// what the draw depends on besides the values and seed: the palette, and with one the engine (umaRnd_drawKey)
static uint8_t umaRnd_lastKey[UMA_RND_VOICES];
// the drum-machine engine: the machine rmc drew (0xff: none, the drum plays the machine its settings choose)
static uint8_t umaRnd_mdDrawn[UMA_RND_VOICES] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

// the kit parameter of a drum's randomise setting: tone env sweep and seed are the PAR_UMA_RND_* block, fine
// (UMA 0.20) is after the fine ratios at the kit's end
static uint16_t umaRnd_par(uint8_t drum, uint8_t index)
{
	if(drum >= UMA_RND_DRUMS)	// the snare, cymbal, hi-hat: rto rev rsd and the two palettes (uma_voice.h)
	{
		if(index == FMD_RND_TONE) return UMA_VX_PAR(drum, UMA_VX_RND_TONE);
		if(index == FMD_RND_ENV) return UMA_VX_PAR(drum, UMA_VX_RND_ENV);
		if(index == UMA_RND_SEED) return UMA_VX_PAR(drum, UMA_VX_RND_SEED);
		if(index == UMA_RND_PPAL) return UMA_VX_PAR(drum, UMA_VX_PRK_PAL);
		if(index == UMA_RND_MPAL) return UMA_VX_PAR(drum, UMA_VX_MD_PAL);
		return PAR_NONE;	// sweep, fine, the ratio palette: FM only
	}
	if(index == FMD_RND_FINE)
		return (uint16_t)(PAR_UMA_RND_FINE1 + drum);
	if(index == UMA_RND_SEED)
		return (uint16_t)(PAR_UMA_RND_SEED1 + drum * UMA_RND_PARS_PER_DRUM);
	if(index == UMA_RND_PAL)
		return (uint16_t)(UMA_RPL_PAR + drum);
	if(index == UMA_RND_PPAL)
		return (uint16_t)(UMA_PRK_PAL_PAR + drum);
	if(index == UMA_RND_MPAL)
		return (uint16_t)(UMA_MD_PAL_PAR + drum);
	return (uint16_t)(UMA_RND_FIRST_PAR + drum * UMA_RND_PARS_PER_DRUM + index);
}
static volatile uint8_t umaRnd_hitFlag[UMA_RND_VOICES];

static uint8_t umaRnd_get(uint8_t drum, uint8_t index)
{
	return umaRnd_held[drum][index];
}

static uint8_t umaRnd_palette(uint8_t drum)
{
	const uint8_t p = umaRnd_get(drum, UMA_RND_PAL);
	return p < FMD_NUM_PALETTES ? p : FMD_PAL_ALL;
}

// 1 if the drum plays FM Tone (the palettes draw for it differently)
static uint8_t umaRnd_tone(uint8_t drum)
{
	return umaVoice_engine(drum) == UMA_ENG_FMT;
}

// 1 if the drum plays the prk engine: randomise draws its algorithm, variation, p1, p2 instead
static uint8_t umaRnd_prk(uint8_t drum)
{
	return umaVoice_engine(drum) == UMA_ENG_PRK;
}

// 1 if the drum plays the drum-machine engine: rto draws a machine (rmc), rev its knobs (rkn)
static uint8_t umaRnd_md(uint8_t drum)
{
	return umaVoice_engine(drum) == UMA_ENG_MDK;
}

// 1 for the FM engines' randomise (Drum 1-3 on fmd or fmt)
static uint8_t umaRnd_fm(uint8_t drum)
{
	const uint8_t e = umaVoice_engine(drum);
	return drum < UMA_RND_DRUMS && (e == UMA_ENG_FMD || e == UMA_ENG_FMT);
}

// the machine the drum's settings choose (the home of rmc's palette, and what rkn draws the knobs of)
static uint8_t umaRnd_mdHome(uint8_t drum)
{
	return mdDrum_machineOf(activeParameters[umaVoice_mdPar(drum, 0)], activeParameters[umaVoice_mdPar(drum, 1)]);
}

uint8_t umaRnd_mdMachine(uint8_t drum)
{
	return drum < UMA_RND_VOICES ? umaRnd_mdDrawn[drum] : 0xff;
}

// all draws the same on both FM engines; the other palettes don't; prk and mdk draw their own, by their palette
static uint8_t umaRnd_drawKey(uint8_t drum)
{
	if(umaRnd_md(drum))
	{
		const uint8_t mp = umaRnd_get(drum, UMA_RND_MPAL);
		return (uint8_t)(0x80 | (mp < MD_NUM_PALETTES ? mp : 0) | (umaRnd_mdHome(drum) << 2));
	}
	if(umaRnd_prk(drum))
	{
		const uint8_t pp = umaRnd_get(drum, UMA_RND_PPAL);
		return (uint8_t)(0x40 | (pp < PRK_NUM_PALETTES ? pp : 0));
	}
	if(!umaRnd_fm(drum))
		return 0;	// lxr: nothing to draw
	const uint8_t p = umaRnd_palette(drum);
	return p == FMD_PAL_ALL ? 0 : (uint8_t)(p | umaRnd_tone(drum) << 2);
}

// update the values in effect for one drum: knob changes at once, modulation only on a hit, so CV or an
// LFO re-rolls once per hit and never mid-hit
static void umaRnd_updateHeld(uint8_t drum, uint8_t hit)
{
	for(uint8_t i = 0; i < UMA_RND_INDICES; i++)
	{
		const uint16_t p = umaRnd_par(drum, i);
		if(p == PAR_NONE)
			umaRnd_held[drum][i] = 0;
		else if(!umaMod_isModulated(p))
			umaRnd_held[drum][i] = activeParameters[p];
		else if(hit)
			umaRnd_held[drum][i] = umaMod_get(p);
	}
}

// the kit parameter an FmDrum parameter of this drum is stored in. The sweep is the voice's own pitch EG
static uint16_t umaRnd_kitParam(uint8_t drum, uint8_t fmdParam)
{
	static const uint16_t sweepDec[UMA_RND_DRUMS] = {PAR_MOD_EG1, PAR_MOD_EG2, PAR_MOD_EG3};
	static const uint16_t sweepAmt[UMA_RND_DRUMS] = {PAR_MODAMNT1, PAR_MODAMNT2, PAR_MODAMNT3};
	static const uint16_t sweepSlope[UMA_RND_DRUMS] = {PAR_PITCH_SLOPE1, PAR_PITCH_SLOPE2, PAR_PITCH_SLOPE3};
	const uint16_t fm = PAR_UMA_ENG1 + drum * UMA_FM_PARS_PER_DRUM;
	switch(fmdParam)
	{
	case FMD_ALGO:			return fm + (PAR_UMA_ALGO1 - PAR_UMA_ENG1);
	case FMD_FEEDBACK:		return fm + (PAR_UMA_FDBK1 - PAR_UMA_ENG1);
	case FMD_RATIO_A:		return fm + (PAR_UMA_RATIO_A1 - PAR_UMA_ENG1);
	case FMD_RATIO_B:		return fm + (PAR_UMA_RATIO_B1 - PAR_UMA_ENG1);
	case FMD_MOD_A:			return fm + (PAR_UMA_MOD_A1 - PAR_UMA_ENG1);
	case FMD_ATK_A:			return fm + (PAR_UMA_ATK_A1 - PAR_UMA_ENG1);
	case FMD_DEC_A:			return fm + (PAR_UMA_DEC_A1 - PAR_UMA_ENG1);
	case FMD_END_A:			return fm + (PAR_UMA_END_A1 - PAR_UMA_ENG1);
	case FMD_MOD_B:			return fm + (PAR_UMA_MOD_B1 - PAR_UMA_ENG1);
	case FMD_ATK_B:			return fm + (PAR_UMA_ATK_B1 - PAR_UMA_ENG1);
	case FMD_DEC_B:			return fm + (PAR_UMA_DEC_B1 - PAR_UMA_ENG1);
	case FMD_END_B:			return fm + (PAR_UMA_END_B1 - PAR_UMA_ENG1);
	case FMD_FINE_A:		return (uint16_t)(PAR_UMA_FINE_A1 + drum * 2);
	case FMD_FINE_B:		return (uint16_t)(PAR_UMA_FINE_B1 + drum * 2);
	case FMD_RATIO_C:		return (uint16_t)(PAR_UMA_RTC1 + drum * UMA_TONE_PARS);
	case FMD_RATIO_B2:		return (uint16_t)(PAR_UMA_RB2_1 + drum * UMA_TONE_PARS);
	case FMD_MOD_B2:		return (uint16_t)(PAR_UMA_M2_1 + drum * UMA_TONE_PARS);
	case FMD_ATK_B2:		return (uint16_t)(PAR_UMA_ATK_B2_1 + drum * UMA_B2ENV_PARS);
	case FMD_DEC_B2:		return (uint16_t)(PAR_UMA_DEC_B2_1 + drum * UMA_B2ENV_PARS);
	case FMD_END_B2:		return (uint16_t)(PAR_UMA_END_B2_1 + drum * UMA_B2ENV_PARS);
	case FMD_SWEEP_DEC:		return sweepDec[drum];
	case FMD_SWEEP_AMT:		return sweepAmt[drum];
	case FMD_SWEEP_SLOPE:	return sweepSlope[drum];
	default:				return PAR_NONE;
	}
}

// set a kit parameter and pass it on to the sound engine the way the menu does. The FM settings are read
// from activeParameters by the engine (DrumVoice.c); the sweep is a voice parameter below 128
static void umaRnd_setParam(uint16_t param, uint8_t value)
{
	if(param == PAR_NONE)
		return;
	activeParameters[param] = value;
	if(param < 128)
		frontPanel_sendData(MIDI_CC, (uint8_t)param, value);
	else if(param < 256)
		frontPanel_sendData(CC_2, (uint8_t)(param - 128), value);
}

// the kit parameter a prk draw sets
static uint16_t umaRnd_prkParam(uint8_t drum, uint8_t target)
{
	switch(target)
	{
	case PRK_RV_ALG: case PRK_RV_VAR: case PRK_RV_P1: case PRK_RV_P2:
		return umaVoice_prkPar(drum, (uint8_t)(target - PRK_RV_ALG));
	case PRK_RV_ATK:			return umaVoice_ampPar(drum, 0);
	case PRK_RV_DEC:			return umaVoice_ampPar(drum, 1);
	case PRK_RV_SLOPE:			return umaVoice_ampPar(drum, 2);
	// the pitch sweep: Drum 1-3's own (the other voices have none to draw)
	case PRK_RV_SWEEP_AMT:		return drum < UMA_RND_DRUMS ? umaRnd_kitParam(drum, FMD_SWEEP_AMT) : PAR_NONE;
	case PRK_RV_SWEEP_DEC:		return drum < UMA_RND_DRUMS ? umaRnd_kitParam(drum, FMD_SWEEP_DEC) : PAR_NONE;
	case PRK_RV_SWEEP_SLOPE:	return drum < UMA_RND_DRUMS ? umaRnd_kitParam(drum, FMD_SWEEP_SLOPE) : PAR_NONE;
	default:					return PAR_NONE;
	}
}

static void umaRnd_takeCopy(uint8_t drum, uint8_t group)
{
	FmDrumRandomValue v[FMD_RND_MAX_VALUES];
	// for the group's parameter list: the longest FM draw (FM Tone with hrm) holds every one any palette sets
	// (Drum 1-3); then what prk's and mdk's draws set that isn't there yet (fine is first, in the FM draw's order,
	// umaRnd_apply)
	const uint8_t n = drum < UMA_RND_DRUMS ? fmDrum_randomPal(group, 0, 1, drum, 0.f, FMD_PAL_HRM, 1, v) : 0;
	uint8_t c = 0;
	for(uint8_t i = 0; i < n; i++)
	{
		const uint16_t p = umaRnd_kitParam(drum, v[i].param);
		umaRnd_copy[drum][group][c++] = (UmaRndValue){p, activeParameters[p]};
	}
	if(group == FMD_RND_ENV)	// the drum-machine engine's knobs (rkn)
		for(uint8_t k = 0; k < MD_CONTROLS && c < UMA_RND_COPY_MAX; k++)
		{
			const uint16_t p = umaVoice_mdPar(drum, (uint8_t)(2 + k));
			umaRnd_copy[drum][group][c++] = (UmaRndValue){p, activeParameters[p]};
		}
	if(group == FMD_RND_ENV && umaVoice_ampPar(drum, 3) != PAR_NONE && c < UMA_RND_COPY_MAX)	// the hi-hat's open decay
		umaRnd_copy[drum][group][c++] = (UmaRndValue){umaVoice_ampPar(drum, 3), activeParameters[umaVoice_ampPar(drum, 3)]};
	PrkRandomValue pv[PRK_RND_MAX_VALUES];
	const uint8_t m = prkDrum_random(group, 0, 1, drum, PRK_PAL_ALL, pv);
	for(uint8_t i = 0; i < m && c < UMA_RND_COPY_MAX; i++)
	{
		const uint16_t p = umaRnd_prkParam(drum, pv[i].target);
		uint8_t seen = 0;
		for(uint8_t j = 0; j < c; j++)
			if(umaRnd_copy[drum][group][j].param == p) seen = 1;
		if(!seen && p != PAR_NONE)
			umaRnd_copy[drum][group][c++] = (UmaRndValue){p, activeParameters[p]};
	}
	umaRnd_copyCount[drum][group] = c;
}

static void umaRnd_restore(uint8_t drum, uint8_t group)
{
	if(group == FMD_RND_TONE)
		umaRnd_mdDrawn[drum] = 0xff;	// the drum-machine engine: back to the machine its settings choose
	for(uint8_t i = 0; i < umaRnd_copyCount[drum][group]; i++)
		umaRnd_setParam(umaRnd_copy[drum][group][i].param, umaRnd_copy[drum][group][i].value);
}

// the drum's amp EG decay in seconds: SlopeEg2 falls from 1 to 0 by slopeEg2_calcTime(value, TIME_K_DECAY)
// once per audio block (OUTPUT_DMA_SIZE samples at about 44.1 kHz); 0 if it never ends
static float umaRnd_ampDecaySeconds(uint8_t drum)
{
	const float k = 2.f * 0.999f / (1.f - 0.999f);	// TIME_K_DECAY
	const float v = activeParameters[umaVoice_ampPar(drum, 1)] / 127.f;
	float step = (1.f - ((1.f + k) * v / (1.f + k * v))) * 2.f;
	if(step > 1.f) step = 1.f;
	return step > 0.f ? (1.f / step) * 64.f / 44140.f : 0.f;
}

static void umaRnd_apply(uint8_t drum, uint8_t group)
{
	FmDrumRandomValue v[FMD_RND_MAX_VALUES];
	float length = umaRnd_ampDecaySeconds(drum);
	if(length <= 0.f) length = 5.f;	// never ends
	// with the hold before the decay (UMA 0.22, uma_env.c: up to 2 s on a squared curve)
	const float hold = drum < UMA_RND_DRUMS ? activeParameters[PAR_UMA_HOLD1 + drum * UMA_ENV_PARS] / 127.f : 0.f;
	length += hold * hold * 2.f;
	const uint8_t value = umaRnd_get(drum, group);
	if(umaRnd_md(drum))
	{
		const uint8_t seed = umaRnd_get(drum, UMA_RND_SEED), home = umaRnd_mdHome(drum);
		if(group == FMD_RND_TONE)	// rmc: which machine plays (the settings stay)
			umaRnd_mdDrawn[drum] = mdDrum_randomMachine(seed, value, drum, umaRnd_get(drum, UMA_RND_MPAL), home);
		else if(group == FMD_RND_ENV)	// rkn: the home machine's 8 knobs, as if turned by hand
		{
			uint8_t knobs[MD_CONTROLS];
			mdDrum_randomKnobs(seed, value, drum, home, knobs);
			for(uint8_t k = 0; k < MD_CONTROLS; k++)
				umaRnd_setParam(umaVoice_mdPar(drum, (uint8_t)(2 + k)), knobs[k]);
		}
		return;
	}
	if(umaRnd_prk(drum))
	{
		PrkRandomValue pv[PRK_RND_MAX_VALUES];
		const uint8_t m = prkDrum_random(group, umaRnd_get(drum, UMA_RND_SEED), value, drum,
			umaRnd_get(drum, UMA_RND_PPAL), pv);
		for(uint8_t i = 0; i < m; i++)
		{
			umaRnd_setParam(umaRnd_prkParam(drum, pv[i].target), pv[i].value);
			if(pv[i].target == PRK_RV_DEC && umaVoice_ampPar(drum, 3) != PAR_NONE)	// the hi-hat: open 30 longer
				umaRnd_setParam(umaVoice_ampPar(drum, 3), (uint8_t)(pv[i].value > 97 ? 127 : pv[i].value + 30));
		}
		return;
	}
	if(!umaRnd_fm(drum))
		return;	// lxr: nothing to draw
	const uint8_t palette = umaRnd_palette(drum);
	const uint8_t n = fmDrum_randomPal(group, umaRnd_get(drum, UMA_RND_SEED), value, drum, length, palette,
		umaRnd_tone(drum), v);
	for(uint8_t i = 0; i < n; i++)
	{
		uint8_t out = v[i].value;	// ratios: the table index, as in the kit
		if(group == FMD_RND_FINE)
		{
			// 1-63 (bas, hrm: any value): only metallic ratios take it, the others get the drum's own fine back
			// (the restore copy)
			const uint16_t ratio = umaRnd_kitParam(drum, v[i].param == FMD_FINE_A ? FMD_RATIO_A : FMD_RATIO_B);
			if(!fmDrum_randomFineAppliesPal(value, activeParameters[ratio], palette))
				out = i < umaRnd_copyCount[drum][group] ? umaRnd_copy[drum][group][i].value : 63;
		}
		umaRnd_setParam(umaRnd_kitParam(drum, v[i].param), out);
	}
}

void umaRnd_hit(uint8_t drum)
{
	if(drum < UMA_RND_VOICES)
		umaRnd_hitFlag[drum] = 1;
}

void umaRnd_block(void)
{
	for(uint8_t d = 0; d < UMA_RND_VOICES; d++)
	{
		const uint8_t hit = umaRnd_hitFlag[d];
		umaRnd_hitFlag[d] = 0;
		umaRnd_updateHeld(d, hit);

		const uint8_t seed = umaRnd_get(d, UMA_RND_SEED);
		const uint8_t key = umaRnd_drawKey(d);
		// a new seed, palette, or engine with a palette: every group in use draws again
		const uint8_t keyChanged = key != umaRnd_lastKey[d];
		const uint8_t seedChanged = seed != umaRnd_last[d][UMA_RND_SEED] || keyChanged;
		umaRnd_last[d][UMA_RND_SEED] = seed;
		umaRnd_lastKey[d] = key;

		uint8_t ratiosChanged = 0;
		for(uint8_t g = 0; g < FMD_NUM_RND_GROUPS; g++)
		{
			const uint8_t value = umaRnd_get(d, g), old = umaRnd_last[d][g];
			// random fine depends on the ratios (1-63: metallic ones only): work it out again when tone changed them
			const uint8_t refine = g == FMD_RND_FINE && value && ratiosChanged;
			if(value == old && !(seedChanged && value) && !refine)
				continue;
			umaRnd_last[d][g] = value;
			if(g == FMD_RND_TONE)
				ratiosChanged = 1;
			if(value == 0)
			{
				if(old != 0)
					umaRnd_restore(d, g);
				continue;
			}
			if(old == 0)
				umaRnd_takeCopy(d, g);
			else if(keyChanged)
				umaRnd_restore(d, g);	// what only the old palette set (B2, C's ratio) goes back to the drum's own
			umaRnd_apply(d, g);
		}
	}
}

// kit load and save run in the main loop, which the audio block (umaRnd_block) can interrupt: keep it out
// while the restore copies change
void umaRnd_kitLoaded(void)
{
	const uint32_t primask = __get_PRIMASK();
	__disable_irq();
	for(uint8_t d = 0; d < UMA_RND_VOICES; d++)
	{
		for(uint8_t i = 0; i < UMA_RND_INDICES; i++)
			umaRnd_held[d][i] = umaRnd_last[d][i] = activeParameters[umaRnd_par(d, i)];
		umaRnd_hitFlag[d] = 0;
		umaRnd_lastKey[d] = umaRnd_drawKey(d);
		umaRnd_mdDrawn[d] = 0xff;
		for(uint8_t g = 0; g < FMD_NUM_RND_GROUPS; g++)
			umaRnd_takeCopy(d, g);
	}
	__set_PRIMASK(primask);
}

void umaRnd_kitSaved(void)
{
	const uint32_t primask = __get_PRIMASK();
	__disable_irq();
	for(uint8_t d = 0; d < UMA_RND_VOICES; d++)
		for(uint8_t g = 0; g < FMD_NUM_RND_GROUPS; g++)
			umaRnd_takeCopy(d, g);
	__set_PRIMASK(primask);
}

#else
void umaRnd_block(void) {}
void umaRnd_hit(uint8_t drum) { (void)drum; }
void umaRnd_kitLoaded(void) {}
void umaRnd_kitSaved(void) {}
uint8_t umaRnd_mdMachine(uint8_t drum) { (void)drum; return 0xff; }
#endif
