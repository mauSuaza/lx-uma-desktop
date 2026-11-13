/*
 * uma_voice.h - UMA: the engines prk and mdk on every voice (docs/design/voice-engines.md). Drum 1-3 host them in
 * DrumVoice.c; the snare, cymbal and hi-hat here, with the same chain: engine -> wavefolder -> + transient -> filter
 * (+ filter envelope) -> amp EG x volume (not for mdk) -> drive. Also where each voice's engine settings are, for the
 * menu, the targets and randomise: voices 0-2 Drum 1-3, 3 snare, 4 cymbal, 5 hi-hat.
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef UMA_VOICE_H_
#define UMA_VOICE_H_

#include <stdint.h>

#define UMA_VOICES	6

// the engines, as Drum 1-3's eng values (the snare, cymbal and hi-hat store lxr prk mdk as 0 1 2)
enum { UMA_ENG_LXR, UMA_ENG_FMD, UMA_ENG_FMT, UMA_ENG_PRK, UMA_ENG_MDK, UMA_ENG_PLA };

// the snare's, cymbal's and hi-hat's settings, UMA_VX_PARS each from UMA_VX_FIRST_PAR (Parameters.h)
enum
{
	UMA_VX_ENG,
	UMA_VX_PRK_ALG, UMA_VX_PRK_VAR, UMA_VX_PRK_P1, UMA_VX_PRK_P2, UMA_VX_PRK_PAL,
	UMA_VX_MD_TYP, UMA_VX_MD_MCH, UMA_VX_MD_K1,
	UMA_VX_MD_PAL = UMA_VX_MD_K1 + 8,
	UMA_VX_RND_TONE, UMA_VX_RND_ENV, UMA_VX_RND_SEED,
	UMA_VX_FOLD, UMA_VX_FOLD_TYPE, UMA_VX_FOLD_SYM, UMA_VX_FOLD_LEV,
	UMA_VX_FENV,
	UMA_VX_COUNT
};
#define UMA_VX_PAR(voice, k)	((uint16_t)(UMA_VX_FIRST_PAR + ((voice) - 3) * UMA_VX_PARS + (k)))

// a voice's engine (UMA_ENG_*): knob = the setting as stored, else with modulation and morph
uint8_t umaVoice_engine(uint8_t voice);
uint8_t umaVoice_engineKnob(uint8_t voice);
// the voice whose engine setting this is (0-5), 0xff if it is none
uint8_t umaVoice_ofEngine(uint16_t param);
// where a voice's settings are, for any voice 0-5
uint16_t umaVoice_prkPar(uint8_t voice, uint8_t k);	// k 0-3: alg var p1 p2
uint16_t umaVoice_prkPalPar(uint8_t voice);
uint16_t umaVoice_mdPar(uint8_t voice, uint8_t k);	// k 0 type, 1 machine, 2-9 the knobs
uint16_t umaVoice_plaPar(uint8_t voice, uint8_t k);	// the Plaits engine: k 0 bank, 1 model, 2-4 hrm tmb mor, 5 output,
														// 6 fil (the DX bank), 7 key mode, 8 TWIST, 9-10 the wavetable's and terrain's files, 11-14 the sub (sub sbs sbo sbf)
// the Plaits engine's settings (with modulation) into the voice's Plaits instance (PlaitsDrum.h voice = voice):
// the bank's model, the three knobs and TWIST, the output, the 6-op's bank and key. Every block, on every voice with eng pla.
// Returns 1 if the sounding model is tonal (the amp EG sets its volume, the pitch EG applies, it isn't run while the
// EG is shut)
uint8_t umaVoice_plaSync(uint8_t voice);
uint16_t umaVoice_mdPalPar(uint8_t voice);
uint16_t umaVoice_foldPar(uint8_t voice, uint8_t k);	// k 0-3: fld fty sym lev
uint16_t umaVoice_fenvPar(uint8_t voice);
// the parameter's voice and index when it is one of those blocks: kind 1 prk (k 0-3), 2 prk palette, 3 mdk (k 0-9),
// 4 mdk palette, 5 Plaits (k 0-8: bnk mdl hrm tmb mor out dxb kmd twi); 0 if it is none
uint8_t umaVoice_which(uint16_t param, uint8_t* voice, uint8_t* k);
// the voice's amp EG: attack, decay (the hi-hat: closed), slope; the hi-hat's open decay (PAR_NONE elsewhere)
uint16_t umaVoice_ampPar(uint8_t voice, uint8_t which);	// 0 attack, 1 decay, 2 slope, 3 open decay

// the snare, cymbal and hi-hat (voice 3-5), from their own code: a hit (isOpen: the hi-hat's open trigger), the
// block's settings after randomise (their calcAsync), the render (their calcSyncBlock: 1 if an engine rendered it, 0
// for their stock sound), their filter with the filter envelope (also for the stock sound)
void umaVoice_init(void);
void umaVoice_trigger(uint8_t voice, uint8_t isOpen);
void umaVoice_async(uint8_t voice);
uint8_t umaVoice_render(uint8_t voice, int16_t* buf, uint8_t size);
void umaVoice_filterBlock(uint8_t voice, int16_t* buf, uint8_t size);

#endif /* UMA_VOICE_H_ */
