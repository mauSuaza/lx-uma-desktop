/*
 * uma_mod.h - UMA: modulation of the UMA settings (PAR_UMA_*). LX_UMA desktop: the Eurorack's (a179390), its
 * settings in the kit's UMA record (uma_kits.h).
 *
 * The mod matrix (ModMatrix_Processor.c) only reaches parameters with a parameterArray entry below 256. The
 * FM Drum settings are read from activeParameters by the engine (DrumVoice.c) and randomise (uma_random.c), so
 * a modulated FM setting is kept here, next to its knob value, and they read it with umaMod_get. Each value
 * carries the mod matrix pass it was set in; when no slot modulates it any more, umaMod_get returns the knob
 * value again within two passes.
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef UMA_MOD_H_
#define UMA_MOD_H_

#include <stdint.h>

// start of a mod matrix pass (CV_process)
void umaMod_newPass(void);
// 1 if the parameter is an FM Drum setting that modulation is kept for here
uint8_t umaMod_isTarget(uint16_t param);
// a mod matrix slot modulates param: as modNode_updateValue, bipolar around the knob value, val 0..1
void umaMod_apply(uint16_t param, float amount, float val);
// a slot's source value for a unipolar slot (polarity uni): the modulation then goes from the knob up by the amount.
// The slots' maths is bipolar (val 0.5 = no change), so this returns 0.5 + 0.5 x the source's 0..1 reach: LFOs,
// Turing Machines, accents their value; the CV jacks their positive voltages; EN the envelope, EI its inverse
float umaMod_unipolar(uint8_t src, float cv);
// the value to use for an FM Drum setting: the modulated one while a slot modulates it, else the knob value
uint8_t umaMod_get(uint16_t param);
// a UMA setting's knob value, morphed towards the morph target kit while the morph (PERF page) is above 0;
// modulation moves around it. Settings that don't morph: the knob value
uint8_t umaMod_knob(uint16_t param);
// 1 while a mod matrix slot modulates the setting
uint8_t umaMod_isModulated(uint16_t param);
// from the main loop: when a drum's engine changes, point its CV slots at the targets of the new engine
void umaMod_tick(void);

// LX_UMA desktop: after a kit (its UMA record) was loaded: every slot's destination again
void umaMod_kitLoaded(void);
#endif /* UMA_MOD_H_ */
