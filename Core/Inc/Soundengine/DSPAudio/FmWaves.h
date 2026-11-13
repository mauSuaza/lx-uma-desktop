/*
 * FmWaves.h - the FM Drum's operator waveform tables (FmWaves.c, written by tools/gen_waves.py).
 *
 * fmw_tables[shape][level]: shape 0 sine, 1 triangle, 2 saw, 3 square, 4 pulse 25 %, 5 bell; level 0..9 keeps
 * 511, 256, 128 ... 2, 1 harmonics, so an operator picks the version without harmonics above Nyquist. 1024 points
 * plus a guard point (= the first), 16-bit, each shape's full version peaking at 1.
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef FMWAVES_H_
#define FMWAVES_H_

#include <stdint.h>

#define FMW_SHAPES	6
#define FMW_LEVELS	10
#define FMW_BITS	10
#define FMW_SIZE	(1 << FMW_BITS)

extern const int16_t fmw_tables[FMW_SHAPES][FMW_LEVELS][FMW_SIZE + 1];

#endif /* FMWAVES_H_ */
