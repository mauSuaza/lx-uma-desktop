/*
 * umaModTargets.h - UMA: the mod matrix targets of each voice (docs/design/mod-targets.md).
 *
 * A mod slot's destination (modSlots.h) is an index into its own voice's list here, 0 = off. Drum 1-3 have one
 * list for every engine: each entry belongs to a set of engines (lxr, fmd, fmt); the other engines' entries are
 * kept by a slot but modulate nothing while that engine is off. The LXR's shared list (modTargets, Cc2Text.c)
 * stays for the velocity targets, and for converting kits saved before kit format 16.
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef UMAMODTARGETS_H_
#define UMAMODTARGETS_H_

#include <stdint.h>

// an entry's engines, a set: the bits of the drum engines it belongs to (UMA: a set since the FM Tone groundwork,
// so a target can belong to two engines)
#define UMA_TGT_LXR	1	// drum engine 0, lxr
#define UMA_TGT_FMD	2	// drum engine 1, fmd (FM Drum)
#define UMA_TGT_FMT	4	// drum engine 2, fmt (FM Tone)
#define UMA_TGT_PRK	8	// drum engine 3, prk (UMA 0.29)
#define UMA_TGT_MD	16	// drum engine 4, mdk: the drum machines (UMA 0.29)
#define UMA_TGT_PLA	32	// drum engine 5, pla: Plaits (UMA 0.31)
#define UMA_TGT_FM	(UMA_TGT_FMD | UMA_TGT_FMT)	// both FM engines (their shared settings)
#define UMA_TGT_ALL	(UMA_TGT_LXR | UMA_TGT_FM | UMA_TGT_PRK | UMA_TGT_MD | UMA_TGT_PLA)	// every engine

// voice 0-5 (Drum 1-3, Snare, Cymbal, Hi-hat): the number of entries, off included
uint8_t umaTgt_count(uint8_t voice);
// a mod source by name for the slot's edit screen, at most 12 letters ("TM C Cymbl", "MIDI CC 74"); MC1-MC8 show
// the CC set for them in the global settings
void umaTgt_sourceName(uint8_t src, char* out, uint8_t size);
uint16_t umaTgt_nameIdx(uint8_t voice, uint8_t idx);
uint16_t umaTgt_rawParam(uint8_t voice, uint8_t idx);	// whatever the engine
uint8_t umaTgt_engineTag(uint8_t voice, uint8_t idx);	// UMA_TGT_* bits
// the voice's engine as a tag bit (UMA_TGT_LXR for voices without a choice)
uint8_t umaTgt_voiceEngine(uint8_t voice);
// the edit screen's note for an entry the current engine doesn't use: "lxr only", "fm only", "fmt only", "prk only"
// (8 characters)
const char* umaTgt_engineNote(uint8_t tag);
// 1 if the entry belongs to the voice's current engine
uint8_t umaTgt_isActive(uint8_t voice, uint8_t idx);
// the parameter to modulate, PAR_NONE when off, past the end, or the other engine's entry
uint16_t umaTgt_param(uint8_t voice, uint8_t idx);
// the parameter mod slot s modulates now
uint16_t umaTgt_slotParam(uint8_t slot);
// the menu: from idx, move by steps over the entries of the current engine (and off), stopping at the ends
uint8_t umaTgt_step(uint8_t voice, uint8_t idx, int16_t steps);
// the entry of the voice's list with this parameter, 0 if none
uint8_t umaTgt_find(uint8_t voice, uint16_t param);

#endif /* UMAMODTARGETS_H_ */
