/*
 * umaModTargets.c - UMA: the mod matrix targets of each voice (see umaModTargets.h).
 *
 * LX_UMA desktop (0.3a): the Eurorack's lists (LX_UMA a179390), entry for entry, so a slot's destination means the same
 * on both firmwares. The snare's, cymbal's and hi-hat's lists are the Eurorack's blocks of its shared list (embedded
 * here: the desktop's shared list lacks the 1V/oct and morph entries). v/o is the desktop's PAR_UMA_DSK_VOCT; the
 * entries the desktop doesn't have (velocity `vel`, the operator waveforms, morph until 0.3e) are kept but never
 * active, so the menu skips them.
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "menu/umaModTargets.h"
#include "uma.h"
#include "Parameters.h"
#include "menu/menu.h"
#include "Soundengine/modSlots.h"
#include "uma_voice.h"
#include "uma_quant.h"
#include "uma_turing.h"	// the sources' order
#include <stdio.h>

typedef struct
{
	uint16_t nameIdx;
	uint16_t param;
	uint8_t engine;	// UMA_TGT_*
} UmaModTarget;

#define A UMA_TGT_ALL
#define L UMA_TGT_LXR
#define F UMA_TGT_FM
#define FP (UMA_TGT_FM | UMA_TGT_PRK)	// randomise: the FM engines and prk
#define FPM (UMA_TGT_FM | UMA_TGT_PRK | UMA_TGT_MD | UMA_TGT_PLA)	// the wavefolder: every engine but lxr

// Drum 1-3, in page order: the LXR's targets and the FM Drum's, each once (docs/design/mod-targets.md)
#define UMA_DRUM_TARGETS(n) \
	{TEXT_EMPTY, PAR_NONE, A}, \
	/* Osc */ \
	{TEXT_1VOCT, PAR_UMA_DSK_VOCT##n, A}, \
	{TEXT_COARSE, PAR_COARSE##n, A}, \
	{TEXT_FINE, PAR_FINE##n, A}, \
	{TEXT_WAVEFORM, PAR_OSC_WAVE_DRUM##n, L}, \
	{TEXT_PWM, PAR_PWM_##n, L}, \
	{TEXT_UMA_T_ALGO, PAR_UMA_ALGO##n, F}, \
	{TEXT_UMA_T_RATIO_A, PAR_UMA_RATIO_A##n, F}, \
	{TEXT_UMA_FINE_A, PAR_UMA_FINE_A##n, F}, \
	{TEXT_UMA_T_RATIO_B, PAR_UMA_RATIO_B##n, F}, \
	{TEXT_UMA_FINE_B, PAR_UMA_FINE_B##n, F}, \
	/* Amp EG */ \
	{TEXT_ATTACK, PAR_VELOA##n, A}, \
	{TEXT_DECAY, PAR_VELOD##n, A}, \
	{TEXT_EG_SLOPE, PAR_VOL_SLOPE##n, A}, \
	{TEXT_UMA_HOLD, PAR_UMA_HOLD##n, A}, \
	{TEXT_UMA_SUSTAIN, PAR_UMA_SUS##n, A}, \
	{TEXT_UMA_RELEASE, PAR_UMA_REL##n, A}, \
	/* Mod: the pitch sweep, velocity */ \
	{TEXT_PITCH_DECAY, PAR_MOD_EG##n, A}, \
	{TEXT_EG_SLOPE, PAR_PITCH_SLOPE##n, A}, \
	{TEXT_MOD_AMOUNT, PAR_MODAMNT##n, A}, \
	{TEXT_VEL_DEST, PAR_VEL_DEST_##n, L}, \
	{TEXT_VEL_AMT, PAR_VELO_MOD_AMT_##n, L}, \
	{TEXT_VEL_MOD_VOL, PAR_VOLUME_MOD_ON_OFF##n, A}, \
	/* randomise */ \
	{TEXT_UMA_T_RND_TONE, PAR_UMA_RND_TONE##n, FPM}, \
	{TEXT_UMA_T_RND_FINE, PAR_UMA_RND_FINE##n, F}, \
	{TEXT_UMA_T_RND_ENV, PAR_UMA_RND_ENV##n, FPM}, \
	{TEXT_UMA_T_RND_SWEEP, PAR_UMA_RND_SWEEP##n, FP}, \
	{TEXT_UMA_T_RND_SEED, PAR_UMA_RND_SEED##n, FPM}, \
	/* FM: the LXR's two-oscillator FM, the FM Drum's operators */ \
	{TEXT_FM_AMOUNT, PAR_FMAMNT##n, L}, \
	{TEXT_FM_FREQ, PAR_FM_FREQ##n, L}, \
	{TEXT_WAVEFORM, PAR_MOD_WAVE_DRUM##n, L}, \
	{TEXT_MODE, PAR_MIX_MOD_##n, L}, \
	{TEXT_UMA_T_MOD_A, PAR_UMA_MOD_A##n, F}, \
	{TEXT_UMA_T_ATK_A, PAR_UMA_ATK_A##n, F}, \
	{TEXT_UMA_T_DEC_A, PAR_UMA_DEC_A##n, F}, \
	{TEXT_UMA_T_END_A, PAR_UMA_END_A##n, F}, \
	{TEXT_UMA_T_MOD_B, PAR_UMA_MOD_B##n, F}, \
	{TEXT_UMA_T_ATK_B, PAR_UMA_ATK_B##n, F}, \
	{TEXT_UMA_T_DEC_B, PAR_UMA_DEC_B##n, F}, \
	{TEXT_UMA_T_END_B, PAR_UMA_END_B##n, F}, \
	{TEXT_UMA_T_FEEDBACK, PAR_UMA_FDBK##n, F}, \
	{TEXT_UMA_WAVE_C, PAR_UMA_WAVE_C##n, 0 /* no operator waveforms on the desktop */}, \
	{TEXT_UMA_WAVE_AB, PAR_UMA_WAVE_AB##n, 0}, \
	/* Transient */ \
	{TEXT_TRANSIENT_WAVE, PAR_TRANS##n##_WAVE, A}, \
	{TEXT_TRANSIENT_VOLUME, PAR_TRANS##n##_VOL, A}, \
	{TEXT_TRANSIENT_FREQ, PAR_TRANS##n##_FREQ, A}, \
	/* the FM body: wavefolder and level */ \
	{TEXT_UMA_FOLD, PAR_UMA_FOLD##n, FPM}, \
	{TEXT_UMA_FOLD_TYPE, PAR_UMA_FOLD_TYPE##n, FPM}, \
	{TEXT_UMA_FOLD_SYM, PAR_UMA_FOLD_SYM##n, FPM}, \
	{TEXT_UMA_BODY_LEV, PAR_UMA_BODY_LEV##n, FPM}, \
	/* Filter */ \
	{TEXT_FILTER_F, PAR_FILTER_FREQ_##n, A}, \
	{TEXT_FILTER_RESO, PAR_RESO_##n, A}, \
	{TEXT_FILTER_TYPE, PAR_FILTER_TYPE_##n, A}, \
	{TEXT_FILTER_DRIVE, PAR_FILTER_DRIVE_##n, A}, \
	{TEXT_UMA_FILTER_ENV, PAR_UMA_FENV##n, A}, \
	{TEXT_UMA_LPG_DCY, PAR_UMA_LPG_DCY##n, A}, \
	{TEXT_UMA_LPG_COL, PAR_UMA_LPG_COL##n, A}, \
	{TEXT_UMA_LPG_EXC, PAR_UMA_LPG_EXC##n, A}, \
	/* LFO A */ \
	{TEXT_FREQ_LFO, PAR_FREQ_LFO##n, A}, \
	{TEXT_SYNC_LFO, PAR_SYNC_LFO##n, A}, \
	{TEXT_MOD_LFO, PAR_AMOUNT_LFO##n, A}, \
	{TEXT_WAVE_LFO, PAR_WAVE_LFO##n, A}, \
	{TEXT_RETRIGGER_LFO, PAR_RETRIGGER_LFO##n, A}, \
	{TEXT_OFFSET_LFO, PAR_OFFSET_LFO##n, A}, \
	/* Mix */ \
	{TEXT_VOLUME, PAR_VOL##n, A}, \
	{TEXT_PAN, PAR_PAN##n, A}, \
	{TEXT_SAMPLE_RATE, PAR_VOICE_DECIMATION##n, A}, \
	{TEXT_DRIVE, PAR_DRIVE##n, A}, \
	/* FX and kit */ \
	{TEXT_FX_P1, PAR_FX_P1, A}, \
	{TEXT_FX_P2, PAR_FX_P2, A}, \
	{TEXT_FX_P3, PAR_FX_P3, A}, \
	{TEXT_FX_P4, PAR_FX_P4, A}, \
	{TEXT_MORPH, PAR_MORPH, 0 /* kit morph: 0.3e */}

#define UMA_DRUM_TARGET_COUNT	73

static const UmaModTarget umaTgt_drums[3][UMA_DRUM_TARGET_COUNT] =
{
	{ UMA_DRUM_TARGETS(1) },
	{ UMA_DRUM_TARGETS(2) },
	{ UMA_DRUM_TARGETS(3) },
};

#undef A
#undef L
#undef F

// the snare's, cymbal's and hi-hat's base lists: the Eurorack's shared-list blocks (modTargetVoiceOffsets 4-6), off first
static const UmaModTarget umaTgt_vxBase3[39] =
{
	{TEXT_DRIVE, PAR_DRIVE3, UMA_TGT_ALL},
	{TEXT_FX_P1, PAR_FX_P1, UMA_TGT_ALL},
	{TEXT_FX_P2, PAR_FX_P2, UMA_TGT_ALL},
	{TEXT_FX_P3, PAR_FX_P3, UMA_TGT_ALL},
	{TEXT_FX_P4, PAR_FX_P4, UMA_TGT_ALL},
	{TEXT_MORPH, PAR_MORPH, 0 /* kit morph: 0.3e */},
	{TEXT_1VOCT, PAR_UMA_DSK_VOCT4, UMA_TGT_ALL},
	{TEXT_COARSE, PAR_COARSE4, UMA_TGT_ALL},
	{TEXT_FINE, PAR_FINE4, UMA_TGT_ALL},
	{TEXT_NOISE, PAR_NOISE_FREQ1, UMA_TGT_ALL},
	{TEXT_MIX, PAR_MIX1, UMA_TGT_ALL},
	{TEXT_WAVEFORM, PAR_OSC_WAVE_SNARE, UMA_TGT_ALL},
	{TEXT_PWM, PAR_PWM_4, UMA_TGT_ALL},
	{TEXT_ATTACK, PAR_VELOA4, UMA_TGT_ALL},
	{TEXT_DECAY, PAR_VELOD4, UMA_TGT_ALL},
	{TEXT_REPEAT, PAR_REPEAT4, UMA_TGT_ALL},
	{TEXT_EG_SLOPE, PAR_VOL_SLOPE4, UMA_TGT_ALL},
	{TEXT_PITCH_DECAY, PAR_MOD_EG4, UMA_TGT_ALL},
	{TEXT_EG_SLOPE, PAR_PITCH_SLOPE4, UMA_TGT_ALL},
	{TEXT_MOD_AMOUNT, PAR_MODAMNT4, UMA_TGT_ALL},
	{TEXT_VEL_DEST, PAR_VEL_DEST_4, UMA_TGT_ALL},
	{TEXT_VEL_AMT, PAR_VELO_MOD_AMT_4, UMA_TGT_ALL},
	{TEXT_VEL_MOD_VOL, PAR_VOLUME_MOD_ON_OFF4, UMA_TGT_ALL},
	{TEXT_TRANSIENT_WAVE, PAR_TRANS4_WAVE, UMA_TGT_ALL},
	{TEXT_TRANSIENT_VOLUME, PAR_TRANS4_VOL, UMA_TGT_ALL},
	{TEXT_TRANSIENT_FREQ, PAR_TRANS4_FREQ, UMA_TGT_ALL},
	{TEXT_FILTER_F, PAR_FILTER_FREQ_4, UMA_TGT_ALL},
	{TEXT_FILTER_RESO, PAR_RESO_4, UMA_TGT_ALL},
	{TEXT_FILTER_TYPE, PAR_FILTER_TYPE_4, UMA_TGT_ALL},
	{TEXT_FILTER_DRIVE, PAR_FILTER_DRIVE_4, UMA_TGT_ALL},
	{TEXT_FREQ_LFO, PAR_FREQ_LFO4, UMA_TGT_ALL},
	{TEXT_SYNC_LFO, PAR_SYNC_LFO4, UMA_TGT_ALL},
	{TEXT_MOD_LFO, PAR_AMOUNT_LFO4, UMA_TGT_ALL},
	{TEXT_WAVE_LFO, PAR_WAVE_LFO4, UMA_TGT_ALL},
	{TEXT_RETRIGGER_LFO, PAR_RETRIGGER_LFO4, UMA_TGT_ALL},
	{TEXT_OFFSET_LFO, PAR_OFFSET_LFO4, UMA_TGT_ALL},
	{TEXT_TARGET_VOICE_LFO, PAR_VOICE_LFO4, UMA_TGT_ALL},
	{TEXT_TARGET_LFO, PAR_TARGET_LFO4, UMA_TGT_ALL},
	{TEXT_VOLUME, PAR_VOL4, UMA_TGT_ALL},
};
static const UmaModTarget umaTgt_vxBase4[40] =
{
	{TEXT_PAN, PAR_PAN4, UMA_TGT_ALL},
	{TEXT_SAMPLE_RATE, PAR_VOICE_DECIMATION4, UMA_TGT_ALL},
	{TEXT_DRIVE, PAR_SNARE_DISTORTION, UMA_TGT_ALL},
	{TEXT_FX_P1, PAR_FX_P1, UMA_TGT_ALL},
	{TEXT_FX_P2, PAR_FX_P2, UMA_TGT_ALL},
	{TEXT_FX_P3, PAR_FX_P3, UMA_TGT_ALL},
	{TEXT_FX_P4, PAR_FX_P4, UMA_TGT_ALL},
	{TEXT_MORPH, PAR_MORPH, 0 /* kit morph: 0.3e */},
	{TEXT_1VOCT, PAR_UMA_DSK_VOCT5, UMA_TGT_ALL},
	{TEXT_COARSE, PAR_COARSE5, UMA_TGT_ALL},
	{TEXT_FINE, PAR_FINE5, UMA_TGT_ALL},
	{TEXT_WAVEFORM, PAR_WAVE1_CYM, UMA_TGT_ALL},
	{TEXT_PWM, PAR_PWM_5, UMA_TGT_ALL},
	{TEXT_ATTACK, PAR_VELOA5, UMA_TGT_ALL},
	{TEXT_DECAY, PAR_VELOD5, UMA_TGT_ALL},
	{TEXT_REPEAT, PAR_REPEAT5, UMA_TGT_ALL},
	{TEXT_EG_SLOPE, PAR_VOL_SLOPE5, UMA_TGT_ALL},
	{TEXT_VEL_DEST, PAR_VEL_DEST_5, UMA_TGT_ALL},
	{TEXT_VEL_AMT, PAR_VELO_MOD_AMT_5, UMA_TGT_ALL},
	{TEXT_VEL_MOD_VOL, PAR_VOLUME_MOD_ON_OFF5, UMA_TGT_ALL},
	{TEXT_MOD_OSC1_FREQ, PAR_MOD_OSC_F1_CYM, UMA_TGT_ALL},
	{TEXT_MOD_OSC2_FREQ, PAR_MOD_OSC_F2_CYM, UMA_TGT_ALL},
	{TEXT_MOD_OSC1_GAIN, PAR_MOD_OSC_GAIN1_CYM, UMA_TGT_ALL},
	{TEXT_MOD_OSC2_GAIN, PAR_MOD_OSC_GAIN2_CYM, UMA_TGT_ALL},
	{TEXT_WAVEFORM, PAR_WAVE2_CYM, UMA_TGT_ALL},
	{TEXT_WAVEFORM, PAR_WAVE3_CYM, UMA_TGT_ALL},
	{TEXT_TRANSIENT_WAVE, PAR_TRANS5_WAVE, UMA_TGT_ALL},
	{TEXT_TRANSIENT_VOLUME, PAR_TRANS5_VOL, UMA_TGT_ALL},
	{TEXT_TRANSIENT_FREQ, PAR_TRANS5_FREQ, UMA_TGT_ALL},
	{TEXT_FILTER_F, PAR_FILTER_FREQ_5, UMA_TGT_ALL},
	{TEXT_FILTER_RESO, PAR_RESO_5, UMA_TGT_ALL},
	{TEXT_FILTER_TYPE, PAR_FILTER_TYPE_5, UMA_TGT_ALL},
	{TEXT_FILTER_DRIVE, PAR_FILTER_DRIVE_5, UMA_TGT_ALL},
	{TEXT_FREQ_LFO, PAR_FREQ_LFO5, UMA_TGT_ALL},
	{TEXT_SYNC_LFO, PAR_SYNC_LFO5, UMA_TGT_ALL},
	{TEXT_MOD_LFO, PAR_AMOUNT_LFO5, UMA_TGT_ALL},
	{TEXT_WAVE_LFO, PAR_WAVE_LFO5, UMA_TGT_ALL},
	{TEXT_RETRIGGER_LFO, PAR_RETRIGGER_LFO5, UMA_TGT_ALL},
	{TEXT_OFFSET_LFO, PAR_OFFSET_LFO5, UMA_TGT_ALL},
	{TEXT_TARGET_VOICE_LFO, PAR_VOICE_LFO5, UMA_TGT_ALL},
};
static const UmaModTarget umaTgt_vxBase5[40] =
{
	{TEXT_TARGET_LFO, PAR_TARGET_LFO5, UMA_TGT_ALL},
	{TEXT_VOLUME, PAR_VOL5, UMA_TGT_ALL},
	{TEXT_PAN, PAR_PAN5, UMA_TGT_ALL},
	{TEXT_SAMPLE_RATE, PAR_VOICE_DECIMATION5, UMA_TGT_ALL},
	{TEXT_DRIVE, PAR_CYMBAL_DISTORTION, UMA_TGT_ALL},
	{TEXT_FX_P1, PAR_FX_P1, UMA_TGT_ALL},
	{TEXT_FX_P2, PAR_FX_P2, UMA_TGT_ALL},
	{TEXT_FX_P3, PAR_FX_P3, UMA_TGT_ALL},
	{TEXT_FX_P4, PAR_FX_P4, UMA_TGT_ALL},
	{TEXT_MORPH, PAR_MORPH, 0 /* kit morph: 0.3e */},
	{TEXT_1VOCT, PAR_UMA_DSK_VOCT6, UMA_TGT_ALL},
	{TEXT_COARSE, PAR_COARSE6, UMA_TGT_ALL},
	{TEXT_FINE, PAR_FINE6, UMA_TGT_ALL},
	{TEXT_WAVEFORM, PAR_WAVE1_HH, UMA_TGT_ALL},
	{TEXT_PWM, PAR_PWM_6, UMA_TGT_ALL},
	{TEXT_ATTACK, PAR_VELOA6, UMA_TGT_ALL},
	{TEXT_DECAY_CLOSED, PAR_VELOD6_CLOSED, UMA_TGT_ALL},
	{TEXT_DECAY_OPEN, PAR_VELOD6_OPEN, UMA_TGT_ALL},
	{TEXT_EG_SLOPE, PAR_VOL_SLOPE6, UMA_TGT_ALL},
	{TEXT_VEL_DEST, PAR_VEL_DEST_6, UMA_TGT_ALL},
	{TEXT_VEL_AMT, PAR_VELO_MOD_AMT_6, UMA_TGT_ALL},
	{TEXT_VEL_MOD_VOL, PAR_VOLUME_MOD_ON_OFF6, UMA_TGT_ALL},
	{TEXT_MOD_OSC1_FREQ, PAR_MOD_OSC_F1, UMA_TGT_ALL},
	{TEXT_MOD_OSC2_FREQ, PAR_MOD_OSC_F2, UMA_TGT_ALL},
	{TEXT_MOD_OSC1_GAIN, PAR_MOD_OSC_GAIN1, UMA_TGT_ALL},
	{TEXT_MOD_OSC2_GAIN, PAR_MOD_OSC_GAIN2, UMA_TGT_ALL},
	{TEXT_WAVEFORM, PAR_WAVE2_HH, UMA_TGT_ALL},
	{TEXT_WAVEFORM, PAR_WAVE3_HH, UMA_TGT_ALL},
	{TEXT_TRANSIENT_WAVE, PAR_TRANS6_WAVE, UMA_TGT_ALL},
	{TEXT_TRANSIENT_VOLUME, PAR_TRANS6_VOL, UMA_TGT_ALL},
	{TEXT_TRANSIENT_FREQ, PAR_TRANS6_FREQ, UMA_TGT_ALL},
	{TEXT_FILTER_F, PAR_FILTER_FREQ_6, UMA_TGT_ALL},
	{TEXT_FILTER_RESO, PAR_RESO_6, UMA_TGT_ALL},
	{TEXT_FILTER_TYPE, PAR_FILTER_TYPE_6, UMA_TGT_ALL},
	{TEXT_FILTER_DRIVE, PAR_FILTER_DRIVE_6, UMA_TGT_ALL},
	{TEXT_FREQ_LFO, PAR_FREQ_LFO6, UMA_TGT_ALL},
	{TEXT_SYNC_LFO, PAR_SYNC_LFO6, UMA_TGT_ALL},
	{TEXT_MOD_LFO, PAR_AMOUNT_LFO6, UMA_TGT_ALL},
	{TEXT_WAVE_LFO, PAR_WAVE_LFO6, UMA_TGT_ALL},
	{TEXT_RETRIGGER_LFO, PAR_RETRIGGER_LFO6, UMA_TGT_ALL},
};
static const UmaModTarget* const umaTgt_vxBase[3] = {umaTgt_vxBase3, umaTgt_vxBase4, umaTgt_vxBase5};
static const uint8_t umaTgt_vxBaseCount[3] = {39, 40, 40};

// every list ends with the amounts of the voice's own 8 mod slots (UMA 0.21: after the rest, so the targets saved
// in older kits keep their index), then its LFO B (UMA 0.27), then on Drum 1-3 FM Tone's settings (UMA 0.28)
static uint8_t umaTgt_baseCount(uint8_t voice)
{
	if(voice < 3)
		return UMA_DRUM_TARGET_COUNT;
	if(voice < 6)
		return (uint8_t)(umaTgt_vxBaseCount[voice - 3] + 1);
	return 1;
}

// then the voice's LFO B (UMA 0.27): frB ofB wvB rtB snB, and the folders of LFO A and B (UMA 0.28): fdA fdB
#define UMA_TGT_LFOB_COUNT	7
static const uint16_t umaTgt_lfoBNames[UMA_TGT_LFOB_COUNT] =
	{TEXT_UMA_LFOB_FREQ, TEXT_UMA_LFOB_OFS, TEXT_UMA_LFOB_WAVE, TEXT_UMA_LFOB_RTG, TEXT_UMA_LFOB_SYNC,
	 TEXT_UMA_LFOA_FOLD, TEXT_UMA_LFOB_FOLD};

static uint16_t umaTgt_lfoBParam(uint8_t voice, uint8_t k)
{
	if(k < UMA_LFOB_PARS)
		return (uint16_t)(UMA_LFOB_FIRST_PAR + voice * UMA_LFOB_PARS + k);
	if(k == UMA_LFOB_PARS)
		return (uint16_t)(UMA_LFOB_SYNC_PAR + voice);
	return (uint16_t)((k == UMA_LFOB_PARS + 1 ? UMA_LFOA_FOLD_PAR : UMA_LFOB_FOLD_PAR) + voice);
}

// then on Drum 1-3 FM Tone's (UMA 0.28): rtc rb2 m2 dtn mix sub sbs sbo sbf (the kit's order), then B2's
// envelope a2 d2 e2, then randomise's ratio palette rpl (UMA 0.28, both FM engines), then the prk engine's
// alg var p1 p2 (UMA 0.29, prk only)
#define UMA_TGT_RPL_AT		(UMA_TONE_PARS + UMA_B2ENV_PARS)
#define UMA_TGT_PRK_AT		(UMA_TGT_RPL_AT + 1)
#define UMA_TGT_MD_AT		(UMA_TGT_PRK_AT + UMA_PRK_PARS + 1)	// after prk's palette rpk (prk only)
#define UMA_TGT_TONE_COUNT	(UMA_TGT_MD_AT + UMA_MD_PARS + 1)	// then the drum machines' typ mch k1-k8 rpm (mdk only)
static const uint16_t umaTgt_toneNames[UMA_TGT_TONE_COUNT] =
	{TEXT_UMA_RATIO_C, TEXT_UMA_RATIO_B2, TEXT_UMA_T_MOD_B2, TEXT_UMA_DETUNE, TEXT_UMA_MIX_XY,
	 TEXT_UMA_SUB, TEXT_UMA_SUB_SHAPE, TEXT_UMA_SUB_OCT, TEXT_UMA_SUB_FOLD,
	 TEXT_UMA_T_ATK_B2, TEXT_UMA_T_DEC_B2, TEXT_UMA_T_END_B2, TEXT_UMA_RND_PALETTE,
	 TEXT_UMA_T_PRK_ALG, TEXT_UMA_T_PRK_VAR, TEXT_UMA_T_PRK_P1, TEXT_UMA_T_PRK_P2, TEXT_UMA_T_PRK_PAL,
	 TEXT_UMA_MD_TYP, TEXT_UMA_MD_MCH, TEXT_UMA_MD_K1, TEXT_UMA_MD_K2, TEXT_UMA_MD_K3, TEXT_UMA_MD_K4,
	 TEXT_UMA_MD_K5, TEXT_UMA_MD_K6, TEXT_UMA_MD_K7, TEXT_UMA_MD_K8, TEXT_UMA_T_MD_PAL};

static uint16_t umaTgt_toneParam(uint8_t voice, uint8_t k)
{
	if(k == UMA_TGT_MD_AT + UMA_MD_PARS)
		return (uint16_t)(UMA_MD_PAL_PAR + voice);
	if(k >= UMA_TGT_MD_AT)
		return (uint16_t)(UMA_MD_FIRST_PAR + voice * UMA_MD_PARS + k - UMA_TGT_MD_AT);
	if(k == UMA_TGT_PRK_AT + UMA_PRK_PARS)
		return (uint16_t)(UMA_PRK_PAL_PAR + voice);
	if(k >= UMA_TGT_PRK_AT)
		return (uint16_t)(UMA_PRK_FIRST_PAR + voice * UMA_PRK_PARS + k - UMA_TGT_PRK_AT);
	if(k == UMA_TGT_RPL_AT)
		return (uint16_t)(UMA_RPL_PAR + voice);
	return k < UMA_TONE_PARS ? (uint16_t)(UMA_TONE_FIRST_PAR + voice * UMA_TONE_PARS + k)
	                         : (uint16_t)(UMA_B2ENV_FIRST_PAR + voice * UMA_B2ENV_PARS + k - UMA_TONE_PARS);
}

// the first entry after LFO B
static uint8_t umaTgt_toneStart(uint8_t voice)
{
	return (uint8_t)(umaTgt_baseCount(voice) + MOD_SLOTS_PER_VOICE + UMA_TGT_LFOB_COUNT);
}

// the end of the list before the last entry, `vel` (UMA 0.29: every voice's hit velocity, last of every list)
static uint8_t umaTgt_velAt(uint8_t voice)
{
	return (uint8_t)(umaTgt_toneStart(voice) + (voice < 3 ? UMA_TGT_TONE_COUNT : 0));
}

// then on the snare, cymbal and hi-hat their engines' settings (UMA 0.29, docs/design/voice-engines.md): prk's,
// mdk's, randomise, the folder, the filter envelope; in their block's order (uma_voice.h), eng left out (as on Drum 1-3)
typedef struct { uint8_t k; uint16_t name; uint8_t engine; } UmaVxTarget;
#define PM (UMA_TGT_PRK | UMA_TGT_MD)
static const UmaVxTarget umaTgt_vx[] =
{
	{UMA_VX_PRK_ALG, TEXT_UMA_T_PRK_ALG, UMA_TGT_PRK}, {UMA_VX_PRK_VAR, TEXT_UMA_T_PRK_VAR, UMA_TGT_PRK},
	{UMA_VX_PRK_P1, TEXT_UMA_T_PRK_P1, UMA_TGT_PRK}, {UMA_VX_PRK_P2, TEXT_UMA_T_PRK_P2, UMA_TGT_PRK},
	{UMA_VX_PRK_PAL, TEXT_UMA_T_PRK_PAL, UMA_TGT_PRK},
	{UMA_VX_MD_TYP, TEXT_UMA_MD_TYP, UMA_TGT_MD}, {UMA_VX_MD_MCH, TEXT_UMA_MD_MCH, UMA_TGT_MD},
	{UMA_VX_MD_K1, TEXT_UMA_MD_K1, UMA_TGT_MD}, {UMA_VX_MD_K1 + 1, TEXT_UMA_MD_K2, UMA_TGT_MD},
	{UMA_VX_MD_K1 + 2, TEXT_UMA_MD_K3, UMA_TGT_MD}, {UMA_VX_MD_K1 + 3, TEXT_UMA_MD_K4, UMA_TGT_MD},
	{UMA_VX_MD_K1 + 4, TEXT_UMA_MD_K5, UMA_TGT_MD}, {UMA_VX_MD_K1 + 5, TEXT_UMA_MD_K6, UMA_TGT_MD},
	{UMA_VX_MD_K1 + 6, TEXT_UMA_MD_K7, UMA_TGT_MD}, {UMA_VX_MD_K1 + 7, TEXT_UMA_MD_K8, UMA_TGT_MD},
	{UMA_VX_MD_PAL, TEXT_UMA_T_MD_PAL, UMA_TGT_MD},
	{UMA_VX_RND_TONE, TEXT_UMA_T_RND_TONE, PM}, {UMA_VX_RND_ENV, TEXT_UMA_T_RND_ENV, PM},
	{UMA_VX_RND_SEED, TEXT_UMA_T_RND_SEED, PM},
	{UMA_VX_FOLD, TEXT_UMA_FOLD, PM}, {UMA_VX_FOLD_TYPE, TEXT_UMA_FOLD_TYPE, PM}, {UMA_VX_FOLD_SYM, TEXT_UMA_FOLD_SYM, PM},
	{UMA_VX_FOLD_LEV, TEXT_UMA_BODY_LEV, PM},
	{UMA_VX_FENV, TEXT_UMA_FILTER_ENV, UMA_TGT_ALL},
};
#define UMA_TGT_VX_COUNT	(sizeof(umaTgt_vx) / sizeof(umaTgt_vx[0]))

// then on every voice the pitch screen (UMA 0.31, docs/design/quantizer.md): oct key scl, the last of every list
static uint8_t umaTgt_pitchAt(uint8_t voice)
{
	return (uint8_t)(umaTgt_velAt(voice) + 1 + (voice >= 3 ? UMA_TGT_VX_COUNT : 0));
}
static const uint16_t umaTgt_pitchNames[UMA_QUANT_PARS] = {TEXT_UMA_OCT, TEXT_UMA_KEY, TEXT_UMA_SCALE};

// then the Plaits engine's (UMA 0.32): bnk mdl hrm tmb mor out, pla only, then fil (0.33, dxb until 0.34d) kmd twi, sub sbs sbo sbf (0.34e)
static uint8_t umaTgt_plaAt(uint8_t voice)
{
	return (uint8_t)(umaTgt_pitchAt(voice) + UMA_QUANT_PARS);
}
static const uint16_t umaTgt_plaNames[UMA_PLA_TARGETS] = {TEXT_UMA_PLA_BNK, TEXT_UMA_PLA_MDL, TEXT_UMA_PLA_HRM,
	TEXT_UMA_PLA_TMB, TEXT_UMA_PLA_MOR, TEXT_UMA_PLA_OUT, TEXT_UMA_PLA_DXB, TEXT_UMA_PLA_KMD,
	TEXT_UMA_PLA_TWI, TEXT_UMA_SUB, TEXT_UMA_SUB_SHAPE, TEXT_UMA_SUB_OCT, TEXT_UMA_SUB_FOLD};
// an entry of the Plaits targets -> umaVoice_plaPar's k: 0-8 as they are, then the sub (0.34e) at k 11-14 (9-10, the
// wavetable's and terrain's files, are reached through `fil`)
static uint8_t umaTgt_plaK(uint8_t entry) { return entry < 9 ? entry : (uint8_t)(entry + 2); }

uint8_t umaTgt_count(uint8_t voice)
{
	if(voice >= 6)
		return 1;
	return (uint8_t)(umaTgt_plaAt(voice) + UMA_PLA_TARGETS);
}

// an entry of the Plaits engine (0-5), or 0xff
static uint8_t umaTgt_plaEntry(uint8_t voice, uint8_t idx)
{
	if(voice >= 6 || idx < umaTgt_plaAt(voice) || idx >= umaTgt_count(voice))
		return 0xff;
	return (uint8_t)(idx - umaTgt_plaAt(voice));
}

// an entry of the pitch screen (0-2: oct key scl), or 0xff
static uint8_t umaTgt_pitchEntry(uint8_t voice, uint8_t idx)
{
	if(voice >= 6 || idx < umaTgt_pitchAt(voice) || idx >= umaTgt_plaAt(voice))
		return 0xff;
	return (uint8_t)(idx - umaTgt_pitchAt(voice));
}

// an entry after vel on the snare, cymbal, hi-hat, or NULL
static const UmaVxTarget* umaTgt_vxEntry(uint8_t voice, uint8_t idx)
{
	if(voice < 3 || voice >= 6 || idx <= umaTgt_velAt(voice) || idx >= umaTgt_pitchAt(voice))
		return 0;
	return &umaTgt_vx[idx - umaTgt_velAt(voice) - 1];
}

uint16_t umaTgt_nameIdx(uint8_t voice, uint8_t idx)
{
	if(idx == 0 || idx >= umaTgt_count(voice))
		return TEXT_EMPTY;
	const uint8_t base = umaTgt_baseCount(voice);
	if(umaTgt_plaEntry(voice, idx) != 0xff)
		return umaTgt_plaNames[umaTgt_plaEntry(voice, idx)];
	if(umaTgt_pitchEntry(voice, idx) != 0xff)
		return umaTgt_pitchNames[umaTgt_pitchEntry(voice, idx)];
	if(umaTgt_vxEntry(voice, idx))
		return umaTgt_vxEntry(voice, idx)->name;
	if(idx == umaTgt_velAt(voice))
		return TEXT_UMA_VEL;
	if(idx >= umaTgt_toneStart(voice))
		return umaTgt_toneNames[idx - umaTgt_toneStart(voice)];
	if(idx >= base + MOD_SLOTS_PER_VOICE)
		return umaTgt_lfoBNames[idx - base - MOD_SLOTS_PER_VOICE];
	if(idx >= base)
		return (uint16_t)(TEXT_UMA_T_SLOT_AMT1 + idx - base);
	if(voice < 3)
		return umaTgt_drums[voice][idx].nameIdx;
	return umaTgt_vxBase[voice - 3][idx - 1].nameIdx;
}

uint16_t umaTgt_rawParam(uint8_t voice, uint8_t idx)
{
	if(idx == 0 || idx >= umaTgt_count(voice))
		return PAR_NONE;
	const uint8_t base = umaTgt_baseCount(voice);
	if(umaTgt_plaEntry(voice, idx) != 0xff)
		return umaVoice_plaPar(voice, umaTgt_plaK(umaTgt_plaEntry(voice, idx)));
	if(umaTgt_pitchEntry(voice, idx) != 0xff)
		return (uint16_t)(UMA_QUANT_FIRST_PAR + voice * UMA_QUANT_PARS + umaTgt_pitchEntry(voice, idx));
	if(umaTgt_vxEntry(voice, idx))
		return UMA_VX_PAR(voice, umaTgt_vxEntry(voice, idx)->k);
	if(idx == umaTgt_velAt(voice))
		return (uint16_t)(UMA_VEL_PAR + voice);
	if(idx >= umaTgt_toneStart(voice))
		return umaTgt_toneParam(voice, (uint8_t)(idx - umaTgt_toneStart(voice)));
	if(idx >= base + MOD_SLOTS_PER_VOICE)
		return umaTgt_lfoBParam(voice, (uint8_t)(idx - base - MOD_SLOTS_PER_VOICE));
	if(idx >= base)
		return modSlot_amt((uint8_t)(voice * MOD_SLOTS_PER_VOICE + idx - base));
	if(voice < 3)
		return umaTgt_drums[voice][idx].param;
	return umaTgt_vxBase[voice - 3][idx - 1].param;
}

uint8_t umaTgt_engineTag(uint8_t voice, uint8_t idx)
{
	if(voice < 3 && idx < UMA_DRUM_TARGET_COUNT)
		return umaTgt_drums[voice][idx].engine;
	if(umaTgt_pitchEntry(voice, idx) != 0xff)
		return UMA_TGT_ALL;	// the pitch screen: every engine
	if(umaTgt_plaEntry(voice, idx) != 0xff)
		return UMA_TGT_PLA;
	if(umaTgt_vxEntry(voice, idx))
		return umaTgt_vxEntry(voice, idx)->engine;
	if(idx == umaTgt_velAt(voice))
		return 0;	// LX_UMA desktop: no `vel` setting (the steps have velocity)
	if(idx >= umaTgt_baseCount(voice) + MOD_SLOTS_PER_VOICE && idx < umaTgt_toneStart(voice)
		&& umaTgt_lfoBParam(voice, (uint8_t)(idx - umaTgt_baseCount(voice) - MOD_SLOTS_PER_VOICE)) == (uint16_t)(UMA_LFOA_FOLD_PAR + voice))
		return 0;	// LX_UMA desktop: LFO A is the stock LFO (no folder)
	if(voice >= 3 && voice < 6 && idx >= 1 && idx <= umaTgt_vxBaseCount[voice - 3])
		return umaTgt_vxBase[voice - 3][idx - 1].engine;
	if(voice < 3 && idx >= umaTgt_toneStart(voice))
	{
		const uint8_t k = (uint8_t)(idx - umaTgt_toneStart(voice));
		return k >= UMA_TGT_MD_AT ? UMA_TGT_MD : k >= UMA_TGT_PRK_AT ? UMA_TGT_PRK
		     : k == UMA_TGT_RPL_AT ? UMA_TGT_FM : UMA_TGT_FMT;	// rpl: both FM
	}
	return UMA_TGT_ALL;
}

uint8_t umaTgt_voiceEngine(uint8_t voice)
{
#if UMA_FM
	if(voice < 6)
		return (uint8_t)(1u << umaVoice_engineKnob(voice));	// the engine's bit (UMA_ENG_* = the bit numbers)
#endif
	return UMA_TGT_LXR;
}

const char* umaTgt_engineNote(uint8_t tag)
{
	if(tag == UMA_TGT_LXR) return "lxr only";
	if(tag == UMA_TGT_FM) return "fm only ";
	if(tag == UMA_TGT_FMT) return "fmt only";
	if(tag == UMA_TGT_PRK) return "prk only";
	if(tag == UMA_TGT_MD) return "mdk only";
	if(tag == UMA_TGT_PLA) return "pla only";
	if(tag == (UMA_TGT_FM | UMA_TGT_PRK | UMA_TGT_MD | UMA_TGT_PLA)) return "not lxr ";
	if(tag == (UMA_TGT_PRK | UMA_TGT_MD)) return "prk, mdk";
	if(tag == (UMA_TGT_FM | UMA_TGT_PRK)) return "fm, prk ";
	return "other   ";
}

uint8_t umaTgt_isActive(uint8_t voice, uint8_t idx)
{
	return (umaTgt_engineTag(voice, idx) & umaTgt_voiceEngine(voice)) != 0;
}

uint16_t umaTgt_param(uint8_t voice, uint8_t idx)
{
	return umaTgt_isActive(voice, idx) ? umaTgt_rawParam(voice, idx) : PAR_NONE;
}

uint16_t umaTgt_slotParam(uint8_t slot)
{
	const uint8_t voice = (uint8_t)(slot / MOD_SLOTS_PER_VOICE);
	return umaTgt_param(voice, activeParameters[modSlot_dest(slot)]);
}

uint8_t umaTgt_step(uint8_t voice, uint8_t idx, int16_t steps)
{
	const uint8_t count = umaTgt_count(voice);
	int16_t i = idx < count ? idx : (int16_t)(count - 1);
	const int16_t dir = steps < 0 ? -1 : 1;
	int16_t last = i;	// the last shown entry reached
	if(!umaTgt_isActive(voice, (uint8_t)i))
		last = dir < 0 ? 0 : -1;	// from the other engine's entry: the nearest shown one in the direction
	for(int16_t n = steps < 0 ? -steps : steps; n > 0; )
	{
		i += dir;
		if(i < 0 || i >= count)
			break;
		if(i == 0 || umaTgt_isActive(voice, (uint8_t)i))
		{
			last = i;
			n--;
		}
	}
	if(last < 0)	// nothing shown after it: stay
		return idx;
	return (uint8_t)last;
}

uint8_t umaTgt_find(uint8_t voice, uint16_t param)
{
	if(param == PAR_NONE)
		return 0;
	const uint8_t count = umaTgt_count(voice);
	for(uint8_t i = 1; i < count; i++)
		if(umaTgt_rawParam(voice, i) == param)
			return i;
	return 0;
}


// the voices as the sources name them (5 letters, so "EnvInv Cymbl" fits the 12 places)
static const char umaTgt_voiceNames[6][6] = {"Drum1", "Drum2", "Drum3", "Snare", "Cymbl", "HiHat"};

void umaTgt_sourceName(uint8_t src, char* out, uint8_t size)
{
	if(src < 5)
		snprintf(out, size, "---");	// LX_UMA desktop: no CV inputs
	else if(src < 11)
		snprintf(out, size, "LFO A %s", umaTgt_voiceNames[src - 5]);
	else if(src < UMA_TM_FIRST_SOURCE)
		snprintf(out, size, "---");	// LX_UMA desktop: no accent jacks
	else if(src < UMA_TM_FIRST_SOURCE + UMA_TM_SOURCES)
	{
		const uint8_t k = (uint8_t)(src - UMA_TM_FIRST_SOURCE);
		snprintf(out, size, "TM %c %s", 'A' + k % 4, umaTgt_voiceNames[k / 4]);
	}
	else if(src < UMA_ENV_FIRST_SOURCE)
		snprintf(out, size, "LFO B %s", umaTgt_voiceNames[src - UMA_TM_FIRST_SOURCE - UMA_TM_SOURCES]);
	else if(src < UMA_ENV_FIRST_SOURCE + 6)
		snprintf(out, size, "Env %s", umaTgt_voiceNames[src - UMA_ENV_FIRST_SOURCE]);
	else if(src < UMA_VEL_FIRST_SOURCE)
		snprintf(out, size, "EnvInv %s", umaTgt_voiceNames[src - UMA_ENV_FIRST_SOURCE - 6]);
	else if(src < UMA_MC_FIRST_SOURCE)
		snprintf(out, size, "Veloc %s", umaTgt_voiceNames[src - UMA_VEL_FIRST_SOURCE]);
	else if(src < UMA_NUM_MOD_SOURCES)
		snprintf(out, size, "---");	// LX_UMA desktop: no MIDI CC sources
	else if(size)
		out[0] = 0;
}
