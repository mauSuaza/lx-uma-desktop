/*
 * uma_fmvoice.h - LX_UMA: FM Drum and FM Tone on Drum 1-3 (docs/phase1-spec.md, section 11, 0.1d).
 * The Eurorack's voice integration (LX_UMA a179390, DrumVoice.c umaFm_*), without what comes in Phases 2-3
 * (modulation: a setting is the kit's value; the LPG: the fold follows the amp EG). The functions live in flash
 * (noinline): only their calls are in the ITCM voice code.
 */
#ifndef UMA_FMVOICE_H_
#define UMA_FMVOICE_H_
#include <stdint.h>

void umaFmV_init(void);										// initDrumVoice
void umaFmV_trigger(uint8_t voiceNr);						// Drum_trigger
void umaFmV_block(uint8_t voiceNr, float freqHz);			// calcDrumVoiceAsync, with the voice's pitch (pitch EG in)
// calcDrumVoiceSyncBlock: 1 if the drum is on an FM engine and buf holds its block (silence while the amp EG is shut:
// the engine isn't run then), 0 for the LXR oscillators
uint8_t umaFmV_render(uint8_t voiceNr, int16_t* buf, uint8_t size);
// 1 if the drum's engine shapes its own volume (mdk): the amp EG doesn't scale it
uint8_t umaFmV_ownEnv(uint8_t voiceNr);

#endif /* UMA_FMVOICE_H_ */
