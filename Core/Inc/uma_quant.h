/*
 * uma_quant.h - UMA: the scale and chord quantizer, and each voice's octave (docs/design/quantizer.md).
 * MIDI note or CV -> modulation (v/o, coarse and its modulation) + octave -> quantizer -> fine tune, pitch EG.
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef UMA_QUANT_H_
#define UMA_QUANT_H_

#include <stdint.h>

#define UMA_QUANT_VOICES	6		// Drum 1-3, snare, cymbal, hi-hat
#define UMA_QUANT_PARS		3		// per voice: OCT KEY SCL
#define UMA_QUANT_OCT_ZERO	4		// oct is stored 0-8: -4 .. +4
#define UMA_QUANT_OCT_MAX	8
#define UMA_QUANT_KEYS		12
#define UMA_QUANT_SCALES	51		// off, 10 scales, 33 chords, the 7 harmonic-minor chords
#define UMA_QUANT_HYST		0.2f	// a held note stays until the input is this many semitones past the midpoint
#define UMA_QUANT_NONE		(-1000)	// no note held yet

// a scale's notes as a 12-bit mask from its root (bit n: n semitones above); scale 0 (off) is every note
uint16_t umaQuant_mask(uint8_t scale);
const char* umaQuant_scaleShort(uint8_t scale);	// "m9", 3 letters at most
const char* umaQuant_scaleName(uint8_t scale);		// "Minor 9", for the edit screen
const char* umaQuant_keyName(uint8_t key);			// "C#"

// the allowed note (a MIDI note number, whole) nearest to note (a MIDI note number, any fraction) in the scale on
// key; ties go down. *held is the note this oscillator played last (UMA_QUANT_NONE at first): it stays while it is
// within UMA_QUANT_HYST of being the nearest, and is updated
int16_t umaQuant_snap(float note, uint8_t key, uint8_t scale, int16_t* held);

// the voice's settings (with modulation) into its oscillators, every block before their frequency is set
void umaQuant_block(uint8_t voice);

#endif /* UMA_QUANT_H_ */
