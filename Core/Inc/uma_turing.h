/*
 * uma_turing.h - UMA: four Turing Machines per voice as mod matrix sources (TuringMachine.c).
 *
 * Settings per machine: kit parameters PAR_UMA_TM<voice><A-D>_LEN/PRB/CLK/DIV. A machine steps on the hits
 * of its clock voice (CLK: 0 = its own voice, 1-6 = that voice), or on the 1/16 ticks of the OP HH clock
 * (CLK 7 = clk, uma_clock.h), every DIV-th one. Sources T1A-T6D in the
 * mod matrix (cvSrcNames, CV_process).
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef UMA_TURING_H_
#define UMA_TURING_H_

#include <stdint.h>

#define UMA_TM_FIRST_SOURCE	17	// mod matrix source index of T1A (after cv1-5, lf1-6, ac1-6)
#define UMA_TM_SOURCES		24	// T1A T1B T1C T1D, T2A ... T6D: by voice
#define UMA_ENV_FIRST_SOURCE	(UMA_TM_FIRST_SOURCE + UMA_TM_SOURCES + 6)	// EN1 (after LFO B): EN1-EN6, then EI1-EI6
#define UMA_VEL_FIRST_SOURCE	(UMA_ENV_FIRST_SOURCE + 12)	// VL1 (after EI6): each voice's last hit velocity (UMA 0.29)
#define UMA_MC_FIRST_SOURCE	(UMA_VEL_FIRST_SOURCE + 6)	// MC1 (after VL6): the MIDI CC sources (uma_cc.h)
#define UMA_NUM_MOD_SOURCES	(UMA_MC_FIRST_SOURCE + 8)	// the length of cvSrcNames

// at start-up: random start patterns
void umaTm_init(uint32_t seed);
// from voiceControl_processQueue, before the voice is triggered: voice 0-6 (6 = open hi-hat, as voice 6)
void umaTm_hit(uint8_t voice);
#define UMA_TM_CLOCK_TICK	7	// CLK: the OP HH clock's 1/16 ticks

// from the audio block: a 1/16 tick of the OP HH clock
void umaTm_clockTick(void);
// the value of mod matrix source index UMA_TM_FIRST_SOURCE + i, 0..1
float umaTm_value(uint8_t i);
// after a kit was loaded into activeParameters: the machines take its saved patterns. Every step writes the
// pattern back (PAR_UMA_TM*_REG_*), so saving the kit saves them
void umaTm_kitLoaded(void);

#endif /* UMA_TURING_H_ */
