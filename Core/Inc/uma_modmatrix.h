/*
 * uma_modmatrix.h - LX_UMA desktop: the mod matrix (the Eurorack's ModMatrix_Processor.c CV_process, without its
 * jacks): 8 slots per voice (modSlots.h), each a source (the Eurorack's numbers, uma_turing.h), an amount, a target
 * of its voice (umaModTargets.h) and a polarity.
 *
 * Sources on the desktop: LFO A, Turing Machines A-D (0.3c), LFO B (0.3b), the envelopes EN / EI, velocity VL, each
 * per voice. The Eurorack's CV inputs, accent jacks and MIDI CC sources have no desktop source: a slot on one does
 * nothing (the menu skips them).
 */
#ifndef UMA_MODMATRIX_H_
#define UMA_MODMATRIX_H_
#include <stdint.h>
#include "Soundengine/DSPAudio/modulationNode.h"
#include "Soundengine/modSlots.h"

extern ModulationNode cvModulators[NUM_MOD_SLOTS];

void umaModMx_init(void);
// once per audio block, after the voices' LFOs and before randomise and the voices read their settings
void umaModMx_process(void);
// 1 if the source exists on the desktop (the menu lists only these)
uint8_t umaModMx_sourceExists(uint8_t src);
// the sequencer: started (step 0 next), and each sixteenth (seq_nextStep)
void umaModMx_seqStart(void);
void umaModMx_seqStep(void);
// voice 0-6 is hit (6: the open hi-hat): LFO B's retrigger
void umaModMx_hit(uint8_t voice);

#endif /* UMA_MODMATRIX_H_ */
