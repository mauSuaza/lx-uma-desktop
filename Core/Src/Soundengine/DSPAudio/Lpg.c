/*
 * Lpg.c - a low-pass gate with a vactrol envelope, ported to C from Mutable Instruments Plaits (see Lpg.h for
 * the copyright and MIT license notice, and what changed).
 */
#include "Soundengine/DSPAudio/Lpg.h"
#include <math.h>

#define LPG_ATTACK_S	0.002f	// time to reach the excite level (Plaits: a few ms, from the note)
#define LPG_PI			3.14159265f

void lpg_init(Lpg* l)
{
	l->vactrol = 0.f;
	l->target = 1.f;
	l->rampUp = 0;
	l->gain = l->prevGain = 0.f;
	l->g = 0.01f; l->r = 2.5f; l->h = 1.f;
	l->hfBleed = 0.f;
	l->s1 = l->s2 = 0.f;
}

void lpg_trigger(Lpg* l, float excite)
{
	l->target = excite < 0.f ? 0.f : excite > 1.f ? 1.f : excite;
	l->rampUp = 1;
}

float lpg_level(const Lpg* l)
{
	return l->vactrol;
}

void lpg_control(Lpg* l, float decay, float colour, float open, float resonance, float sampleRate)
{
	const float hf = colour;
	const float block = (float)LPG_BLOCK / sampleRate;
	// as Plaits voice.cc: short decay 2^(-96 st * decay), the tail 2^(-72 st * decay + 12 st * colour)
	const float shortDecay = 200.f * block * exp2f(-8.f * decay);
	const float decayTail = 20.f * block * exp2f(-6.f * decay + hf) - shortDecay;

	// ping: rise to the excite level, then let go
	if(l->rampUp)
	{
		l->vactrol += block / LPG_ATTACK_S;
		if(l->vactrol >= l->target)
		{
			l->vactrol = l->target;
			l->rampUp = 0;
		}
	}
	const float input = l->rampUp ? l->vactrol : 0.f;

	// LPGEnvelope::ProcessLP
	const float error = input - l->vactrol;
	const float v2 = l->vactrol * l->vactrol;
	const float v4 = v2 * v2;
	const float tail = 1.f - l->vactrol;
	const float tail2 = tail * tail;
	// rising: 0.6 per Plaits block (0.25 ms), for our block length
	const float rise = 1.f - powf(0.4f, block / 0.00025f);
	float coefficient = error > 0.f ? rise : shortDecay + (1.f - v4) * decayTail;
	if(coefficient > 0.9f) coefficient = 0.9f;	// very short decays: never past the target
	l->vactrol += coefficient * error;
	if(l->vactrol < 0.f) l->vactrol = 0.f;

	l->gain = l->vactrol;
	float f = 0.003f + 0.3f * v4 * open + hf * 0.04f;
	if(f > 0.45f) f = 0.45f;
	l->hfBleed = (tail2 + (1.f - tail2) * hf) * hf * hf;

	// stmlib Svf set_f_q: Plaits uses q 0.4 (no peak); resonance raises it towards 4
	const float q = 0.4f + resonance * resonance * 3.6f;
	l->g = tanf(LPG_PI * f);
	l->r = 1.f / q;
	l->h = 1.f / (1.f + l->r * l->g + l->g * l->g);
}

void lpg_process(Lpg* l, float* buf, uint8_t n)
{
	const float step = n ? (l->gain - l->prevGain) / n : 0.f;
	float gain = l->prevGain;
	for(uint8_t i = 0; i < n; i++)
	{
		gain += step;
		const float s = buf[i] * gain;
		// stmlib Svf, low-pass
		const float hp = (s - l->r * l->s1 - l->g * l->s1 - l->s2) * l->h;
		const float bp = l->g * hp + l->s1;
		l->s1 = l->g * hp + bp;
		const float lp = l->g * bp + l->s2;
		l->s2 = l->g * bp + lp;
		buf[i] = lp + (s - lp) * l->hfBleed;
	}
	l->prevGain = l->gain;
}
