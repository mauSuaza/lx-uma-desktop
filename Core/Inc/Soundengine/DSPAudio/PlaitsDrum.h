/*
 * PlaitsDrum.h - UMA: the pla engine (docs/design/plaits-engine.md): Plaits' synthesis engines (Emilie Gillet, MIT,
 * Core/Src/Soundengine/DSPAudio/mi) driven directly, without Plaits' voice (our voice has the envelope, LPG,
 * transient and velocity). Plain C functions over the C++ engines (PlaitsDrum.cpp).
 *
 * Phase 0 (0.31a): one model, the bass drum.
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef PLAITSDRUM_H_
#define PLAITSDRUM_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PL_VOICES		6		// Drum 1-3, the snare, cymbal and hi-hat (the Mac tests use them too)
#define PL_BLOCK		64		// render works in blocks of this many samples (the voice's block)
#define PL_CHUNK		16		// one engine Render: at most Plaits' kMaxBlockSize (24), and PL_BLOCK / PL_CHUNK whole

// Plaits' own rate: its pitch maths assume the real module's 47872.34 Hz (plaits/dsp/dsp.h kCorrectedSampleRate)
#define PL_PLAITS_FS	47872.34f

// speech (spk): dropped in 0.34a for flash (~20 KB) and RAM (15 KB). To bring it back: 1 here, and the speech sources
// back in Core/CMakeLists.txt (both lists) and tools/fmtest/Makefile (commit of 0.34a has them)
#define PL_SPEECH		0

enum PlaitsModel { PL_MODEL_BASS_DRUM, PL_MODEL_WAVETABLE, PL_MODEL_FM2, PL_MODEL_DX7, PL_MODEL_MODAL, PL_MODEL_STRING,
	PL_MODEL_TERRAIN, PL_MODEL_VA, PL_MODEL_WAVESHAPE, PL_MODEL_PD, PL_MODEL_ADDITIVE, PL_MODEL_CHORDS, PL_MODEL_SPEECH,
	PL_NUM_MODELS };
#define PL_MODEL_NONE	0xff	// an empty bank: silent
enum PlaitsOutput { PL_OUT_OUT, PL_OUT_AUX, PL_OUT_MIX, PL_NUM_OUTPUTS };

// the models in two banks, as the user picks them (bnk, then mdl): drums, tonal
enum PlaitsBank { PL_BANK_DRUMS, PL_BANK_TONAL, PL_NUM_BANKS };
#define PL_BANK_MAX_MODELS	16
uint8_t plaitsDrum_bankCount(uint8_t bank);				// models in a bank (0 while empty)
uint8_t plaitsDrum_modelOf(uint8_t bank, uint8_t index);	// a bank's index-th model, PL_MODEL_NONE if none
const char* plaitsDrum_bankShort(uint8_t bank);			// "drm"
const char* plaitsDrum_bankName(uint8_t bank);			// "Drums"
const char* plaitsDrum_modelShort(uint8_t model);		// "bd", 3 letters at most; "---" for none
const char* plaitsDrum_modelName(uint8_t model);		// "Bass drum", 12 letters at most
const char* plaitsDrum_knobName(uint8_t model, uint8_t knob);	// what HARMONICS (0), TIMBRE (1), MORPH (2) do: "Decay"
const char* plaitsDrum_outputName(uint8_t model, uint8_t output);	// what out / aux / mix are: "Analog"
const char* plaitsDrum_twistName(uint8_t model);			// what TWIST does: "Punch"
uint8_t plaitsDrum_selfEnveloped(uint8_t model);			// 1: the model shapes its own volume (drums)

// the 6-op FM (DX7 patches, docs/design/plaits-6op.md): built in, A-C Plaits' three banks; then the SD card's
// (0.34b; 0.34c moved the user's own four banks to the card: sdcard/DX7/)
#define PL_DX_BUILTIN_BANKS	3
uint8_t plaitsDrum_dxBankCount(void);						// banks to choose from
void plaitsDrum_setDxBank(uint8_t v, uint8_t bank);		// the voice's bank (the patch: HARMONICS, as on Plaits)
uint8_t plaitsDrum_dxPatchOf(float harmonics);			// the patch HARMONICS picks, 0-31 (without hysteresis)
void plaitsDrum_dxPatchName(uint8_t bank, uint8_t patch, char* out);	// the patch's 10-letter name, 0-terminated
const char* plaitsDrum_dxBankShort(uint8_t bank);		// "A"
const char* plaitsDrum_dxBankName(uint8_t bank);		// "Bass", "Plaits 1"
// the voice's card file (0.34d, `fil`): on the 6-op its bank (as setDxBank), on the wavetable and the wave terrain
// 0 the built-in tables, then the SD card's WAVES / TERRAIN files (a file that's gone: built in). Taken at the hit
void plaitsDrum_setFile(uint8_t v, uint8_t file);
uint8_t plaitsDrum_userFileCount(uint8_t model);		// the wavetable's or terrain's choices: 1 + the card's files
const char* plaitsDrum_userFileShort(uint8_t model, uint8_t file);	// "bit", "1", "---" (gone)
const char* plaitsDrum_userFileName(uint8_t model, uint8_t file);	// "Built-in", "GLASS", "No file"
// the sub oscillator on the wavetable and the wave terrain (0.34e, docs/design/plaits-sub.md): fmt's, with its
// settings: level 0-127 (0 off), shape 0-3 (sin tri saw sqr), octave 0-3 (-1..-4), fold 0-127. Other models ignore it
void plaitsDrum_setSub(uint8_t v, uint8_t level, uint8_t shape, uint8_t octave, uint8_t fold);
void plaitsDrum_setGate(uint8_t v, uint8_t held);		// the DX7 key: held while held (a hit alone taps it)
// the 6-op's key mode (docs/design/plaits-6op.md section 7): ptc, the patch's own envelopes (as Plaits, triggered: a
// hit taps the key, Drum 1-3's gate holds it); frz, frozen: no key, MORPH scrubs the patch's timeline, the Amp EG
// shapes the volume (as Plaits with nothing patched). (0.33b's amp mode was dropped in 0.33d: the user preferred these)
enum PlaitsDxKeyMode { PL_DXK_PATCH, PL_DXK_FROZEN, PL_NUM_DXK };
void plaitsDrum_setDxKeyMode(uint8_t v, uint8_t mode);
const char* plaitsDrum_dxKeyShort(uint8_t mode);			// "ptc" "frz"
const char* plaitsDrum_dxKeyName(uint8_t mode);			// "Patch env" "Frozen"

// fs: the rate the voice is played at (REAL_FS on the module; the test app also renders at PL_PLAITS_FS as the
// reference). Pitch follows fs exactly; Plaits' time constants stay those of 47872 Hz (option B, section 3.2)
void plaitsDrum_init(uint8_t v, float fs);
void plaitsDrum_setModel(uint8_t v, uint8_t model);		// takes effect at the next trigger
void plaitsDrum_setKnobs(uint8_t v, float harmonics, float timbre, float morph);	// 0..1, Plaits' three knobs
// TWIST, Plaits Palette's fourth macro (0..1; 0.5 plays exactly as Mutable's Plaits): punch, strike hardness, pluck
// size, phase warp, ratio shift, the 6-op's modulator detune and tone... per model (plaitsDrum_twistName)
void plaitsDrum_setTwist(uint8_t v, float twist);
void plaitsDrum_setOutput(uint8_t v, uint8_t output);	// PL_OUT_*
void plaitsDrum_setPitch(uint8_t v, float hz);			// the note to play, in Hz
float plaitsDrum_noteOf(float hz, float fs);				// the Plaits note (MIDI, float) that plays hz at fs
void plaitsDrum_trigger(uint8_t v, float accent);		// accent 0..1 (Plaits: 0.8 with nothing patched)
void plaitsDrum_render(uint8_t v, float* out);			// one block of PL_BLOCK samples, about -1..1
// 1 once the last hit has died away (under -94 dB for 40 ms) or before the first: render then outputs zeros without
// running the model (the silent skip, 0.32a). The output setting takes effect at the next hit
uint8_t plaitsDrum_silent(uint8_t v);
// 1 if the voice's sounding model shapes its own volume (the drums): the host doesn't apply its amp EG
uint8_t plaitsDrum_playsEnveloped(uint8_t v);
// Plaits' noise source is one generator shared by all voices (stmlib::Random); the Mac tests seed it to compare
// renders sample by sample
void plaitsDrum_seedNoise(uint32_t seed);

#ifdef __cplusplus
}
#endif

#endif /* PLAITSDRUM_H_ */
