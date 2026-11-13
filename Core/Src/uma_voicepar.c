/*
 * uma_voicepar.c - UMA: where each voice's engine settings are, and its engine (uma_voice.h). Apart from the
 * voices' sound (uma_voice.c) so the menu, the targets, randomise and the Mac tests can use it alone.
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "uma_voice.h"
#include "uma.h"

#if UMA_FM
#include "Parameters.h"
#include "uma_mod.h"
extern uint8_t activeParameters[];

// --- where the settings are ---

static const uint8_t umaVoice_vxEngine[4] = {UMA_ENG_LXR, UMA_ENG_PRK, UMA_ENG_MDK, UMA_ENG_PLA};	// the stored 0-3

static uint8_t umaVoice_kind(uint8_t voice, uint8_t value)
{
#if UMA_PLAITS
	if(voice < 3) return value <= UMA_ENG_PLA ? value : UMA_ENG_LXR;
#else
	if(voice < 3) return value <= UMA_ENG_MDK ? value : UMA_ENG_LXR;	// pla: only in the Plaits test builds
#endif
#if UMA_PLAITS
	return value < 4 ? umaVoice_vxEngine[value] : UMA_ENG_LXR;
#else
	return value < 3 ? umaVoice_vxEngine[value] : UMA_ENG_LXR;	// pla: only in the Plaits test builds
#endif
}

static uint16_t umaVoice_engPar(uint8_t voice)
{
	return voice < 3 ? (uint16_t)(PAR_UMA_ENG1 + voice * UMA_FM_PARS_PER_DRUM) : UMA_VX_PAR(voice, UMA_VX_ENG);
}

uint8_t umaVoice_engine(uint8_t voice)
{
	return voice < UMA_VOICES ? umaVoice_kind(voice, umaMod_get(umaVoice_engPar(voice))) : UMA_ENG_LXR;
}

uint8_t umaVoice_engineKnob(uint8_t voice)
{
	return voice < UMA_VOICES ? umaVoice_kind(voice, activeParameters[umaVoice_engPar(voice)]) : UMA_ENG_LXR;
}

uint8_t umaVoice_ofEngine(uint16_t param)
{
	for(uint8_t v = 0; v < UMA_VOICES; v++)
		if(umaVoice_engPar(v) == param) return v;
	return 0xff;
}

uint16_t umaVoice_prkPar(uint8_t voice, uint8_t k)
{
	return voice < 3 ? (uint16_t)(UMA_PRK_FIRST_PAR + voice * UMA_PRK_PARS + k) : UMA_VX_PAR(voice, UMA_VX_PRK_ALG + k);
}
uint16_t umaVoice_prkPalPar(uint8_t voice)
{
	return voice < 3 ? (uint16_t)(UMA_PRK_PAL_PAR + voice) : UMA_VX_PAR(voice, UMA_VX_PRK_PAL);
}
uint16_t umaVoice_mdPar(uint8_t voice, uint8_t k)
{
	return voice < 3 ? (uint16_t)(UMA_MD_FIRST_PAR + voice * UMA_MD_PARS + k) : UMA_VX_PAR(voice, UMA_VX_MD_TYP + k);
}
uint16_t umaVoice_mdPalPar(uint8_t voice)
{
	return voice < 3 ? (uint16_t)(UMA_MD_PAL_PAR + voice) : UMA_VX_PAR(voice, UMA_VX_MD_PAL);
}
uint16_t umaVoice_foldPar(uint8_t voice, uint8_t k)
{
	return voice < 3 ? (uint16_t)(UMA_FOLD_FIRST_PAR + voice * UMA_FOLD_PARS + k) : UMA_VX_PAR(voice, UMA_VX_FOLD + k);
}
uint16_t umaVoice_fenvPar(uint8_t voice)
{
	return voice < 3 ? (uint16_t)(PAR_UMA_FENV1 + voice * UMA_ENV_PARS) : UMA_VX_PAR(voice, UMA_VX_FENV);
}

uint16_t umaVoice_plaPar(uint8_t voice, uint8_t k)
{
	if(voice >= UMA_VOICES) return PAR_NONE;
	if(k < UMA_PLA_PARS) return (uint16_t)(UMA_PLA_FIRST_PAR + voice * UMA_PLA_PARS + k);
	if(k == 6) return (uint16_t)(UMA_PLA_DXB_PAR + voice);	// `fil` (the 6-op's bank; the mod target of all three)
	if(k == 9) return (uint16_t)(UMA_PLA_WVF_PAR + voice);	// the wavetable's file
	if(k == 10) return (uint16_t)(UMA_PLA_TRF_PAR + voice);	// the terrain's file
	if(k >= 11 && k <= 14) return (uint16_t)(UMA_PLA_SUB_PAR + (k - 11) * UMA_VOICES + voice);	// the sub: sub sbs sbo sbf
	return k == 7 ? (uint16_t)(UMA_PLA_KMD_PAR + voice) : (uint16_t)(UMA_PLA_TWI_PAR + voice);
}

uint8_t umaVoice_which(uint16_t p, uint8_t* voice, uint8_t* k)
{
	if(p >= UMA_PLA_FIRST_PAR && p < UMA_PLA_FIRST_PAR + UMA_VOICES * UMA_PLA_PARS)
	{
		*voice = (uint8_t)((p - UMA_PLA_FIRST_PAR) / UMA_PLA_PARS);
		*k = (uint8_t)((p - UMA_PLA_FIRST_PAR) % UMA_PLA_PARS);
		return 5;
	}
	if(p >= UMA_PLA_DXB_PAR && p < UMA_PLA_DXB_PAR + UMA_VOICES)
	{
		*voice = (uint8_t)(p - UMA_PLA_DXB_PAR);
		*k = 6;	// the 6-op's bank
		return 5;
	}
	if(p >= UMA_PLA_WVF_PAR && p < UMA_PLA_WVF_PAR + UMA_VOICES)
	{
		*voice = (uint8_t)(p - UMA_PLA_WVF_PAR);
		*k = 9;	// the wavetable's file
		return 5;
	}
	if(p >= UMA_PLA_TRF_PAR && p < UMA_PLA_TRF_PAR + UMA_VOICES)
	{
		*voice = (uint8_t)(p - UMA_PLA_TRF_PAR);
		*k = 10;	// the terrain's file
		return 5;
	}
	if(p >= UMA_PLA_SUB_PAR && p < UMA_PLA_SUB_PAR + 4 * UMA_VOICES)
	{
		*voice = (uint8_t)((p - UMA_PLA_SUB_PAR) % UMA_VOICES);
		*k = (uint8_t)(11 + (p - UMA_PLA_SUB_PAR) / UMA_VOICES);	// the sub: sub sbs sbo sbf
		return 5;
	}
	if(p >= UMA_PLA_KMD_PAR && p < UMA_PLA_KMD_PAR + UMA_VOICES)
	{
		*voice = (uint8_t)(p - UMA_PLA_KMD_PAR);
		*k = 7;	// the 6-op's key mode
		return 5;
	}
	if(p >= UMA_PLA_TWI_PAR && p < UMA_PLA_TWI_PAR + UMA_VOICES)
	{
		*voice = (uint8_t)(p - UMA_PLA_TWI_PAR);
		*k = 8;	// TWIST
		return 5;
	}
	for(uint8_t v = 0; v < UMA_VOICES; v++)
	{
		if(p >= umaVoice_prkPar(v, 0) && p <= umaVoice_prkPar(v, 3)) { *voice = v; *k = (uint8_t)(p - umaVoice_prkPar(v, 0)); return 1; }
		if(p == umaVoice_prkPalPar(v)) { *voice = v; *k = 0; return 2; }
		if(p >= umaVoice_mdPar(v, 0) && p <= umaVoice_mdPar(v, 9)) { *voice = v; *k = (uint8_t)(p - umaVoice_mdPar(v, 0)); return 3; }
		if(p == umaVoice_mdPalPar(v)) { *voice = v; *k = 0; return 4; }
	}
	return 0;
}

uint16_t umaVoice_ampPar(uint8_t voice, uint8_t which)
{
	static const uint16_t atk[UMA_VOICES] = {PAR_VELOA1, PAR_VELOA2, PAR_VELOA3, PAR_VELOA4, PAR_VELOA5, PAR_VELOA6};
	static const uint16_t dec[UMA_VOICES] = {PAR_VELOD1, PAR_VELOD2, PAR_VELOD3, PAR_VELOD4, PAR_VELOD5, PAR_VELOD6_CLOSED};
	static const uint16_t slope[UMA_VOICES] = {PAR_VOL_SLOPE1, PAR_VOL_SLOPE2, PAR_VOL_SLOPE3, PAR_VOL_SLOPE4, PAR_VOL_SLOPE5, PAR_VOL_SLOPE6};
	if(voice >= UMA_VOICES) return PAR_NONE;
	switch(which)
	{
	case 0: return atk[voice];
	case 1: return dec[voice];
	case 2: return slope[voice];
	default: return voice == 5 ? PAR_VELOD6_OPEN : PAR_NONE;
	}
}

#endif
