/*
 * uma_kits.h - LX_UMA: the kits' UMA settings (docs/phase1-spec.md, section 3, option A).
 *
 * The desktop keeps the stock part of all 64 kits in RAM (mainParameters). UMA's part of a kit (UMA_KIT_BYTES) is
 * kept in RAM only for the kit being played and edited (activeParameters, from PAR_UMA_BLOCK) and for the next one
 * (the prefetch). The working copy of every kit's UMA part is the file UMA_KIT_STORE at the card's root: a 1 KB
 * record per kit, filled when a project loads (from each kit file's appended part), written when a kit is saved, and
 * read back into the kit files when the project is saved. It is outside /tmp, which a project save deletes.
 *
 * Kit files: the stock 255 bytes (name + settings), then the UMA record. A stock firmware reads only the first 255.
 */
#ifndef UMA_KITS_H_
#define UMA_KITS_H_
#include <stdint.h>
#include "ff.h"

#define UMA_KIT_BYTES		1024	// a kit's UMA record, in RAM and on the card (the settings use ~800 by Phase 3)
#define UMA_KIT_FORMAT		2		// record byte 0: the format; conversions of older records go by it
										// 1: 0.1b, only the test stamp. 2: 0.1c, the FM settings
#define UMA_KIT_STORE		"/UMAKITS.DAT"
#define UMA_KIT_NONE		0xff

// record layout (grows with the phases; PAR_UMA_* numbers are PAR_UMA_BLOCK + the offset)
#define UMA_REC_FORMAT		0
#define UMA_REC_STAMP		1		// test 0.1b only: the kit number + 1 when it was last saved (0: never)
#define UMA_REC_PARS		2		// the settings start here (uma_params.h)

extern volatile uint8_t umaKit_deferred;	// set around a kit load from the audio path (sequencer, MIDI bank change)

void umaKit_onKitLoad(uint8_t kit);			// preset_loadDrumsetRam, after the stock part
void umaKit_store(uint8_t kit);				// preset_saveDrumsetRam: the edit buffer's record into the store
void umaKit_fromFile(FIL* f, uint8_t kit);	// after a kit file's stock part was read: its record into the store
void umaKit_defaults(uint8_t kit);			// a kit slot without a file: a default record into the store
void umaKit_toFile(FIL* f, uint8_t kit);	// after a kit file's stock part was written: the store's record after it
void umaKit_activeToFile(FIL* f);			// tmp.snd: the edit buffer's record
void umaKit_activeFromFile(FIL* f);			// tmp.snd
void umaKit_service(void);					// main loop: prefetch the next kit's record, apply a late one

#endif /* UMA_KITS_H_ */
