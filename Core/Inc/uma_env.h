/*
 * uma_env.h - UMA: the body envelope's hold and gate mode, and the filter envelope, for Drum 1-3
 * (docs/design/envelopes.md).
 *
 * Kit parameters PAR_UMA_HOLD/SUS/REL/GATE/FENV<drum>: hld (0 = off, up to 2 s), sus and rel (gate mode only),
 * gat (0 trg: attack-hold-decay, 1 gat: attack-hold-decay-sustain-release while a gate is held), env (63 = off,
 * the body envelope moves the cutoff up to 6 octaves either way). The gate is a MIDI note held (LX_UMA desktop:
 * no trigger jacks).
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef UMA_ENV_H_
#define UMA_ENV_H_

#include <stdint.h>

// once per audio block, before the drum's body envelope is calculated: settings, and the end of a gate
void umaEnv_block(uint8_t drum);
// from Drum_trigger, after the envelope was triggered: in gate mode a hit with a gate held sustains
void umaEnv_trigger(uint8_t drum);
// the drum's filter for one block, with the filter envelope (instead of SVF_calcBlockZDF)
void umaEnv_filterBlock(uint8_t drum, int16_t* buf, uint8_t size);
// the filter type value (voiceArray[].filterType) of the low-pass gate: after FILTER_OFF (typ index 8)
#define UMA_FILTER_LPG	9
// 1 while the drum's filter type is lpg: the LPG sets the volume, the amp envelope only modulates
uint8_t umaEnv_lpgOn(uint8_t drum);
// the envelope the FM wavefolder follows: the amp EG, or with typ lpg the LPG's vactrol, 0..1
float umaEnv_foldEnv(uint8_t drum);
// MIDI note on (on = 1) or off for voice 0-6: a held note is a gate for Drum 1-3
void umaEnv_midiNote(uint8_t voice, uint8_t note, uint8_t on);
// 1 while the drum's gate is held (a MIDI note)
uint8_t umaEnv_gateHeld(uint8_t drum);

#endif /* UMA_ENV_H_ */
