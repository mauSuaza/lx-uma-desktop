/*
 * SlopeEg2.h
 *
 *  Created on: 16.06.2012
 *
 * Copyright (C) 2012-2026 Erica Synths
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * This file is part of the LXR firmware. It is licensed under the GNU General
 * Public License version 3, with the additional permission and terms in
 * NOTICE.md. See LICENSE and NOTICE.md in the root of the repository.
 */


#ifndef SLOPEEG2_H_
#define SLOPEEG2_H_



#include "stm32f7xx_hal.h"


#define EG_STOPPED 	0
#define EG_A		1
#define EG_D		2
#define EG_REPEAT	3
// UMA (Drum 1-3): hold, and in gate mode sustain and release
#define EG_H		4
#define EG_S		5
#define EG_R		6

typedef struct SLOPE_EG2_Struct
{
	double 	attack;
	double 	decay;
	double 	slope;
	double 	invSlope;
	uint8_t repeat; 				/**<number of repetitions of the attack phase*/
	uint8_t repeatCnt;				/**<a counter for the already played repeats*/
	double 	value;
	uint8_t state;

	// UMA: all 0 = the LXR's envelope as it was
	uint16_t holdBlocks;	// hold at full level after the attack, in calls of slopeEg2_calc (audio blocks)
	uint16_t holdCnt;
	double	sustain;		// gate mode: the decay stops here (in the same units as value) while the gate is held
	double	release;		// gate mode: per call after the gate ends, like decay
	uint8_t	gated;			// this hit is held by a gate: decay stops at sustain, slopeEg2_gateEnd starts the release

} SlopeEg2;


void slopeEg2_init(SlopeEg2* eg);
void slopeEg2_trigger(SlopeEg2* eg);
double slopeEg2_calc(SlopeEg2* eg);
void slopeEg2_setAttack(SlopeEg2* eg, uint8_t data2, uint8_t isSync);
void slopeEg2_setDecay(SlopeEg2* eg, uint8_t data2, uint8_t isSync);
void slopeEg2_setSlope(SlopeEg2* eg, uint8_t data2);
double slopeEg2_calcDecay(uint8_t data2);
// UMA: hold in calls of slopeEg2_calc; gate mode: sustain as an output level 0..1 (on the envelope's curve) and
// release like slopeEg2_setDecay. gated: this hit sustains until slopeEg2_gateEnd (set after slopeEg2_trigger)
void slopeEg2_setHold(SlopeEg2* eg, uint16_t blocks);
void slopeEg2_setSustain(SlopeEg2* eg, float level);
void slopeEg2_setRelease(SlopeEg2* eg, uint8_t data2);
void slopeEg2_gateEnd(SlopeEg2* eg);


#endif /* SLOPEEG2_H_ */
