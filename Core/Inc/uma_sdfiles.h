/*
 * uma_sdfiles.h - UMA: files from the SD card for the Plaits models (0.34, docs/design/sd-loaders.md): DX7 banks
 * (/DX7 and one level of subfolders, 0.34b), wavetables (/WAVES) and wave terrains (/TERRAIN) (0.34d).
 *
 * The folders are listed at power-on and when the card goes back in (names only). The files' contents load into a
 * pool of 4 KB slots from the main loop (umaSdf_service), never in the audio interrupt: each voice says which file
 * it wants (umaSdf_setWant), and the audio side takes a file once it's loaded (umaSdf_data; 0 while loading).
 * A wavetable or terrain plays straight from its slot: the voice says so (umaSdf_setUsing), and that slot is
 * replaced only when no other is free; the voice checks its slot each block (umaSdf_data) and drops it if it's gone.
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef UMA_SDFILES_H_
#define UMA_SDFILES_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { UMA_SDF_DX7, UMA_SDF_WAVES, UMA_SDF_TERRAIN, UMA_SDF_KINDS };
#define UMA_SDF_NONE		0xff
#define UMA_SDF_MAX			64		// files per kind at most: 64 DX7 banks, 32 wavetables, 32 terrains (umaSdf_max)
#define UMA_SDF_MAX_FOLDERS	16		// the kind's folder and up to 15 subfolders
#define UMA_SDF_SLOTS		6		// the pool: 6 x 4 KB (LX_UMA desktop: 10 on the Eurorack; docs/SOURCES.md)
#define UMA_SDF_DATA		4096	// a file's data (a .syx without its 6-byte header and 2-byte trailer)
#define UMA_SDF_SYX_SIZE	4104	// a 32-voice DX7 bulk dump
#define UMA_SDF_BIN_SIZE	4096	// a wavetable (an 8 x 8 map + 15 waves) or a terrain (64 x 64 heights)
#define UMA_SDF_WAVE_MAX	206		// a wavetable map's largest entry (191 Plaits' waves, then the file's 15)
#define UMA_SDF_VOICES		6

// what a file is doing
enum { UMA_SDF_LOADING, UMA_SDF_READY, UMA_SDF_BAD };

// the list (any context)
uint8_t umaSdf_count(uint8_t kind);
uint8_t umaSdf_max(uint8_t kind);	// how many files of the kind are listed at most
void umaSdf_name(uint8_t kind, uint8_t index, char* out);	// "ROMS/ROM1A" (12 + 1 bytes at most; 8.3, no extension)
uint8_t umaSdf_generation(void);	// changes at every scan: what a position means may have changed

// the contents
void umaSdf_setWant(uint8_t voice, uint8_t kind, uint8_t index);	// UMA_SDF_NONE: nothing (audio side, every block)
void umaSdf_setUsing(uint8_t voice, uint8_t kind, uint8_t index);	// the file the voice plays from (its slot is kept)
const uint8_t* umaSdf_data(uint8_t kind, uint8_t index);	// the file's 4,096 bytes once loaded, else 0
uint8_t umaSdf_status(uint8_t kind, uint8_t index);			// UMA_SDF_LOADING / READY / BAD

// main loop
void umaSdf_scan(void);		// list the folders (power-on, card back in); empties the pool
uint8_t umaSdf_service(void);	// load one wanted file that isn't loaded yet; 1 if it did (good or bad)
uint8_t umaSdf_needsLoad(void);	// LX_UMA desktop: 1 while there is one to load

// the checks (pure, for the tests): a .syx file's 4,104 bytes -> its 4,096 data bytes; 1 if valid
uint8_t umaSdf_checkSyx(const uint8_t* file, uint8_t* data);
// a .bin's 4,096 bytes, in place: a wavetable's map entries above 206 become 206 (Plaits would read past the file)
void umaSdf_fixBin(uint8_t kind, uint8_t* data);

// the list and file access, below the logic: FatFs on the module, the tests' own on the Mac
void umaSdf_clearList(void);
uint8_t umaSdf_addFolder(uint8_t kind, const char* name);	// a subfolder (8 letters at most); its number, 0xff if full
uint8_t umaSdf_addFile(uint8_t kind, uint8_t folder, const char* name);	// "ROM1A.SYX"; 1 if added (umaSdf_max)
void umaSdf_sortList(uint8_t kind);	// folder 0 first, then the subfolders in name order, files in name order
// read the index-th file of a kind and check it: data gets its 4,096 bytes; returns READY or BAD
uint8_t umaSdf_readFile(uint8_t kind, uint8_t index, uint8_t* data);
void umaSdf_path(uint8_t kind, uint8_t index, char* out);	// "/DX7/ROMS/ROM1A.SYX", "/WAVES/GLASS.BIN" (40 bytes)

#ifdef __cplusplus
}
#endif

#endif /* UMA_SDFILES_H_ */
