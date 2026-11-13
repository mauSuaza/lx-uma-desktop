/*
 * uma.h - LX_UMA: custom firmware for the Erica Synths LXR-02 drum machine (desktop). Version and feature flags.
 * Included by main.h, so every file sees the same flags. Port of LX_UMA for the LXR Eurorack module
 * (github.com/mauSuaza/LX_UMA); plan in docs/port-plan.md.
 */
#ifndef UMA_H_
#define UMA_H_

// version on the start-up screen and in the update file name (LXR_V2_UMA_V<version>.img, CMakeLists.txt)
#define UMA_VERSION "0.0b"	// Phase 0 test build: 0.0a2 (stock, renamed, CPU readout) + the sector-7 test

// CPU load readout on the global page (SHIFT + step 12, 3rd press), 2nd sub-page, 4th slot `pk`: the longest
// sequencer + sound render in the last 2 s, in % of a 64-sample block's time. The main loop renders
// (soundengine_process); the TIM2 interrupt only during SD saves (USE_INTERRUPT_ONLY_DURING_SAVE_PROJ); both timed
#define UMA_LOAD 1

// test build 0.0b (docs/port-plan.md): a 4 KB pattern at the start of flash sector 7, checked at start-up
// ("S7 ok" / "S7 BAD n" on the start-up screen). 1 = the pattern (0.0b), 2 = its inverse (0.0b2), 0 = off
#define UMA_S7TEST 1

#endif /* UMA_H_ */
