/*
 * uma.h - LX_UMA: custom firmware for the Erica Synths LXR-02 drum machine (desktop). Version and feature flags.
 * Included by main.h, so every file sees the same flags. Port of LX_UMA for the LXR Eurorack module
 * (github.com/mauSuaza/LX_UMA); plan in docs/port-plan.md.
 */
#ifndef UMA_H_
#define UMA_H_

// version on the start-up screen and in the update file name (LXR_V2_UMA_V<version>.img, CMakeLists.txt)
#define UMA_VERSION "0.0a"	// Phase 0 test build: stock desktop firmware, renamed, with the CPU readout

// CPU load readout on the global page (SHIFT + LOAD/SAVE), 2nd sub-page, 4th slot `pk`: the highest load of the
// audio interrupt (sequencer + sound + MIDI) in the last 2 s, in % of a 64-sample block's time
#define UMA_LOAD 1

#endif /* UMA_H_ */
