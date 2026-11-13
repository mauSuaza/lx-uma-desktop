/*
 * uma_voice.c - UMA: prk and mdk on the snare, cymbal and hi-hat, and where every voice's engine settings are
 * (uma_voice.h, docs/design/voice-engines.md).
 *
 * LX_UMA desktop (0.2d): the Eurorack's file (LX_UMA a179390) with prk compiled out (UMA_PRK 0: no prk engine on the
 * desktop, its instances would take 25 KB of RAM) and the oscillator's osc_getFreq without a CV input (Oscillator.c).
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "uma_voice.h"
#include "uma.h"

#if UMA_FM
#include <math.h>
#include "Parameters.h"
#include "menu/menu.h"	// activeParameters
#include "uma_mod.h"
#include "uma_random.h"
#include "uma_modmatrix.h"
#include "Soundengine/config.h"
#include "Soundengine/DSPAudio/Snare.h"
#include "Soundengine/DSPAudio/CymbalVoice.h"
#include "Soundengine/DSPAudio/HiHat.h"
#include "Soundengine/DSPAudio/FmDrum.h"
#if UMA_PRK
#include "Soundengine/DSPAudio/PrkDrum.h"
#endif
#include "Soundengine/DSPAudio/MdDrum.h"
#include "Soundengine/DSPAudio/PlaitsDrum.h"
#include "Soundengine/DSPAudio/BufferTools.h"
#include "uma_env.h"	// the drums' gate (the 6-op FM's key)

#ifndef UMA_PRK
#define UMA_PRK 0
#endif


// the Plaits engine's settings, on every voice (Drum 1-3 call it from DrumVoice.c)
uint8_t umaVoice_plaSync(uint8_t voice)
{
	if(voice >= UMA_VOICES) return 0;
	const uint8_t bank = umaMod_get(umaVoice_plaPar(voice, 0));
	plaitsDrum_setModel(voice, plaitsDrum_modelOf(bank, umaMod_get(umaVoice_plaPar(voice, 1))));
	plaitsDrum_setKnobs(voice, umaMod_get(umaVoice_plaPar(voice, 2)) * (1.f / 127.f),
		umaMod_get(umaVoice_plaPar(voice, 3)) * (1.f / 127.f), umaMod_get(umaVoice_plaPar(voice, 4)) * (1.f / 127.f));
	// TWIST (Plaits Palette's fourth macro): stored 0-127 with noon at 63, exactly 0.5 there (Mutable's sound, bit
	// for bit: noon is set as 0.5 itself, never left to the rounding of the scaling)
	const uint8_t twi = umaMod_get(umaVoice_plaPar(voice, 8));
	plaitsDrum_setTwist(voice, twi == UMA_PLA_TWI_NOON ? 0.5f
		: twi < UMA_PLA_TWI_NOON ? twi * (0.5f / UMA_PLA_TWI_NOON)
		: 0.5f + (float)(twi - UMA_PLA_TWI_NOON) * (0.5f / (127 - UMA_PLA_TWI_NOON)));
	plaitsDrum_setOutput(voice, umaMod_get(umaVoice_plaPar(voice, 5)));
	// `fil`: the file of the voice's model (the 6-op's bank, the wavetable's or terrain's file; uma_mod.c reads the
	// model's own setting); a kit's card file that's gone (files removed): bank A, the built-in tables (shown as ---)
	plaitsDrum_setFile(voice, umaMod_get(umaVoice_plaPar(voice, 6)));
	// the sub on the wavetable and terrain (0.34e)
	plaitsDrum_setSub(voice, umaMod_get(umaVoice_plaPar(voice, 11)), umaMod_get(umaVoice_plaPar(voice, 12)),
		umaMod_get(umaVoice_plaPar(voice, 13)), umaMod_get(umaVoice_plaPar(voice, 14)));
	// the DX7 key: in patch mode Drum 1-3's gate (jack or MIDI), a hit alone taps it; frozen has none
	const uint8_t kmd = umaMod_get(umaVoice_plaPar(voice, 7));
	plaitsDrum_setDxKeyMode(voice, kmd);
	plaitsDrum_setGate(voice, kmd == PL_DXK_PATCH && voice < 3 ? umaEnv_gateHeld(voice) : 0);
	return !plaitsDrum_playsEnveloped(voice);
}

// --- the snare, cymbal and hi-hat ---

#define UMA_VX_N	3
#if UMA_PRK
static PrkDrum umaVoice_prk[UMA_VX_N];
#endif
static FmDrum umaVoice_fold[UMA_VX_N];	// only its wavefolder is used
static volatile uint8_t umaVoice_pending[UMA_VX_N], umaVoice_openHit[UMA_VX_N];
static uint8_t umaVoice_open[UMA_VX_N];	// the sounding hit is the hi-hat's open one
static float umaVoice_lastEg[UMA_VX_N];	// the amp EG's value in the last block

// what the host needs of a voice (the three voices' structs are alike, not the same type)
typedef struct
{
	OscInfo* osc;
	ResonantFilter* filter;
	uint8_t filterType;
	float vol, velo;
	uint8_t volumeMod;
	TransientGenerator* trans;
	float eg;
	Distortion* dist;
} UmaVoiceView;

static UmaVoiceView umaVoice_view(uint8_t voice)
{
	UmaVoiceView w;
#define UMA_VIEW(x) w.osc = &x.osc; w.filter = &x.filter; w.filterType = x.filterType; w.vol = x.vol; w.velo = x.velo; \
	w.volumeMod = x.volumeMod; w.trans = &x.transGen; w.eg = x.egValueOscVol; w.dist = &x.distortion;
	if(voice == 3) { UMA_VIEW(snareVoice) }
	else if(voice == 4) { UMA_VIEW(cymbalVoice) }
	else { UMA_VIEW(hatVoice) }
#undef UMA_VIEW
	return w;
}

void umaVoice_init(void)
{
	for(uint8_t h = 0; h < UMA_VX_N; h++)
	{
#if UMA_PRK
		prkDrum_init(&umaVoice_prk[h]);
		prkDrum_setAmpEnv(&umaVoice_prk[h], 0);	// the voice's amp EG shapes the volume
#endif
		fmDrum_init(&umaVoice_fold[h], REAL_FS);
		mdDrum_init((uint8_t)(3 + h));
		plaitsDrum_init((uint8_t)(3 + h), REAL_FS);
	}
}

void umaVoice_trigger(uint8_t voice, uint8_t isOpen)
{
	if(voice < 3 || voice >= UMA_VOICES) return;
	umaVoice_openHit[voice - 3] = isOpen;
	umaVoice_pending[voice - 3] = 1;	// started in umaVoice_async, after randomise
	umaRnd_hit(voice);
	umaModMx_hit((uint8_t)(voice == 5 && isOpen ? 6 : voice));	// LX_UMA: LFO B's retrigger
}

#if UMA_PRK
// the amp EG's decay in seconds (as the drums', umaRnd_ampDecaySeconds): how long prk's resonators ring
static float umaVoice_ringTime(uint8_t voice)
{
	const uint16_t p = voice == 5 && umaVoice_open[2] ? PAR_VELOD6_OPEN : umaVoice_ampPar(voice, 1);
	const float k = 2.f * 0.999f / (1.f - 0.999f);	// TIME_K_DECAY
	const float v = umaMod_get(p) / 127.f;
	float step = (1.f - ((1.f + k) * v / (1.f + k * v))) * 2.f;
	if(step > 1.f) step = 1.f;
	const float t = step > 0.f ? (1.f / step) * 64.f / REAL_FS : 8.f;
	return t > 8.f ? 8.f : t;
}
#endif

// the folder's settings on the voice's FmDrum, its drive following the amp EG
static void umaVoice_syncFold(uint8_t voice, float eg)
{
	FmDrum* d = &umaVoice_fold[voice - 3];
	static const uint8_t foldParam[4] = {FMD_FOLD, FMD_FOLD_TYPE, FMD_FOLD_SYM, FMD_BODY_LEVEL};
	for(uint8_t j = 0; j < 4; j++)
	{
		uint8_t v = umaMod_get(umaVoice_foldPar(voice, j));
		if(j == 1) v = v ? 1 : 0;
		if(fmDrum_getParam(d, foldParam[j]) != v)
			fmDrum_setParam(d, foldParam[j], v);
	}
	fmDrum_setFoldEnv(d, eg);
}

void umaVoice_async(uint8_t voice)
{
	if(voice < 3 || voice >= UMA_VOICES) return;
	const uint8_t h = (uint8_t)(voice - 3);
	const uint8_t engine = umaVoice_engine(voice);
	if(engine != UMA_ENG_PRK && engine != UMA_ENG_MDK && engine != UMA_ENG_PLA)
	{
		umaVoice_pending[h] = 0;	// a hit on the stock sound doesn't wait
		return;
	}
	const UmaVoiceView w = umaVoice_view(voice);
	const uint8_t starting = umaVoice_pending[h];
	if(starting)
		umaVoice_open[h] = umaVoice_openHit[h];
	umaVoice_syncFold(voice, w.eg);
	// the voice's pitch without its pitch EG: C4 plays the machine as voiced (prk: the pitch itself)
	const float f = osc_getFreq(w.osc) / (w.osc->pitchMod > 0.f ? w.osc->pitchMod : 1.f) * (44100.f / REAL_FS);
	if(engine == UMA_ENG_PLA)
	{
		// Plaits (0.32d): as on Drum 1-3; the pitch in Hz without the pitch EG (the model sweeps its own)
		const uint8_t tonal = umaVoice_plaSync(voice);	// tonal models: with the pitch EG
		plaitsDrum_setPitch(voice, osc_getFreq(w.osc) / (!tonal && w.osc->pitchMod > 0.f ? w.osc->pitchMod : 1.f));
		if(starting) { umaVoice_pending[h] = 0; plaitsDrum_trigger(voice, 0.8f); }
		return;
	}
#if UMA_PRK
	if(engine == UMA_ENG_PRK)
	{
		PrkDrum* d = &umaVoice_prk[h];
		for(uint8_t j = 0; j < 4; j++)
		{
			const uint8_t v = umaMod_get(umaVoice_prkPar(voice, j));
			if(prkDrum_getParam(d, (uint8_t)(PRK_ALG + j)) != v)
				prkDrum_setParam(d, (uint8_t)(PRK_ALG + j), v);
		}
		prkDrum_setEnv(d, w.eg);
		prkDrum_setRingTime(d, umaVoice_ringTime(voice));
		prkDrum_setPitch(d, osc_getFreq(w.osc) * (44100.f / REAL_FS));
		if(starting) { umaVoice_pending[h] = 0; prkDrum_trigger(d, 1.f); }
		return;
	}
#endif
	// mdk: as Drum 1-3 (DrumVoice.c umaMd_syncParams), and the hi-hat's open hit plays the machine's open version
	const uint8_t home = mdDrum_machineOf(activeParameters[umaVoice_mdPar(voice, 0)], activeParameters[umaVoice_mdPar(voice, 1)]);
	const uint8_t drawn = umaRnd_mdMachine(voice);
	uint8_t played = drawn < MD_MACHINES ? drawn : mdDrum_machineOf(umaMod_get(umaVoice_mdPar(voice, 0)), umaMod_get(umaVoice_mdPar(voice, 1)));
	const uint8_t open = voice == 5 && umaVoice_open[h];
	if(open && played == 6) played = 7;	// TRX CH: TRX OH
	const uint8_t md = (uint8_t)(3 + h);
	mdDrum_setMachine(md, played);
	uint8_t defaults[MD_CONTROLS], homeDefaults[MD_CONTROLS];
	mdDrum_defaults(played, defaults);
	mdDrum_defaults(home, homeDefaults);
	for(uint8_t c = 0; c < MD_CONTROLS; c++)
	{
		int v = umaMod_get(umaVoice_mdPar(voice, (uint8_t)(2 + c)));
		if(played != home) v = defaults[c] + v - homeDefaults[c];
		if(open && played != 7)
		{
			const char* l = mdDrum_controlName(played, c);
			if(played == 29 && c == 7) v = 0;	// P-I HH: CLOS released
			else if(played != 29 && l[0] == 'D' && l[1] == 'E' && l[2] == 'C' && !l[3]) v += 40;	// longer
		}
		mdDrum_setControl(md, c, (uint8_t)(v < 0 ? 0 : v > 127 ? 127 : v));
	}
	mdDrum_setTranspose(md, f > 1.f ? 12.f * log2f(f / 261.6256f) : 0.f);
	if(starting) { umaVoice_pending[h] = 0; mdDrum_trigger(md); }
}

void umaVoice_filterBlock(uint8_t voice, int16_t* buf, uint8_t size)
{
	if(voice < 3 || voice >= UMA_VOICES) return;
	const UmaVoiceView w = umaVoice_view(voice);
	ResonantFilter* f = w.filter;
	const int8_t env = (int8_t)(umaMod_get(umaVoice_fenvPar(voice)) - 63);
	if(env == 0)
	{
		SVF_calcBlockZDF(f, w.filterType, buf, size);
		return;
	}
	// as the drums' (uma_env.c): the cutoff moved in octaves by the amp EG, for this block only
	const float amount = (env > 63 ? 63 : env) / 63.f;
	float fc = f->f * exp2f(amount * 6.f * w.eg);
	if(fc > 0.449f) fc = 0.449f;
	if(fc < 0.0002f) fc = 0.0002f;
	const float keepF = f->f, keepG = f->g;
	f->f = fc;
	f->g = tanf(3.14159265f * fc);
	SVF_calcBlockZDF(f, w.filterType, buf, size);
	f->f = keepF;
	f->g = keepG;
}

uint8_t umaVoice_render(uint8_t voice, int16_t* buf, uint8_t size)
{
	if(voice < 3 || voice >= UMA_VOICES) return 0;
	const uint8_t engine = umaVoice_engine(voice);
	if(engine != UMA_ENG_PRK && engine != UMA_ENG_MDK && engine != UMA_ENG_PLA)
		return 0;
	const uint8_t h = (uint8_t)(voice - 3);
	const UmaVoiceView w = umaVoice_view(voice);
	const uint8_t closed = w.eg <= 0.f && umaVoice_lastEg[h] <= 0.f;	// this block and the last
	umaVoice_lastEg[h] = w.eg;
	float b[MD_BLOCK];
	const uint8_t plaTonal = engine == UMA_ENG_PLA && !plaitsDrum_playsEnveloped(voice);
	if((engine == UMA_ENG_PRK || plaTonal) && closed)
		for(uint8_t i = 0; i < MD_BLOCK; i++) b[i] = 0.f;	// the amp EG shut: prk / a tonal model would be silent, isn't run
#if UMA_PRK
	else if(engine == UMA_ENG_PRK)
		prkDrum_render(&umaVoice_prk[h], b, size);
#endif
	else if(engine == UMA_ENG_PLA && size == PL_BLOCK)
		plaitsDrum_render(voice, b);
	else if(engine == UMA_ENG_PLA)
		for(uint8_t i = 0; i < MD_BLOCK; i++) b[i] = 0.f;
	else if(size == MD_BLOCK)
		mdDrum_render((uint8_t)(3 + h), b);
	else
		for(uint8_t i = 0; i < MD_BLOCK; i++) b[i] = 0.f;
	fmDrum_foldBlock(&umaVoice_fold[h], b, size);
	for(uint8_t i = 0; i < size; i++)
	{
		float s = b[i] * 32767.f;
		buf[i] = (int16_t)(s > 32767.f ? 32767.f : s < -32767.f ? -32767.f : s);
	}
	int16_t trans[MD_BLOCK];
	transient_calcBlock(w.trans, trans, size);
	bufferTool_addBuffersSaturating(buf, trans, size);
	umaVoice_filterBlock(voice, buf, size);
	// the volume; the amp EG too, unless the sound sets its own (mdk, Plaits' drums)
	const uint8_t ownEnv = engine == UMA_ENG_MDK || (engine == UMA_ENG_PLA && plaitsDrum_playsEnveloped(voice));
	const float gain = w.vol * (w.volumeMod ? w.velo : 1.f) * (ownEnv ? 1.f : w.eg);
	for(uint8_t i = 0; i < size; i++)
		buf[i] = (int16_t)(buf[i] * gain);
	calcDistBlock(w.dist, buf, size);
	return 1;
}

#else
void umaVoice_init(void) {}
void umaVoice_trigger(uint8_t voice, uint8_t isOpen) { (void)voice; (void)isOpen; }
void umaVoice_async(uint8_t voice) { (void)voice; }
uint8_t umaVoice_render(uint8_t voice, int16_t* buf, uint8_t size) { (void)voice; (void)buf; (void)size; return 0; }
#endif
