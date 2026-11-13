/*
 * PrkDrum.h - UMA: the prk engine, percussion generators after the Perkons Voice's algorithms
 * (docs/design/prk-engine.md). Most are adapted from Percolator (MIT, see PrkDrum.c); pls, ssn and kps are UMA's.
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef PRKDRUM_H_
#define PRKDRUM_H_

#include <stdint.h>

enum PrkDrumParam
{
	PRK_ALG,	// 0 .. PRK_NUM_ALGS-1
	PRK_VAR,	// 0 .. prkDrum_varCount(alg)-1
	PRK_P1,		// 0-127
	PRK_P2,		// 0-127
	// the test app's own (on the module the voice sets pitch, envelope and level)
	PRK_TUNE,	// MIDI note of the pitch
	PRK_DECAY,	// 0-127: the engine's own decay, used only with prkDrum_setAmpEnv(d, 1)
	PRK_LEVEL,	// 0-127
	PRK_NUM_PARAMS
};

enum PrkDrumAlg
{
	PRK_FLD,	// fold drum
	PRK_CPX,	// complex FM drum
	PRK_WAV,	// wave morph drum
	PRK_PLS,	// pulse drum
	PRK_RBD,	// resonant bass drum
	PRK_RSN,	// resonant snare
	PRK_SSN,	// noise + tone snare
	PRK_SLP,	// clap
	PRK_KPS,	// Karplus-Strong
	PRK_CYM,	// cymbal, bell
	PRK_NCR,	// noise crush
	PRK_MTN,	// metallic PCM noise
	PRK_TOM,	// resonator tom
	PRK_NUM_ALGS
};

#define PRK_BLOCK_SIZE	64
#define PRK_REF_HZ		110.f	// the pitch at which the noise and metal algorithms sound as designed
#define PRK_KPS_LEN		2048	// Karplus-Strong delay line: down to about 22 Hz
#define PRK_ROOM_LEN	1976	// the room's four combs and two allpasses

typedef struct { float s1, s2; } PrkSvf;

typedef struct
{
	uint8_t params[PRK_NUM_PARAMS];
	float pitch;		// Hz
	float ringTime;		// seconds: how long the resonators ring (the Amp EG's decay on the module)
	uint8_t ampEnvOn;	// 1: the engine's own decay envelope (test app); 0: the voice's envelope (module)
	float envIn, envLast;	// the voice's envelope, for the sounds that change with it (fold depth, FM index)
	float env;			// own envelope
	float level;		// own level x velocity
	// voice state
	uint32_t ns;		// samples since the hit
	float penv, tr, nenv, bt;
	uint8_t bursts;
	float ph[8];
	float zr[3], zi[3];
	uint32_t rng;
	PrkSvf f1, f2;
	float lp1, hp1, hold, holdc;
	uint32_t kw;		// Karplus-Strong write position
	float kexc;			// samples of excitation left
	float kprev;
	// the room (slp cym ncr) and the string (kps) never play at once: one buffer
	union
	{
		float room[PRK_ROOM_LEN];
		float kps[PRK_KPS_LEN];
	} buf;
	uint16_t ci[4], ai[2];
	float damp[4];
	uint8_t roomUsed;
} PrkDrum;

void prkDrum_init(PrkDrum* d);
void prkDrum_setParam(PrkDrum* d, uint8_t param, uint8_t value);
uint8_t prkDrum_getParam(const PrkDrum* d, uint8_t param);
void prkDrum_setAmpEnv(PrkDrum* d, uint8_t on);
void prkDrum_setPitch(PrkDrum* d, float hz);		// module: the voice's pitch (tune, MIDI, CV, sweep)
void prkDrum_setRingTime(PrkDrum* d, float seconds);	// module: from the Amp EG's decay
void prkDrum_setEnv(PrkDrum* d, float env);			// module: the Amp EG's value, once per block
void prkDrum_trigger(PrkDrum* d, float velocity);
void prkDrum_render(PrkDrum* d, float* out, uint32_t numSamples);

uint8_t prkDrum_varCount(uint8_t alg);
const char* prkDrum_algShort(uint8_t alg);		// 3 letters: "fld"
const char* prkDrum_algName(uint8_t alg);		// "Fold Drum"
const char* prkDrum_varName(uint8_t alg, uint8_t var);
const char* prkDrum_p1Name(uint8_t alg);
const char* prkDrum_p2Name(uint8_t alg);
float prkDrum_decayTime(uint8_t value);			// PRK_DECAY in seconds (-60 dB)

// --- randomise (UMA 0.29): the same groups and numbers as the FM drums' (FmDrum.h FMD_RND_*) ---
// palettes: which algorithms the tone group draws
enum PrkDrumPalette
{
	PRK_PAL_ALL,	// every algorithm
	PRK_PAL_PRC,	// the noise percussion: rsn slp cym ncr mtn
	PRK_PAL_DRM,	// the tonal drums: fld cpx wav pls rbd tom
	PRK_NUM_PALETTES
};
// what a drawn value sets
enum PrkRandomTarget
{
	PRK_RV_ALG, PRK_RV_VAR, PRK_RV_P1, PRK_RV_P2,	// tone (group 0)
	PRK_RV_ATK, PRK_RV_DEC, PRK_RV_SLOPE,			// the voice's Amp EG (group 1)
	PRK_RV_SWEEP_AMT, PRK_RV_SWEEP_DEC, PRK_RV_SWEEP_SLOPE,	// the voice's pitch sweep (group 2)
};
#define PRK_RND_MAX_VALUES 4
typedef struct
{
	uint8_t target;	// PRK_RV_*
	uint8_t value;	// 0-127 (the algorithm and variation as their index)
} PrkRandomValue;
// fills out[] for a group (0 tone, 1 envelope, 2 sweep; 3 fine: none) and returns how many; value 1-127; the same
// seed, value and drum always give the same sound
uint8_t prkDrum_random(uint8_t group, uint8_t seed, uint8_t value, uint8_t drum, uint8_t palette,
	PrkRandomValue out[PRK_RND_MAX_VALUES]);
uint8_t prkDrum_paletteHas(uint8_t palette, uint8_t alg);

#endif /* PRKDRUM_H_ */
