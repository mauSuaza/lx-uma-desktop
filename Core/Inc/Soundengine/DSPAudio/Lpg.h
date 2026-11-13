/*
 * Lpg.h - a low-pass gate with a vactrol envelope, ported to C from Mutable Instruments Plaits
 * (plaits/dsp/fx/low_pass_gate.h, plaits/dsp/envelope.h LPGEnvelope, and its use in plaits/dsp/voice.cc).
 *
 * Copyright 2014-2017 Emilie Gillet (the Plaits original, MIT license, below).
 * Copyright (C) 2026 UMA contributors (the C port and changes: an excite level per hit, an opening control and
 * resonance, timing for 16-sample control blocks at 44.1 kHz).
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without
 * limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
 * Software, and to permit persons to whom the Software is furnished to do so, subject to the following
 * conditions: The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * How it works (Plaits' "ping" mode): a hit drives the vactrol up to the excite level; it then falls in two
 * speeds, quick at the top and a long soft tail at the bottom. The vactrol value v sets the gain (v) and the
 * cutoff of a 2-pole low-pass (0.003 + 0.3 v^4: the brightness falls faster than the volume); the colour mixes
 * unfiltered signal back in, mostly in the tail, opens the cutoff a little and lengthens the tail.
 * Plain C, no hardware code: built into the firmware and the Mac tests.
 */
#ifndef LPG_H_
#define LPG_H_

#include <stdint.h>

#define LPG_BLOCK	16	// samples per control update (lpg_control), as Plaits' 12 at 48 kHz

typedef struct
{
	float vactrol;		// 0..1
	float target;		// the excite level of the hit being struck
	uint8_t rampUp;
	float gain, prevGain;
	float g, r, h;		// filter coefficients for the control block
	float hfBleed;
	float s1, s2;		// filter state
} Lpg;

void lpg_init(Lpg* l);
// a hit: excite 0..1, how high the vactrol is driven (lower: quieter and darker)
void lpg_trigger(Lpg* l, float excite);
// once per LPG_BLOCK samples: decay 0..1 (how long it rings), colour 0..1 (0: full low-pass gate, 1: close to a
// VCA), open 0..1 (how far the cutoff opens; 1 = as Plaits), resonance 0..1 (0 = as Plaits, no peak)
void lpg_control(Lpg* l, float decay, float colour, float open, float resonance, float sampleRate);
// filter and gain LPG_BLOCK (or fewer) samples in place, -1..1
void lpg_process(Lpg* l, float* buf, uint8_t n);
// the vactrol, 0..1 (for tests)
float lpg_level(const Lpg* l);

#endif /* LPG_H_ */
