/*
 * uma_mod.h - LX_UMA desktop: a stand-in for the Eurorack's modulation layer (uma_mod.c) until Phase 3. The copied
 * engine, voice and randomise code asks it for a setting's value; with no modulation yet that is the kit's value.
 */
#ifndef UMA_MOD_H_
#define UMA_MOD_H_
#include <stdint.h>
extern uint8_t activeParameters[];
static inline uint8_t umaMod_get(uint16_t param) { return activeParameters[param]; }		// the value in effect
static inline uint8_t umaMod_knob(uint16_t param) { return activeParameters[param]; }		// the knob's value
static inline uint8_t umaMod_isModulated(uint16_t param) { (void)param; return 0; }
static inline uint8_t umaMod_isTarget(uint16_t param) { (void)param; return 0; }
#endif /* UMA_MOD_H_ */
