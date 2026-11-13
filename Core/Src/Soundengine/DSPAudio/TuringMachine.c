/*
 * TuringMachine.c - a looping random sequence, a UMA modulation source (see TuringMachine.h).
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "Soundengine/DSPAudio/TuringMachine.h"

void turing_init(TuringMachine* t, uint64_t reg)
{
	t->reg = reg;
	t->count = 0;
}

void turing_clock(TuringMachine* t, uint8_t length, uint8_t prob, uint8_t div, uint32_t random)
{
	if(div < 1) div = 1;
	if(++t->count < div)
		return;
	t->count = 0;

	if(length < 1) length = 1;
	if(length > TURING_MAX_LENGTH) length = TURING_MAX_LENGTH;
	if(prob > 127) prob = 127;

	// the bit leaving the loop comes back in, or a random bit instead with a chance of (prob/127)^2:
	// squared, so the settings just above 0 change the loop only now and then
	uint64_t bit = (t->reg >> (length - 1)) & 1u;
	if((random & 0xffffu) % (127u * 127u) < (uint32_t)prob * prob)
		bit = (random >> 16) & 1u;
	t->reg = (t->reg << 1) | bit;
}

float turing_value(const TuringMachine* t)
{
	return (t->reg & 0xffu) * (1.f / 255.f);
}
