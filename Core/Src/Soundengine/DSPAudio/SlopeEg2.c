/*
 * SlopeEg.c
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




#include "Soundengine/DSPAudio/SlopeEg2.h"
#include "Soundengine/config.h"
#include <math.h>

//--------------------------------------------------
void slopeEg2_init(SlopeEg2* eg)
{
	eg->attack	= 0.01;
	eg->decay 	= 0.01;
	eg->value 	= 0;
	eg->state 	= EG_STOPPED;
	eg->repeat	= 0;
	eg->holdBlocks = eg->holdCnt = 0;
	eg->sustain = 1.0;
	eg->release = 0.01;
	eg->gated = 0;

	slopeEg2_setSlope(eg,0.5f);
}
//--------------------------------------------------
void slopeEg2_trigger(SlopeEg2* eg)
{
	//if no repeat is selected beginn in attack state
	if(!eg->repeat)
	{
		eg->state = EG_A;
	}
	// if repeat is active we want to repeat the decay stage during the attack time
	else
	{
		eg->state = EG_REPEAT;
		//since we have no attack we start with  eg->value = max
		//we use the attack time as a repeat phase decay time
		 eg->value = 1.0;
	}
	eg->repeatCnt = eg->repeat;
	eg->holdCnt = eg->holdBlocks;
	eg->gated = 0;	// the caller sets it for a gated hit
}
//--------------------------------------------------
IN_ITCM_RAM double slopeEg2_calcSlopeValue(double val, double slope)
{
	return (1.0+slope)*val/(1.0+slope*fabs(val));
}
//--------------------------------------------------
IN_ITCM_RAM double slopeEg2_calc(SlopeEg2* eg)
{
	register double val = eg->value;
	switch(eg->state)
	{

	case EG_STOPPED:
		return 0.0;
		break;

	case EG_REPEAT:
		/*
		 *  if the repeat mode is active we use the attack time as a 2nd decay time for the loop part
		 *  the attack phase is looped repeatCnt times
		 */
		if(val>0)
		{
			val -= eg->attack;
		}
		else
		{
			eg->repeatCnt--;
			if(eg->repeatCnt > 0)
			{
				//retrigger the decay phase and decrease repeat counter
				val = 1.0;
			}
			else
			{
				//repeat counter reached zero -> go to real decay stage
				eg->state = eg->holdCnt ? EG_H : EG_D;	// UMA: hold first
				val = 1.0;
			}
		}
		eg->value = val;
		//to get a analogue stile exp eg we need to invert the shape
		return slopeEg2_calcSlopeValue(val,eg->slope);

		break;
	case EG_A:
		val += eg->attack;
		if(val >= 1.0)
		{
			val = 1.0;
			eg->state = eg->holdCnt ? EG_H : EG_D;	// UMA: hold first
		}

		eg->value = val;
		//to get a analogue stile exp eg we need to invert the shape
		return slopeEg2_calcSlopeValue(val,eg->invSlope);
		break;

	case EG_H:	// UMA: hold at full level
		if(eg->holdCnt)
			eg->holdCnt--;
		if(!eg->holdCnt)
			eg->state = EG_D;
		return slopeEg2_calcSlopeValue(val,eg->slope);

	case EG_D:

		if(val>0)
		{
			val -= eg->decay;
			if(eg->gated && val <= eg->sustain)	// UMA gate mode: stay at the sustain level while the gate is held
			{
				val = eg->sustain;
				eg->state = EG_S;
			}
			eg->value = val > 0 ? val : 0;
			return val > 0 ? slopeEg2_calcSlopeValue(val,eg->slope) : 0;
		}
		else
		{
			eg->value = 0;
			return 0;
		}
		break;

	case EG_S:
		return slopeEg2_calcSlopeValue(val,eg->slope);

	case EG_R:	// UMA: after the gate, from wherever it was
		if(val>0)
		{
			val -= eg->release;
			eg->value = val > 0 ? val : 0;
			return val > 0 ? slopeEg2_calcSlopeValue(val,eg->slope) : 0;
		}
		eg->value = 0;
		return 0;
	}
	return 0;
}
//--------------------------------------------------
/*
x = input in [-1..1]
y = output
k = 2*amount/(1-amount);

f(x) = (1+k)*x/(1+k*abs(x))
*/
#define TIME_AMOUNT_DECAY 0.999
#define TIME_AMOUNT_ATTACK 0.99
#define TIME_K_ATTACK (2*TIME_AMOUNT_ATTACK/(1.0-TIME_AMOUNT_ATTACK))
#define TIME_K_DECAY (2*TIME_AMOUNT_DECAY/(1.0-TIME_AMOUNT_DECAY))
double slopeEg2_calcTime(uint8_t data2, double amount)
{
	const double val = (data2)/127.0;
#if USE_BIG_SAMPLE_BUFFER
	//routine is called on ly half as often -> *2
	const float ret = (1.0-((1.0+amount)*val/(1.0+amount*fabs(val)))) * 2.f;
	return ret > 1.f ? 1.f : ret;
#else
	return 1.0-((1.0+amount)*val/(1.0+amount*fabs(val)));
#endif

}
//--------------------------------------------------
IN_ITCM_RAM void slopeEg2_setAttack(SlopeEg2* eg, uint8_t data2, uint8_t isSync)
{
	eg->attack = slopeEg2_calcTime(data2,TIME_K_ATTACK);
	if(isSync)
		eg->attack /= 16;
};
//--------------------------------------------------
IN_ITCM_RAM void slopeEg2_setDecay(SlopeEg2* eg, uint8_t data2, uint8_t isSync)
{
	eg->decay = slopeEg2_calcTime(data2,TIME_K_DECAY);
	if(isSync)
			eg->decay /= 16;
};
//--------------------------------------------------
double slopeEg2_calcDecay(uint8_t data2)
{
	return slopeEg2_calcTime(data2,TIME_K_DECAY);
};
//--------------------------------------------------
void slopeEg2_setHold(SlopeEg2* eg, uint16_t blocks)
{
	eg->holdBlocks = blocks;
}
//--------------------------------------------------
// the value whose curved output is the level: the inverse of slopeEg2_calcSlopeValue on 0..1
void slopeEg2_setSustain(SlopeEg2* eg, float level)
{
	const double y = level < 0.f ? 0.0 : level > 1.f ? 1.0 : level;
	const double k = eg->slope;
	const double d = 1.0 + k - k * y;
	eg->sustain = d > 1e-9 ? y / d : 1.0;
	if(eg->sustain > 1.0) eg->sustain = 1.0;
}
//--------------------------------------------------
void slopeEg2_setRelease(SlopeEg2* eg, uint8_t data2)
{
	eg->release = slopeEg2_calcTime(data2,TIME_K_DECAY);
}
//--------------------------------------------------
void slopeEg2_gateEnd(SlopeEg2* eg)
{
	if(!eg->gated)
		return;
	eg->gated = 0;
	if(eg->state != EG_STOPPED)
		eg->state = EG_R;
}
//--------------------------------------------------
IN_ITCM_RAM void slopeEg2_setSlope(SlopeEg2* eg, uint8_t data2)
{
	const double amount 	 	= ((data2/127.f)-0.5f)*1.999f;
	//negate the amount
	const double invAmount 	= -amount;

	eg->slope 		= 2*amount/(1-amount);
	eg->invSlope	= 2*invAmount/(1-invAmount);

};
