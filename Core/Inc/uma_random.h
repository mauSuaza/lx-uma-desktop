/*
 * uma_random.h - UMA randomise for the FM Drum engine (docs/design/fm-drum-engine.md section 5).
 *
 * Five kit parameters per drum 1-3 (PAR_UMA_RND_*): tone, envelopes, sweep and fine re-roll their group of
 * settings when their value changes (0 restores), seed picks the set of random sounds. The random values
 * come from fmDrum_random, the same rules as in the test app. Fine (rfn, UMA 0.20) re-rolls fine ratio A and
 * B: 1-63 only on metallic ratios, 64-127 on every ratio; it is worked out again when tone changes the
 * ratios.
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef UMA_RANDOM_H_
#define UMA_RANDOM_H_

#include <stdint.h>

// in the audio block, after the mod matrix (CV_process) and before the drum voices read their settings
// (calcDrumVoiceAsync): re-rolls or restores groups whose randomise setting changed. A knob change counts
// at once; a modulated value is taken only when the drum was hit in this block, so a re-roll is in place
// before the hit's first sample and never changes a sound mid-hit
void umaRnd_block(void);
// from Drum_trigger, drum 0-2: the drum is hit in this block
void umaRnd_hit(uint8_t drum);
// after a kit was loaded into activeParameters: no re-roll, the kit's values become the restore point
void umaRnd_kitLoaded(void);
// after the kit was saved: the saved sound becomes the restore point ("saving keeps")
void umaRnd_kitSaved(void);
// the drum-machine engine (UMA 0.29): the machine rmc drew for Drum 1-3, 0xff when it plays its own
uint8_t umaRnd_mdMachine(uint8_t drum);

#endif /* UMA_RANDOM_H_ */
