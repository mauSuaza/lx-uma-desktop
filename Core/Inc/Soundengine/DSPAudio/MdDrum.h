/*
 * MdDrum.h - UMA: the drum-machine engine (docs/design/md-engine.md): 30 machines of the TRX, EFM and P-I families
 * from OP-XY-MNMD (MIT, Core/Src/Soundengine/DSPAudio/mnmd), sorted by sound type. Plain C functions over the C++
 * machines (MdDrum.cpp).
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef MDDRUM_H_
#define MDDRUM_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MD_VOICES		7	// Drum 1-3, the snare, cymbal and hi-hat, and one for the test app
#define MD_BLOCK		64	// render works in blocks of this many samples
#define MD_MACHINES		30
#define MD_CONTROLS		8

// sound types: each lists machines of every family
enum MdDrumType { MD_KICK, MD_SNARE, MD_CLAP, MD_HAT, MD_CYMBAL, MD_PERC, MD_NUM_TYPES };

uint8_t mdDrum_typeCount(uint8_t type);					// machines of a type
uint8_t mdDrum_machineOf(uint8_t type, uint8_t index);	// a type's index-th machine (0 .. MD_MACHINES-1)
const char* mdDrum_typeShort(uint8_t type);				// "kik"
const char* mdDrum_typeName(uint8_t type);				// "Kick"

const char* mdDrum_machineShort(uint8_t machine);		// family letter and name: "tbd", "pmt"
const char* mdDrum_machineName(uint8_t machine);		// "TRX BD"
uint8_t mdDrum_controlCount(uint8_t machine);			// controls in use (the rest of the 8 are blank)
const char* mdDrum_controlName(uint8_t machine, uint8_t c);	// the machine's own: "PTCH", "" for a blank slot
const char* mdDrum_controlShort(uint8_t machine, uint8_t c);	// in 3 letters: "ptc"
void mdDrum_defaults(uint8_t machine, uint8_t controls[MD_CONTROLS]);	// a starting sound for the machine
uint8_t mdDrum_typeOf(uint8_t machine);					// the type a machine belongs to
uint8_t mdDrum_family(uint8_t machine);					// 0 TRX, 1 EFM, 2 P-I

// randomise (docs/design/md-engine.md section 6): the same seed, value and drum always draw the same
enum MdDrumPalette { MD_PAL_TYP, MD_PAL_FAM, MD_PAL_ALL, MD_NUM_PALETTES };	// the home machine's type; type and family; any
// rmc: a machine drawn from the palette around the home machine (value 1-127)
uint8_t mdDrum_randomMachine(uint8_t seed, uint8_t value, uint8_t drum, uint8_t palette, uint8_t home);
// rkn: the 8 knobs drawn around a machine's defaults (pitch near its default, the decay mostly short); blank knobs
// keep their default
void mdDrum_randomKnobs(uint8_t seed, uint8_t value, uint8_t drum, uint8_t machine, uint8_t knobs[MD_CONTROLS]);

void mdDrum_init(uint8_t v);
void mdDrum_setMachine(uint8_t v, uint8_t machine);		// takes effect at the next trigger, or now if silent
void mdDrum_setControl(uint8_t v, uint8_t c, uint8_t value);	// 0-127
void mdDrum_setTranspose(uint8_t v, float semitones);	// 0: the machine as voiced (its PTCH sets its pitch)
void mdDrum_trigger(uint8_t v);
void mdDrum_render(uint8_t v, float* out);				// one block of MD_BLOCK samples, about -1..1
uint8_t mdDrum_finished(uint8_t v);						// 1 once the sound has died away

#ifdef __cplusplus
}
#endif

#endif /* MDDRUM_H_ */
