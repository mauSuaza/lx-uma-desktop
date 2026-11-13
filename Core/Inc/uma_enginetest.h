/*
 * uma_enginetest.h - LX_UMA Phase 0 test 0.0c (docs/port-plan.md): the engines in, without menus.
 * Behind UMA_ENGINE_TEST (uma.h), Drum 1 plays FM Tone, Drum 2 the Plaits wavetable, Drum 3 an mdk kick, with fixed
 * settings, in place of the voice's two oscillators (the transient, filter, amp EG and drive stay). The functions
 * live in flash (noinline): only their calls are added to the ITCM voice code.
 */
#ifndef UMA_ENGINETEST_H_
#define UMA_ENGINETEST_H_
#include <stdint.h>

void umaTest_init(void);								// end of initDrumVoice
void umaTest_trigger(uint8_t voiceNr);					// Drum_trigger
void umaTest_block(uint8_t voiceNr, float freqHz);		// calcDrumVoiceAsync, with the voice's pitch
uint8_t umaTest_render(uint8_t voiceNr, int16_t* buf, uint8_t size);	// 1: buf holds the engine's block
uint8_t umaTest_selfEnveloped(uint8_t voiceNr);			// 1: the engine shapes its own volume (the amp EG is skipped)

#endif /* UMA_ENGINETEST_H_ */
