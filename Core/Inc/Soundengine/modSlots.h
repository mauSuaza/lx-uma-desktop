/*
 * modSlots.h - LX_UMA: the mod matrix slots and the kit settings that hold them (the Eurorack's modSlots.h, LX_UMA
 * a179390, on the desktop's record).
 *
 * Slot s (0 .. NUM_MOD_SLOTS-1) belongs to voice s / MOD_SLOTS_PER_VOICE, 8 per voice. On the Eurorack slots 1-3 of a
 * voice are the LXR's PAR_CV_SRC/MOD_AMT/DEST_1-18; the desktop has no CV matrix, so they are UMA settings at the
 * record's end (PAR_UMA_DSK_MS*). Slots 4-8 are the Eurorack's PAR_UMA_MS<voice>_<slot>_SRC/AMT/DEST.
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef MODSLOTS_H_
#define MODSLOTS_H_
#include <stdint.h>
#include "Parameters.h"
#define MOD_SLOTS_LXR_PER_VOICE	3	// the Eurorack's stock slots (here PAR_UMA_DSK_MS*)
#define MOD_SLOTS_PER_VOICE		(MOD_SLOTS_LXR_PER_VOICE + UMA_MS_PER_VOICE)
#define NUM_MOD_SLOTS			(6 * MOD_SLOTS_PER_VOICE)
// field: 0 source, 1 amount, 2 destination
static inline uint16_t modSlot_par(uint8_t s, uint8_t field)
{
	const uint8_t v = (uint8_t)(s / MOD_SLOTS_PER_VOICE), k = (uint8_t)(s % MOD_SLOTS_PER_VOICE);
	if(k >= MOD_SLOTS_LXR_PER_VOICE)
		return (uint16_t)(UMA_MS_FIRST_PAR + (v * UMA_MS_PER_VOICE + k - MOD_SLOTS_LXR_PER_VOICE) * UMA_MS_PARS + field);
	return (uint16_t)(UMA_DSK_MS_FIRST_PAR + (v * MOD_SLOTS_LXR_PER_VOICE + k) * UMA_MS_PARS + field);
}
#define modSlot_src(s)	modSlot_par((s), 0)
#define modSlot_amt(s)	modSlot_par((s), 1)
#define modSlot_dest(s)	modSlot_par((s), 2)
// the slot's polarity: 0 bi, around the knob; 1 uni, from the knob up by the amount
#define modSlot_pol(s)	((uint16_t)(UMA_SLOT_POL_PAR + (s)))

// the slot whose setting (source, amount or destination: field) this is, 0xff if it is none
static inline uint8_t modSlot_of(uint16_t param, uint8_t* field)
{
	if(param >= UMA_DSK_MS_FIRST_PAR && param < UMA_DSK_MS_FIRST_PAR + 6 * MOD_SLOTS_LXR_PER_VOICE * UMA_MS_PARS)
	{
		const uint16_t i = (uint16_t)((param - UMA_DSK_MS_FIRST_PAR) / UMA_MS_PARS);
		*field = (uint8_t)((param - UMA_DSK_MS_FIRST_PAR) % UMA_MS_PARS);
		return (uint8_t)(i / MOD_SLOTS_LXR_PER_VOICE * MOD_SLOTS_PER_VOICE + i % MOD_SLOTS_LXR_PER_VOICE);
	}
	if(param >= UMA_MS_FIRST_PAR && param < UMA_MS_FIRST_PAR + 6 * UMA_MS_PER_VOICE * UMA_MS_PARS)
	{
		const uint16_t i = (uint16_t)((param - UMA_MS_FIRST_PAR) / UMA_MS_PARS);
		*field = (uint8_t)((param - UMA_MS_FIRST_PAR) % UMA_MS_PARS);
		return (uint8_t)(i / UMA_MS_PER_VOICE * MOD_SLOTS_PER_VOICE + MOD_SLOTS_LXR_PER_VOICE + i % UMA_MS_PER_VOICE);
	}
	return 0xff;
}
// the slot whose destination this is, 0xff if none
static inline uint8_t modSlot_ofDest(uint16_t param)
{
	uint8_t f;
	const uint8_t s = modSlot_of(param, &f);
	return s != 0xff && f == 2 ? s : 0xff;
}

#endif /* MODSLOTS_H_ */
