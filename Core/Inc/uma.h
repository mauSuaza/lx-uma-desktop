/*
 * uma.h - LX_UMA: custom firmware for the Erica Synths LXR-02 drum machine (desktop). Version and feature flags.
 * Included by main.h, so every file sees the same flags. Port of LX_UMA for the LXR Eurorack module
 * (github.com/mauSuaza/LX_UMA); plan in docs/port-plan.md.
 */
#ifndef UMA_H_
#define UMA_H_

// version on the start-up screen and in the update file name (LXR_V2_UMA_V<version>.img, CMakeLists.txt)
#define UMA_VERSION "0.3a"	// Phase 3 test build: the mod slots (LFO A, envelopes, velocity), the mod view

// UMA's engines and settings (the Eurorack's flag: the copied code is written around it)
#define UMA_FM 1
// the Plaits engine as an eng value (0.2e)
#define UMA_PLAITS 1

// CPU load readout on the global page (SHIFT + step 12, 3rd press), 2nd sub-page, 4th slot `pk`: the longest
// sequencer + sound render in the last 2 s, in % of a 64-sample block's time. The main loop renders
// (soundengine_process); the TIM2 interrupt only during SD saves (USE_INTERRUPT_ONLY_DURING_SAVE_PROJ); both timed
#define UMA_LOAD 1
// test build 0.2g: the selected voice's peak load (settings + sound) as `vc` on the CPU screen, in tou's place
#define UMA_VOICE_LOAD 0

// test build 0.0c (docs/port-plan.md): the engines linked in, without menus; Drum 1 FM Tone, Drum 2 Plaits
// wavetable, Drum 3 an mdk kick, fixed settings (Core/Src/uma_enginetest.c)
#define UMA_ENGINE_TEST 0

// test build 0.1b (docs/phase1-spec.md, section 10): saving a kit stamps its UMA record with its number + 1;
// the playing kit's stamp shows as `uk` on the CPU screen (in tou's place), to check that records follow their kits
#define UMA_KIT_TEST 0

// test build 0.1a (docs/phase1-spec.md, section 3): reads a kit file from the card twice a second while the CPU
// readout is shown, the audio from the interrupt; the longest read of the last 2 s replaces tou as `rd` (0.1 ms)
#define UMA_SDREAD_TEST 0

// test build 0.0b (docs/port-plan.md): a 4 KB pattern at the start of flash sector 7, checked at start-up
// ("S7 ok" / "S7 BAD n" on the start-up screen). 1 = the pattern (0.0b), 2 = its inverse (0.0b2), 0 = off
#define UMA_S7TEST 0

#endif /* UMA_H_ */
