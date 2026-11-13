/*
 * TuringMachine.h - a looping random sequence (after the Music Thing Modular Turing Machine), a UMA
 * modulation source. Plain C with no hardware code, so it builds into the firmware and the Mac tests.
 *
 * A 64-bit shift register (16 until UMA 0.31). On each step it shifts by one; the bit that comes in is the one that left the
 * loop LENGTH steps ago, replaced by a random bit with a chance of (PROB/127)^2. PROB 0: the loop repeats
 * exactly; low values change it now and then; 127: every step is random. The output is the register's 8 newest bits, 0..1.
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef TURINGMACHINE_H_
#define TURINGMACHINE_H_

#include <stdint.h>

#define TURING_MAX_LENGTH 64

typedef struct
{
	uint64_t reg;
	uint8_t count;	// clocks since the last step, for the divider
} TuringMachine;

void turing_init(TuringMachine* t, uint64_t reg);
// one clock (a hit): steps every div-th clock. length 1-64, prob 0-127, div 1-16. random: any 32-bit
// random number, used for the chance and the new bit
void turing_clock(TuringMachine* t, uint8_t length, uint8_t prob, uint8_t div, uint32_t random);
float turing_value(const TuringMachine* t);

#endif /* TURINGMACHINE_H_ */
