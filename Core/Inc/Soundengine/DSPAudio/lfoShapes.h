/*
 * lfoShapes.h - UMA: the LFOs' 15 shapes with a morph through them, a wavefolder after them, and the sync divisions (docs/design/lfo.md).
 * Plain C, no hardware code: built into the firmware and the Mac tests.
 *
 * The waveform setting is a morph 0-127 through the shapes, each with a small flat zone (a detent: the pure
 * shape) and crossfades between neighbours. The first 8 are the LXR's, in the LXR's order.
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef LFOSHAPES_H_
#define LFOSHAPES_H_

#include <stdint.h>

#define LFOS_SHAPES		15	// 16 in UMA 0.27 (with a narrow pulse after 8 stairs, removed in 0.28: it clicked)
#define LFOS_SYNC_RATES	16	// off and 15 divisions

// one shape at phase t (0..1): 0..1. rnd: this cycle's random value, rndPrev: the last cycle's (random shapes)
float lfoShapes_shape(uint8_t shape, float t, float rnd, float rndPrev);
// the morph 0-127 at phase t
float lfoShapes_value(uint8_t morph, float t, float rnd, float rndPrev);
// the two shapes a morph value blends and how far towards the second (0 on a detent), for the display
void lfoShapes_blend(uint8_t morph, uint8_t* first, uint8_t* second, float* amount);
const char* lfoShapes_name(uint8_t shape);	// 3 letters
const char* lfoShapes_longName(uint8_t shape);
// a sync division: quarter notes per LFO cycle (0 = off)
float lfoShapes_syncBeats(uint8_t rate);
// the LFO's phase increment per control block for a division at a tempo; 0 if off or no tempo
uint32_t lfoShapes_syncInc(uint8_t rate, float bpm, float controlRate);
// kits before kit format 22: the LXR's waveform 0-7 to its detent on UMA 0.27's 16-shape morph (then
// lfoShapes_from16); the LXR's 12 sync values to the new list
uint8_t lfoShapes_fromOldWave(uint8_t wave);
// kits before kit format 23: a morph value on UMA 0.27's 16 shapes to the same place on today's 15 (pulse's
// stretch goes to its neighbours)
uint8_t lfoShapes_from16(uint8_t morph);
// the LFO's wavefolder, after the shape: value 0..1, amount 0 = off (exact), 1-24 a light bend fading in, up to
// a hard fold at 127
float lfoShapes_fold(float value, uint8_t amount);
uint8_t lfoShapes_fromOldSync(uint8_t sync);

#endif /* LFOSHAPES_H_ */
