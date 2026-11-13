/*
 * uma.h - LX_UMA: custom firmware for the Erica Synths LXR-02 drum machine (desktop). Version and feature flags.
 * Included by main.h, so every file sees the same flags. Port of LX_UMA for the LXR Eurorack module
 * (github.com/mauSuaza/LX_UMA); plan in docs/port-plan.md.
 */
#ifndef UMA_H_
#define UMA_H_

// version on the start-up screen and in the update file name (LXR_V2_UMA_V<version>.img, CMakeLists.txt)
#define UMA_VERSION "0.0a2"	// Phase 0 test build: stock desktop firmware, renamed, with the CPU readout (0.0a2: timed in the main loop)

// CPU load readout on the global page (SHIFT + step 12, 3rd press), 2nd sub-page, 4th slot `pk`: the longest
// sequencer + sound render in the last 2 s, in % of a 64-sample block's time. The main loop renders
// (soundengine_process); the TIM2 interrupt only during SD saves (USE_INTERRUPT_ONLY_DURING_SAVE_PROJ); both timed
#define UMA_LOAD 1

#endif /* UMA_H_ */
