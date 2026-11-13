/*
 * uma_lfo.h - UMA: a second LFO (LFO B) per voice, a mod matrix source. The same LFO as the voice's own
 * (lfo.c: frequency, offset, waveform, retrigger, sync), with settings PAR_UMA_LFOB<voice>_FREQ/OFS/WAVE/RTG
 * and PAR_UMA_LFOB<voice>_SYNC on the LFO page's 2nd screen. Sources lB1-lB6 at the end of the source list.
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef UMA_LFO_H_
#define UMA_LFO_H_

#include <stdint.h>

#define UMA_LFOB_FIRST_SOURCE	41	// mod matrix source index of lB1 (after T6D)
#define UMA_LFOB_SOURCES		6

void umaLfoB_init(void);
// once per audio block, next to the voices' own LFOs: settings from the kit, then the next value
void umaLfoB_block(void);
// from voiceControl_processQueue: voice 0-6 is hit (6 = open hi-hat, as voice 6), for retrigger
void umaLfoB_hit(uint8_t voice);
// LFO B restarts on its bar lines (retrigger 7-10: every 1, 2, 4, 8 bars), tick = the clock's 1/16 ticks since it started
void umaLfoB_barTick(uint32_t tick);
// the fold of voice 0-5's own LFO (LFO A, PAR_UMA_LFOA<voice>_FOLD), modulated and morphed
uint8_t umaLfoA_fold(uint8_t voice);
// the value of LFO B of voice 0-5, 0..1
float umaLfoB_value(uint8_t voice);

#endif /* UMA_LFO_H_ */
