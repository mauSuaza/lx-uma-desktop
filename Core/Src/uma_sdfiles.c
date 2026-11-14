/*
 * uma_sdfiles.c - UMA: files from the SD card for the Plaits models (see uma_sdfiles.h, docs/design/sd-loaders.md).
 *
 * Handing a slot between the main loop and the audio interrupt (one core: the interrupt runs entirely between two
 * steps of the main loop, never during one): the main loop marks a slot empty (file = none) before it writes into
 * it, and marks it with its file only after the data is in and checked. The audio side only reads a slot marked
 * with the file it asks for. The 6-op copies a patch out of the bank inside the interrupt, so a slot it read is free
 * to be replaced as soon as the interrupt returns. A wavetable or terrain reads its slot as it plays: the slot is
 * kept while a voice uses it (umaSdf_setUsing), and if it's ever replaced (every slot wanted or used) the voice sees
 * it gone at its next block (umaSdf_data) before reading it, and plays the built-in table.
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "uma_sdfiles.h"
#include <string.h>

#ifdef UMA_SDF_HOST_TEST
#define UMA_SDF_BARRIER()
#else
#include "stm32f7xx.h"
#define UMA_SDF_BARRIER()	__DMB()
#endif

// ---- the list ----
typedef struct
{
	uint8_t folder;		// 0: the kind's own folder
	char name[13];		// 8.3 as FatFs gives it: "ROM1A.SYX"
} UmaSdfEntry;

static UmaSdfEntry umaSdf_files[UMA_SDF_KINDS][UMA_SDF_MAX];
static char umaSdf_folders[UMA_SDF_KINDS][UMA_SDF_MAX_FOLDERS][9];
static uint8_t umaSdf_fileCount[UMA_SDF_KINDS], umaSdf_folderCount[UMA_SDF_KINDS];
static uint8_t umaSdf_bad[UMA_SDF_KINDS][UMA_SDF_MAX / 8];	// bit set: the file failed its checks
static volatile uint8_t umaSdf_gen;

static const char* const umaSdf_kindFolder[UMA_SDF_KINDS] = {"DX7", "WAVES", "TERRAIN"};
static const uint8_t umaSdf_kindMax[UMA_SDF_KINDS] = {64, 32, 32};

uint8_t umaSdf_max(uint8_t kind) { return kind < UMA_SDF_KINDS ? umaSdf_kindMax[kind] : 0; }

// ---- the pool ----
typedef struct
{
	volatile uint8_t kind, index;	// the file in it; index UMA_SDF_NONE: empty
	volatile uint32_t used;			// when it was last asked for (the oldest is replaced first)
} UmaSdfSlot;

static uint8_t umaSdf_pool[UMA_SDF_SLOTS][UMA_SDF_DATA] __attribute__((aligned(4)));
static UmaSdfSlot umaSdf_slots[UMA_SDF_SLOTS];
static volatile uint8_t umaSdf_wantKind[UMA_SDF_VOICES], umaSdf_wantIndex[UMA_SDF_VOICES];
static volatile uint8_t umaSdf_useKind[UMA_SDF_VOICES], umaSdf_useIndex[UMA_SDF_VOICES] = {
	UMA_SDF_NONE, UMA_SDF_NONE, UMA_SDF_NONE, UMA_SDF_NONE, UMA_SDF_NONE, UMA_SDF_NONE};
static volatile uint32_t umaSdf_clock;

uint8_t umaSdf_count(uint8_t kind) { return kind < UMA_SDF_KINDS ? umaSdf_fileCount[kind] : 0; }
uint8_t umaSdf_generation(void) { return umaSdf_gen; }

static void umaSdf_emptyPool(void)
{
	for(uint8_t s = 0; s < UMA_SDF_SLOTS; s++)
	{
		umaSdf_slots[s].index = UMA_SDF_NONE;
		umaSdf_slots[s].used = 0;
	}
}

void umaSdf_clearList(void)
{
	umaSdf_emptyPool();
	UMA_SDF_BARRIER();
	memset(umaSdf_fileCount, 0, sizeof(umaSdf_fileCount));
	memset(umaSdf_folderCount, 0, sizeof(umaSdf_folderCount));
	memset(umaSdf_bad, 0, sizeof(umaSdf_bad));
	for(uint8_t k = 0; k < UMA_SDF_KINDS; k++)
	{
		umaSdf_folders[k][0][0] = 0;	// folder 0: the kind's own
		umaSdf_folderCount[k] = 1;
	}
	umaSdf_gen++;
}

static void umaSdf_copyName(char* out, const char* in, uint8_t max)
{
	uint8_t i = 0;
	for(; in[i] && i < max; i++) out[i] = in[i];
	out[i] = 0;
}

uint8_t umaSdf_addFolder(uint8_t kind, const char* name)
{
	if(kind >= UMA_SDF_KINDS || umaSdf_folderCount[kind] >= UMA_SDF_MAX_FOLDERS) return 0xff;
	const uint8_t f = umaSdf_folderCount[kind]++;
	umaSdf_copyName(umaSdf_folders[kind][f], name, 8);
	return f;
}

uint8_t umaSdf_addFile(uint8_t kind, uint8_t folder, const char* name)
{
	if(kind >= UMA_SDF_KINDS || umaSdf_fileCount[kind] >= umaSdf_kindMax[kind] || folder >= umaSdf_folderCount[kind]) return 0;
	UmaSdfEntry* e = &umaSdf_files[kind][umaSdf_fileCount[kind]++];
	e->folder = folder;
	umaSdf_copyName(e->name, name, 12);
	return 1;
}

// folder 0 first, then the subfolders by name; within a folder the files by name
static int umaSdf_before(uint8_t kind, const UmaSdfEntry* a, const UmaSdfEntry* b)
{
	if(a->folder != b->folder)
	{
		if(a->folder == 0 || b->folder == 0) return a->folder == 0;
		const int c = strcmp(umaSdf_folders[kind][a->folder], umaSdf_folders[kind][b->folder]);
		if(c) return c < 0;
		return a->folder < b->folder;
	}
	return strcmp(a->name, b->name) < 0;
}

void umaSdf_sortList(uint8_t kind)
{
	if(kind >= UMA_SDF_KINDS) return;
	UmaSdfEntry* f = umaSdf_files[kind];
	for(uint8_t i = 1; i < umaSdf_fileCount[kind]; i++)	// insertion sort: 64 at most
	{
		const UmaSdfEntry e = f[i];
		uint8_t j = i;
		while(j > 0 && umaSdf_before(kind, &e, &f[j - 1])) { f[j] = f[j - 1]; j--; }
		f[j] = e;
	}
}

void umaSdf_name(uint8_t kind, uint8_t index, char* out)
{
	out[0] = 0;
	if(kind >= UMA_SDF_KINDS || index >= umaSdf_fileCount[kind]) return;
	const UmaSdfEntry* e = &umaSdf_files[kind][index];
	uint8_t n = 0;
	if(e->folder)
	{
		for(const char* c = umaSdf_folders[kind][e->folder]; *c && n < 8; c++) out[n++] = *c;
		out[n++] = '/';
	}
	for(const char* c = e->name; *c && *c != '.' && n < 12; c++) out[n++] = *c;
	out[n] = 0;
}

void umaSdf_path(uint8_t kind, uint8_t index, char* out)
{
	out[0] = 0;
	if(kind >= UMA_SDF_KINDS || index >= umaSdf_fileCount[kind]) return;
	const UmaSdfEntry* e = &umaSdf_files[kind][index];
	strcpy(out, "/");
	strcat(out, umaSdf_kindFolder[kind]);
	strcat(out, "/");
	if(e->folder)
	{
		strcat(out, umaSdf_folders[kind][e->folder]);
		strcat(out, "/");
	}
	strcat(out, e->name);
}

// ---- the checks ----
// the file in three parts, as the module reads it (the data straight into its slot): the 6-byte header, the 4,096
// data bytes (masked to 7 bits in place) and the 2-byte trailer
static uint8_t umaSdf_checkSyxParts(const uint8_t* head, uint8_t* data, const uint8_t* tail)
{
	// F0 43 0n 09 20 00 (n: any MIDI channel), 4,096 data bytes, checksum, F7
	if(head[0] != 0xf0 || head[1] != 0x43 || (head[2] & 0xf0) != 0x00 || head[3] != 0x09 || head[4] != 0x20
		|| head[5] != 0x00 || tail[1] != 0xf7)
		return 0;
	uint32_t sum = 0;
	for(uint16_t i = 0; i < UMA_SDF_DATA; i++)
	{
		data[i] &= 0x7f;	// as the DX7: the top bit ignored
		sum += data[i];
	}
	const uint8_t want = (uint8_t)((128 - (sum & 0x7f)) & 0x7f);
	return tail[0] == want || (want == 0 && tail[0] == 128);	// Mutable's editor writes 128 for 0
}

uint8_t umaSdf_checkSyx(const uint8_t* file, uint8_t* data)
{
	memcpy(data, file + 6, UMA_SDF_DATA);
	return umaSdf_checkSyxParts(file, data, file + 6 + UMA_SDF_DATA);
}

void umaSdf_fixBin(uint8_t kind, uint8_t* data)
{
	// a wavetable: 64 map entries, then 15 waves of 132 int16 (any value is a sample). Plaits' LoadUserData clamps
	// a custom wave's number to 15 (min(w - 192, 15)), one past the file's 15: 207 and up would read past them
	if(kind == UMA_SDF_WAVES)
		for(uint8_t i = 0; i < 64; i++)
			if(data[i] > UMA_SDF_WAVE_MAX) data[i] = UMA_SDF_WAVE_MAX;
	// a terrain: any byte is a height (int8)
}

// ---- the pool ----
void umaSdf_setWant(uint8_t voice, uint8_t kind, uint8_t index)
{
	if(voice >= UMA_SDF_VOICES) return;
	umaSdf_wantKind[voice] = kind;
	umaSdf_wantIndex[voice] = kind < UMA_SDF_KINDS ? index : UMA_SDF_NONE;
}

static int8_t umaSdf_find(uint8_t kind, uint8_t index)
{
	for(uint8_t s = 0; s < UMA_SDF_SLOTS; s++)
		if(umaSdf_slots[s].index == index && umaSdf_slots[s].kind == kind) return (int8_t)s;
	return -1;
}

const uint8_t* umaSdf_data(uint8_t kind, uint8_t index)
{
	if(kind >= UMA_SDF_KINDS || index >= umaSdf_fileCount[kind]) return 0;
	const int8_t s = umaSdf_find(kind, index);
	if(s < 0) return 0;
	umaSdf_slots[s].used = ++umaSdf_clock;
	return umaSdf_pool[s];
}

uint8_t umaSdf_status(uint8_t kind, uint8_t index)
{
	if(kind >= UMA_SDF_KINDS || index >= umaSdf_fileCount[kind]) return UMA_SDF_BAD;
	if(umaSdf_bad[kind][index >> 3] & (1u << (index & 7))) return UMA_SDF_BAD;
	return umaSdf_find(kind, index) >= 0 ? UMA_SDF_READY : UMA_SDF_LOADING;
}

void umaSdf_setUsing(uint8_t voice, uint8_t kind, uint8_t index)
{
	if(voice >= UMA_SDF_VOICES) return;
	umaSdf_useKind[voice] = kind;
	umaSdf_useIndex[voice] = kind < UMA_SDF_KINDS ? index : UMA_SDF_NONE;
}

static uint8_t umaSdf_wanted(uint8_t kind, uint8_t index)
{
	for(uint8_t v = 0; v < UMA_SDF_VOICES; v++)
		if(umaSdf_wantIndex[v] == index && umaSdf_wantKind[v] == kind) return 1;
	return 0;
}

static uint8_t umaSdf_used(uint8_t kind, uint8_t index)
{
	for(uint8_t v = 0; v < UMA_SDF_VOICES; v++)
		if(umaSdf_useIndex[v] == index && umaSdf_useKind[v] == kind) return 1;
	return 0;
}

// the longest unused slot that isn't wanted, and, if allowed, isn't played from; -1 if none
static int8_t umaSdf_oldest(uint8_t keepUsed)
{
	int8_t pick = -1;
	uint32_t oldest = 0xffffffffu;
	for(uint8_t s = 0; s < UMA_SDF_SLOTS; s++)
	{
		const uint8_t k = umaSdf_slots[s].kind, i = umaSdf_slots[s].index;
		if(umaSdf_wanted(k, i) || (keepUsed && umaSdf_used(k, i))) continue;
		if(umaSdf_slots[s].used < oldest)
		{
			oldest = umaSdf_slots[s].used;
			pick = (int8_t)s;
		}
	}
	return pick;
}

// LX_UMA desktop: 1 while a voice wants a file that isn't loaded (the main loop then lets the audio interrupt run)
uint8_t umaSdf_needsLoad(void)
{
	for(uint8_t v = 0; v < UMA_SDF_VOICES; v++)
	{
		const uint8_t kind = umaSdf_wantKind[v], index = umaSdf_wantIndex[v];
		if(kind < UMA_SDF_KINDS && index < umaSdf_fileCount[kind] && umaSdf_status(kind, index) == UMA_SDF_LOADING)
			return 1;
	}
	return 0;
}

uint8_t umaSdf_service(void)
{
	for(uint8_t v = 0; v < UMA_SDF_VOICES; v++)
	{
		const uint8_t kind = umaSdf_wantKind[v], index = umaSdf_wantIndex[v];
		if(kind >= UMA_SDF_KINDS || index >= umaSdf_fileCount[kind]) continue;
		if(umaSdf_status(kind, index) != UMA_SDF_LOADING) continue;
		// a slot: an empty one, else the longest unused one no voice wants or plays from, else (every slot wanted or
		// played from: 6 voices each changing file) one only played from: that voice drops it at its next block
		int8_t pick = -1;
		for(uint8_t s = 0; s < UMA_SDF_SLOTS && pick < 0; s++)
			if(umaSdf_slots[s].index == UMA_SDF_NONE) pick = (int8_t)s;
		if(pick < 0) pick = umaSdf_oldest(1);
		if(pick < 0) pick = umaSdf_oldest(0);
		if(pick < 0) return 0;	// every slot wanted (6 voices each wanting a new file while playing from 6 others)
		UmaSdfSlot* slot = &umaSdf_slots[pick];
		slot->index = UMA_SDF_NONE;		// empty before writing: the audio side won't read it meanwhile
		UMA_SDF_BARRIER();
		const uint8_t gen = umaSdf_gen;
		const uint8_t r = umaSdf_readFile(kind, index, umaSdf_pool[pick]);
		if(gen != umaSdf_gen) return 0;	// the card was scanned again meanwhile
		if(r == UMA_SDF_READY)
		{
			UMA_SDF_BARRIER();
			slot->kind = kind;
			slot->used = ++umaSdf_clock;
			UMA_SDF_BARRIER();
			slot->index = index;
		}
		else
			umaSdf_bad[kind][index >> 3] |= (uint8_t)(1u << (index & 7));
		return 1;	// one file per pass of the main loop
	}
	return 0;
}

#ifndef UMA_SDF_HOST_TEST
// ---- FatFs: the card ----
#include "fatfs.h"

static FIL umaSdf_file;

// a file of the kind: .SYX of 4,104 bytes (DX7), .BIN of 4,096 (wavetable, terrain)
static uint8_t umaSdf_isKind(uint8_t kind, const FILINFO* f)
{
	const char* dot = strrchr(f->fname, '.');
	const uint8_t syx = kind == UMA_SDF_DX7;
	return dot && !strcmp(dot, syx ? ".SYX" : ".BIN") && f->fsize == (syx ? UMA_SDF_SYX_SIZE : UMA_SDF_BIN_SIZE)
		&& !(f->fattrib & (AM_DIR | AM_HID | AM_SYS));
}

static void umaSdf_listFolder(uint8_t kind, uint8_t folder, const char* path, uint8_t subfolders)
{
	DIR dir;
	FILINFO f;
	if(f_opendir(&dir, path) != FR_OK) return;
	while(f_readdir(&dir, &f) == FR_OK && f.fname[0])
	{
		if(f.fname[0] == '.' || (f.fattrib & (AM_HID | AM_SYS))) continue;	// also the Mac's ._ files' folders
		if(f.fattrib & AM_DIR)
		{
			if(subfolders) umaSdf_addFolder(kind, f.fname);
		}
		else if(f.fname[0] != '_' && umaSdf_isKind(kind, &f))	// (a Mac's ._NAME.SYX shows as _NAME~1.SYX)
			umaSdf_addFile(kind, folder, f.fname);
	}
	f_closedir(&dir);
}

void umaSdf_scan(void)
{
	umaSdf_clearList();
	for(uint8_t k = 0; k < UMA_SDF_KINDS; k++)
	{
		char path[24];
		strcpy(path, "/");
		strcat(path, umaSdf_kindFolder[k]);
		umaSdf_listFolder(k, 0, path, k == UMA_SDF_DX7);	// subfolders: DX7 only
		const uint8_t folders = umaSdf_folderCount[k];
		for(uint8_t s = 1; s < folders; s++)
		{
			char sub[24];
			strcpy(sub, path);
			strcat(sub, "/");
			strcat(sub, umaSdf_folders[k][s]);
			umaSdf_listFolder(k, s, sub, 0);
		}
		umaSdf_sortList(k);
	}
	umaSdf_gen++;
}

uint8_t umaSdf_readFile(uint8_t kind, uint8_t index, uint8_t* data)
{
	char path[40];
	umaSdf_path(kind, index, path);
	if(f_open(&umaSdf_file, path, FA_OPEN_EXISTING | FA_READ) != FR_OK) return UMA_SDF_BAD;
	if(kind != UMA_SDF_DX7)	// a .bin: its 4,096 bytes as they are (a wavetable's map clamped)
	{
		UINT n = 0;
		const uint8_t ok = f_read(&umaSdf_file, data, UMA_SDF_BIN_SIZE, &n) == FR_OK && n == UMA_SDF_BIN_SIZE;
		f_close(&umaSdf_file);
		if(ok) umaSdf_fixBin(kind, data);
		return ok ? UMA_SDF_READY : UMA_SDF_BAD;
	}
	uint8_t head[6], tail[2];
	UINT a = 0, b = 0, c = 0;
	const uint8_t ok = f_read(&umaSdf_file, head, 6, &a) == FR_OK && f_read(&umaSdf_file, data, UMA_SDF_DATA, &b) == FR_OK
		&& f_read(&umaSdf_file, tail, 2, &c) == FR_OK && a == 6 && b == UMA_SDF_DATA && c == 2;
	f_close(&umaSdf_file);
	return ok && umaSdf_checkSyxParts(head, data, tail) ? UMA_SDF_READY : UMA_SDF_BAD;
}
#endif
