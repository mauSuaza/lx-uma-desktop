/*
 * uma_turing.c - UMA: four Turing Machines per voice as mod matrix sources (see uma_turing.h). From the Eurorack's
 * (LX_UMA a179390) unchanged; on the desktop the clock (CLK 7) is the sequencer's sixteenths (uma_modmatrix.c).
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "main.h"
#include "uma_turing.h"
#include "uma.h"

#if UMA_FM
#include "Parameters.h"
#include "menu/menu.h"
#include "Soundengine/DSPAudio/TuringMachine.h"

static TuringMachine umaTm[UMA_TM_VOICES][UMA_TM_PER_VOICE];
static uint32_t umaTm_rng;

// xorshift32: fast, so a hit never waits for the hardware RNG
static uint32_t umaTm_random(void)
{
	uint32_t x = umaTm_rng;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return umaTm_rng = x;
}

// the kit parameters holding a machine's pattern
static uint8_t* umaTm_regPar(uint8_t v, uint8_t t)
{
	return &activeParameters[UMA_TM_REG_FIRST_PAR + (v * UMA_TM_PER_VOICE + t) * 2];
}

// bytes 2-7 of the pattern (64 steps since UMA 0.31)
static uint8_t* umaTm_regxPar(uint8_t v, uint8_t t)
{
	return &activeParameters[UMA_TM_REGX_FIRST_PAR + (v * UMA_TM_PER_VOICE + t) * UMA_TM_REGX_PARS];
}

static void umaTm_storeReg(uint8_t v, uint8_t t)
{
	const uint64_t reg = umaTm[v][t].reg;
	uint8_t* r = umaTm_regPar(v, t);
	r[0] = (uint8_t)(reg & 0xff);
	r[1] = (uint8_t)(reg >> 8);
	uint8_t* x = umaTm_regxPar(v, t);
	for(uint8_t i = 0; i < UMA_TM_REGX_PARS; i++)
		x[i] = (uint8_t)(reg >> (16 + 8 * i));
}

void umaTm_init(uint32_t seed)
{
	umaTm_rng = seed ? seed : 0x9e3779b9u;
	for(uint8_t v = 0; v < UMA_TM_VOICES; v++)
		for(uint8_t t = 0; t < UMA_TM_PER_VOICE; t++)
			turing_init(&umaTm[v][t], ((uint64_t)umaTm_random() << 32) | umaTm_random());
}

void umaTm_kitLoaded(void)
{
	const uint32_t primask = __get_PRIMASK();
	__disable_irq();	// a hit in the audio block must not step a machine half-way through
	for(uint8_t v = 0; v < UMA_TM_VOICES; v++)
		for(uint8_t t = 0; t < UMA_TM_PER_VOICE; t++)
		{
			const uint8_t* r = umaTm_regPar(v, t);
			const uint8_t* x = umaTm_regxPar(v, t);
			uint64_t reg = (uint64_t)(r[0] | (r[1] << 8));
			for(uint8_t i = 0; i < UMA_TM_REGX_PARS; i++)
				reg |= (uint64_t)x[i] << (16 + 8 * i);
			turing_init(&umaTm[v][t], reg);
		}
	__set_PRIMASK(primask);
}

void umaTm_hit(uint8_t voice)
{
	if(voice >= UMA_TM_VOICES)
		voice = UMA_TM_VOICES - 1;	// the open hi-hat is voice 6 too
	for(uint8_t v = 0; v < UMA_TM_VOICES; v++)
		for(uint8_t t = 0; t < UMA_TM_PER_VOICE; t++)
		{
			const uint8_t* p = &activeParameters[UMA_TM_FIRST_PAR + (v * UMA_TM_PER_VOICE + t) * UMA_TM_PARS_PER_TM];
			if(p[2] == UMA_TM_CLOCK_TICK)
				continue;	// steps on the clock, not on hits
			const uint8_t clock = p[2] ? (uint8_t)(p[2] - 1) : v;	// CLK: 0 = own voice, 1-6 = that voice
			if(clock == voice)
			{
				turing_clock(&umaTm[v][t], p[0], p[1], p[3], umaTm_random());	// LEN PRB DIV
				umaTm_storeReg(v, t);	// the live pattern is in the kit, so saving the kit saves it
			}
		}
}

void umaTm_clockTick(void)
{
	for(uint8_t v = 0; v < UMA_TM_VOICES; v++)
		for(uint8_t t = 0; t < UMA_TM_PER_VOICE; t++)
		{
			const uint8_t* p = &activeParameters[UMA_TM_FIRST_PAR + (v * UMA_TM_PER_VOICE + t) * UMA_TM_PARS_PER_TM];
			if(p[2] == UMA_TM_CLOCK_TICK)
			{
				turing_clock(&umaTm[v][t], p[0], p[1], p[3], umaTm_random());	// LEN PRB DIV
				umaTm_storeReg(v, t);
			}
		}
}

float umaTm_value(uint8_t i)
{
	// sources are T1A T1B T1C T1D, T2A ...: machine i % 4 of voice i / 4
	if(i >= UMA_TM_SOURCES)
		return 0.5f;
	return turing_value(&umaTm[i / UMA_TM_PER_VOICE][i % UMA_TM_PER_VOICE]);
}

#else
void umaTm_init(uint32_t seed) { (void)seed; }
void umaTm_hit(uint8_t voice) { (void)voice; }
void umaTm_clockTick(void) {}
float umaTm_value(uint8_t i) { (void)i; return 0.5f; }
void umaTm_kitLoaded(void) {}
#endif
