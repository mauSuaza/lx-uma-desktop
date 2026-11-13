/*
 * uma_kits.c - LX_UMA: the kits' UMA settings (uma_kits.h).
 *
 * Card access needs the audio to run from the TIM2 interrupt meanwhile, as the stock saves do
 * (USE_INTERRUPT_ONLY_DURING_SAVE_PROJ). The interrupt's sequencer may load a kit while the main loop reads the card:
 * it only uses the prefetch buffer or queues the kit (umaKit_pending); the main loop reads into its own buffer and
 * hands it over with the interrupt off.
 */
#include "main.h"
#include "Soundengine/DSPAudio/random.h"
#include "uma_kits.h"
#include "Parameters.h"
#include "menu/menu.h"
#include "Soundengine/Sequencer/sequencer.h"
#include <string.h>
#include "uma_random.h"
#include "uma_mod.h"
#include "uma_voice.h"
#include "uma_quant.h"
#include "Soundengine/modSlots.h"
#include "Soundengine/DSPAudio/MdDrum.h"
#include "Soundengine/DSPAudio/PlaitsDrum.h"

volatile uint8_t umaKit_deferred;

static FIL umaKit_file;
static uint8_t umaKit_buf[UMA_KIT_BYTES] __attribute__((aligned(4)));		// main loop's card buffer
static uint8_t umaKit_next[UMA_KIT_BYTES] __attribute__((aligned(4)));		// the prefetched record
static volatile uint8_t umaKit_nextKit = UMA_KIT_NONE;	// whose record umaKit_next holds
static volatile uint8_t umaKit_pending = UMA_KIT_NONE;	// a kit loaded from the audio path without its record

static uint8_t* umaKit_active(void)
{
	return &activeParameters[PAR_UMA_BLOCK];
}

// the audio from the interrupt while the card is busy; returns whether it already was
static uint8_t umaKit_irqOn(void)
{
	const uint8_t was = (htim2.Instance->CR1 & TIM_CR1_CEN) != 0;
	if(!was)
		HAL_TIM_Base_Start_IT(&htim2);
	return was;
}
static void umaKit_irqOff(uint8_t was)
{
	if(!was)
		HAL_TIM_Base_Stop_IT(&htim2);
}

#define R(par) rec[(par) - PAR_UMA_BLOCK]
// Phase 2's settings (record format 3): mdk on its first kick (Drum 1-3) or the voice's own type (snare, cymbal,
// hi-hat), Plaits at its start, every voice at full velocity, the quantizer off (the Eurorack's defaults,
// presetManager.c umaFm_kitDefaults, formats 29-40)
static void umaKit_phase2Defaults(uint8_t* rec)
{
	for(uint8_t d = 0; d < 3; d++)
	{
		uint8_t* k = &R(UMA_MD_FIRST_PAR + d * UMA_MD_PARS);
		k[0] = MD_KICK;
		k[1] = 0;
		mdDrum_defaults(mdDrum_machineOf(MD_KICK, 0), &k[2]);
		R(UMA_MD_PAL_PAR + d) = MD_PAL_TYP;
	}
	for(uint8_t v = 3; v < 6; v++)	// the snare, cymbal and hi-hat on their stock sound (lxr)
	{
		uint8_t* x = &R(UMA_VX_PAR(v, 0));
		for(uint8_t k = 0; k < UMA_VX_PARS; k++) x[k] = 0;
		x[UMA_VX_PRK_P1] = 50;
		x[UMA_VX_PRK_P2] = 40;
		static const uint8_t ownType[3] = {MD_SNARE, MD_CYMBAL, MD_HAT};	// mdk starts on the voice's own type
		x[UMA_VX_MD_TYP] = ownType[v - 3];
		mdDrum_defaults(mdDrum_machineOf(ownType[v - 3], 0), &x[UMA_VX_MD_K1]);
		x[UMA_VX_MD_PAL] = MD_PAL_TYP;
		x[UMA_VX_FOLD_SYM] = 63;	// the folder off, centred
		x[UMA_VX_FOLD_LEV] = 100;
		x[UMA_VX_FENV] = 63;		// the filter envelope off
	}
	for(uint8_t v = 0; v < 6; v++)
	{
		uint8_t* x = &R(UMA_PLA_FIRST_PAR + v * UMA_PLA_PARS);	// Plaits: drums, bass drum, knobs at noon, out
		x[0] = PL_BANK_DRUMS; x[1] = 0; x[2] = 64; x[3] = 64; x[4] = 64; x[5] = PL_OUT_OUT;
		R(UMA_PLA_DXB_PAR + v) = 0;					// the 6-op on bank A
		R(UMA_PLA_KMD_PAR + v) = PL_DXK_PATCH;		// its own envelopes
		R(UMA_PLA_TWI_PAR + v) = UMA_PLA_TWI_NOON;	// TWIST at noon (Plaits as Mutable made it)
		R(UMA_PLA_WVF_PAR + v) = 0;					// built-in tables
		R(UMA_PLA_TRF_PAR + v) = 0;
		R(UMA_PLA_SUB_PAR + v) = R(UMA_PLA_SBS_PAR + v) = R(UMA_PLA_SBO_PAR + v) = R(UMA_PLA_SBF_PAR + v) = 0;
		R(UMA_VEL_PAR + v) = 127;					// full velocity (Phase 3)
		R(UMA_QUANT_FIRST_PAR + v * UMA_QUANT_PARS) = UMA_QUANT_OCT_ZERO;	// the quantizer off
		R(UMA_QUANT_FIRST_PAR + v * UMA_QUANT_PARS + 1) = 0;
		R(UMA_QUANT_FIRST_PAR + v * UMA_QUANT_PARS + 2) = 0;
	}
}

// Phase 3's settings (record format 4): every slot off (amount 0) on its voice's LFO A, unipolar off, v/o 0
#define UMA_SRC_LFOA_FIRST	5	// the Eurorack's source numbers: LFO A of Drum 1 (lf1)
static void umaKit_phase3Defaults(uint8_t* rec)
{
	for(uint8_t s = 0; s < NUM_MOD_SLOTS; s++)
	{
		R(modSlot_src(s)) = (uint8_t)(UMA_SRC_LFOA_FIRST + s / MOD_SLOTS_PER_VOICE);
		R(modSlot_amt(s)) = 0;
		R(modSlot_dest(s)) = 0;
		R(modSlot_pol(s)) = 0;
	}
	for(uint8_t v = 0; v < 6; v++)
	{
		R(UMA_DSK_VOCT_PAR + v) = 0;
		uint8_t* l = &R(UMA_LFOB_FIRST_PAR + v * UMA_LFOB_PARS);	// LFO B: a slow sine, free, no retrigger
		l[0] = 30; l[1] = 0; l[2] = 0; l[3] = 0;
		R(UMA_LFOB_SYNC_PAR + v) = 0;
		R(UMA_LFOB_FOLD_PAR + v) = 0;
	}
}

// the Turing Machines (format 5, 0.3c): the Eurorack's defaults (locked, 8 steps, own clock, every hit), random patterns
static void umaKit_turingDefaults(uint8_t* rec)
{
	for(uint8_t i = 0; i < UMA_TM_VOICES * UMA_TM_PER_VOICE; i++)
	{
		uint8_t* tm = &R(UMA_TM_FIRST_PAR + i * UMA_TM_PARS_PER_TM);
		tm[0] = 8;	// length
		tm[1] = 0;	// probability: locked
		tm[2] = 0;	// clock: own voice
		tm[3] = 1;	// every hit
		for(uint8_t b = 0; b < 2; b++)
			R(UMA_TM_REG_FIRST_PAR + i * 2 + b) = (uint8_t)GetRngValue();
		for(uint8_t b = 0; b < UMA_TM_REGX_PARS; b++)
			R(UMA_TM_REGX_FIRST_PAR + i * UMA_TM_REGX_PARS + b) = (uint8_t)GetRngValue();
	}
}

// a new record: every drum on lxr, so a kit sounds as on the stock firmware; the rest a usable start when a drum is
// set to an FM engine. The values are the Eurorack's (presetManager.c umaFm_kitDefaults, LX_UMA a179390); the other
// engines' and modulation's defaults come with Phases 2 and 3.
static void umaKit_makeDefaults(uint8_t* rec)
{
	memset(rec, 0, UMA_KIT_BYTES);
	rec[UMA_REC_FORMAT] = UMA_KIT_FORMAT;
	for(uint8_t d = 0; d < 3; d++)
	{
		uint8_t* p = &rec[PAR_UMA_ENG1 - PAR_UMA_BLOCK + d * UMA_FM_PARS_PER_DRUM];
		p[PAR_UMA_ENG1 - PAR_UMA_ENG1]		= 0;	// lxr
		p[PAR_UMA_ALGO1 - PAR_UMA_ENG1]		= 0;	// algorithm 1
		p[PAR_UMA_FDBK1 - PAR_UMA_ENG1]		= 0;
		p[PAR_UMA_RATIO_A1 - PAR_UMA_ENG1]	= 6;	// x1
		p[PAR_UMA_RATIO_B1 - PAR_UMA_ENG1]	= 18;	// x2
		p[PAR_UMA_MOD_A1 - PAR_UMA_ENG1]	= 60;
		p[PAR_UMA_ATK_A1 - PAR_UMA_ENG1]	= 0;
		p[PAR_UMA_DEC_A1 - PAR_UMA_ENG1]	= 40;
		p[PAR_UMA_END_A1 - PAR_UMA_ENG1]	= 0;
		p[PAR_UMA_MOD_B1 - PAR_UMA_ENG1]	= 0;
		p[PAR_UMA_ATK_B1 - PAR_UMA_ENG1]	= 0;
		p[PAR_UMA_DEC_B1 - PAR_UMA_ENG1]	= 30;
		p[PAR_UMA_END_B1 - PAR_UMA_ENG1]	= 0;

		uint8_t* t = &rec[UMA_TONE_FIRST_PAR - PAR_UMA_BLOCK + d * UMA_TONE_PARS];	// FM Tone
		t[0] = 6;	// ratio C x1
		t[1] = 6;	// ratio B2 x1
		uint8_t* b2 = &rec[UMA_B2ENV_FIRST_PAR - PAR_UMA_BLOCK + d * UMA_B2ENV_PARS];	// B2's envelope
		b2[1] = 30;

		uint8_t* e = &rec[UMA_ENV_FIRST_PAR - PAR_UMA_BLOCK + d * UMA_ENV_PARS];	// the body envelope (Phase 2)
		e[1] = 127;	// sustain (gate mode only)
		e[2] = 40;	// release (gate mode only)
		e[4] = 63;	// filter env off
		uint8_t* l = &rec[UMA_LPG_FIRST_PAR - PAR_UMA_BLOCK + d * UMA_LPG_PARS];	// the low-pass gate (Phase 2)
		l[0] = 50;
		l[1] = 30;
		l[2] = 127;
		uint8_t* f = &rec[UMA_FOLD_FIRST_PAR - PAR_UMA_BLOCK + d * UMA_FOLD_PARS];	// the wavefolder (Phase 2) and the body level
		f[2] = 63;	// symmetric
		f[3] = 100;	// body level 1
	}
	for(uint8_t i = 0; i < 6; i++)
		rec[PAR_UMA_FINE_A1 - PAR_UMA_BLOCK + i] = 63;	// fine ratios A and B exact
	umaKit_phase2Defaults(rec);
	umaKit_phase3Defaults(rec);
	umaKit_turingDefaults(rec);
}

// a record read from a file: n bytes valid; anything not understood becomes the defaults
static void umaKit_check(uint8_t* rec, UINT n)
{
	if(n == 0 || rec[UMA_REC_FORMAT] == 0 || rec[UMA_REC_FORMAT] > UMA_KIT_FORMAT)
	{
		umaKit_makeDefaults(rec);
		return;
	}
	if(n < UMA_KIT_BYTES)
		memset(rec + n, 0, UMA_KIT_BYTES - n);	// a shorter (older) record: the rest default
	if(rec[UMA_REC_FORMAT] < 2)	// 0.1b's records held only the test stamp: the settings' defaults
	{
		const uint8_t stamp = rec[UMA_REC_STAMP];
		umaKit_makeDefaults(rec);
		rec[UMA_REC_STAMP] = stamp;
	}
	if(rec[UMA_REC_FORMAT] < 3)	// records from 0.1-0.2b: Phase 2's settings at their defaults
		umaKit_phase2Defaults(rec);
	if(rec[UMA_REC_FORMAT] < 4)	// records from 0.1-0.2: the mod slots and v/o at their defaults
		umaKit_phase3Defaults(rec);
	if(rec[UMA_REC_FORMAT] < 5)	// records from 0.1-0.3b: the Turing Machines at their defaults
		umaKit_turingDefaults(rec);
	rec[UMA_REC_FORMAT] = UMA_KIT_FORMAT;
}

static FRESULT umaKit_storeRead(uint8_t kit, uint8_t* rec)
{
	UINT n = 0;
	FRESULT res = f_open(&umaKit_file, UMA_KIT_STORE, FA_OPEN_EXISTING | FA_READ);
	if(res == FR_OK)
	{
		res = f_lseek(&umaKit_file, (FSIZE_t)kit * UMA_KIT_BYTES);
		if(res == FR_OK)
			res = f_read(&umaKit_file, rec, UMA_KIT_BYTES, &n);
		f_close(&umaKit_file);
	}
	umaKit_check(rec, res == FR_OK ? n : 0);
	return res;
}

static FRESULT umaKit_storeWrite(uint8_t kit, const uint8_t* rec)
{
	UINT n = 0;
	FRESULT res = f_open(&umaKit_file, UMA_KIT_STORE, FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
	if(res == FR_OK)
	{
		res = f_lseek(&umaKit_file, (FSIZE_t)kit * UMA_KIT_BYTES);	// past the end: the file grows
		if(res == FR_OK)
			res = f_write(&umaKit_file, rec, UMA_KIT_BYTES, &n);
		f_close(&umaKit_file);
	}
	if(umaKit_nextKit == kit)
		umaKit_nextKit = UMA_KIT_NONE;	// the prefetched copy is old now
	return res;
}

void umaKit_onKitLoad(uint8_t kit)
{
	if(kit >= NUM_KIT_SLOTS)
		return;
	if(umaKit_deferred)
	{
		// from the audio path: no card access here
		if(umaKit_nextKit == kit)
		{
			memcpy(umaKit_active(), umaKit_next, UMA_KIT_BYTES);
			umaKit_pending = UMA_KIT_NONE;
			umaRnd_kitLoaded();	// loading never re-rolls: the loaded kit is the restore point
			umaMod_kitLoaded();	// its mod slots' destinations
		}
		else
			umaKit_pending = kit;	// umaKit_service reads it in a few ms
		return;
	}
	umaKit_pending = UMA_KIT_NONE;
	const uint8_t was = umaKit_irqOn();
	umaKit_storeRead(kit, umaKit_buf);
	__disable_irq();
	const uint8_t took = umaKit_pending == UMA_KIT_NONE;	// unless the sequencer switched kits meanwhile
	if(took)
		memcpy(umaKit_active(), umaKit_buf, UMA_KIT_BYTES);
	__enable_irq();
	umaKit_irqOff(was);
	if(took)
		{
			umaRnd_kitLoaded();	// loading never re-rolls: the loaded kit is the restore point
			umaMod_kitLoaded();
		}
}

void umaKit_store(uint8_t kit)
{
	if(kit >= NUM_KIT_SLOTS)
		return;
#if UMA_KIT_TEST
	umaKit_active()[UMA_REC_STAMP] = (uint8_t)(kit + 1);
#endif
	umaKit_active()[UMA_REC_FORMAT] = UMA_KIT_FORMAT;
	const uint8_t was = umaKit_irqOn();
	umaKit_storeWrite(kit, umaKit_active());
	umaKit_irqOff(was);
	umaRnd_kitSaved();	// saving keeps: randomise at 0 now restores the saved sound
}

void umaKit_fromFile(FIL* f, uint8_t kit)
{
	if(kit >= NUM_KIT_SLOTS)
		return;
	UINT n = 0;
	if(f_read(f, umaKit_buf, UMA_KIT_BYTES, &n) != FR_OK)
		n = 0;
	umaKit_check(umaKit_buf, n);
	umaKit_storeWrite(kit, umaKit_buf);
}

void umaKit_defaults(uint8_t kit)
{
	if(kit >= NUM_KIT_SLOTS)
		return;
	umaKit_makeDefaults(umaKit_buf);
	umaKit_storeWrite(kit, umaKit_buf);
}

void umaKit_toFile(FIL* f, uint8_t kit)
{
	UINT n;
	umaKit_storeRead(kit, umaKit_buf);
	f_write(f, umaKit_buf, UMA_KIT_BYTES, &n);
}

void umaKit_activeToFile(FIL* f)
{
	UINT n;
	f_write(f, umaKit_active(), UMA_KIT_BYTES, &n);
}

void umaKit_activeFromFile(FIL* f)
{
	UINT n = 0;
	if(f_read(f, umaKit_buf, UMA_KIT_BYTES, &n) != FR_OK || n == 0)
		return;	// an older tmp.snd: the kit's own record stays
	umaKit_check(umaKit_buf, n);
	memcpy(umaKit_active(), umaKit_buf, UMA_KIT_BYTES);
}

// the kit the sequencer will load next, or UMA_KIT_NONE
static uint8_t umaKit_upcoming(void)
{
	if(activeParameters[PAR_PATTERN_MODE] != 1)
		return UMA_KIT_NONE;	// kits don't follow patterns
	uint8_t pattern = seq_pendingPattern;
	if(pattern == seq_activePattern)
		pattern = seq_peekNextPattern();	// song mode: its next entry; else the same pattern again
	if(pattern == seq_activePattern || pattern >= NUM_PATTERN)
		return UMA_KIT_NONE;
	return seq_patternSet.kitNr[pattern];
}

void umaKit_service(void)
{
	// a kit the audio path loaded without its record: read it now
	const uint8_t late = umaKit_pending;
	if(late != UMA_KIT_NONE)
	{
		const uint8_t was = umaKit_irqOn();
		umaKit_storeRead(late, umaKit_buf);
		__disable_irq();
		const uint8_t took = umaKit_pending == late;
		if(took)
		{
			memcpy(umaKit_active(), umaKit_buf, UMA_KIT_BYTES);
			umaKit_pending = UMA_KIT_NONE;
		}
		__enable_irq();
		umaKit_irqOff(was);
		if(took)
			{
				umaRnd_kitLoaded();
				umaMod_kitLoaded();
			}
		return;
	}

	// the next pattern's kit, before the pattern starts
	const uint8_t want = umaKit_upcoming();
	if(want == UMA_KIT_NONE || want >= NUM_KIT_SLOTS || want == umaKit_nextKit)
		return;
	const uint8_t was = umaKit_irqOn();
	umaKit_storeRead(want, umaKit_buf);
	__disable_irq();
	memcpy(umaKit_next, umaKit_buf, UMA_KIT_BYTES);
	umaKit_nextKit = want;
	__enable_irq();
	umaKit_irqOff(was);
}

#if UMA_KIT_TEST
uint8_t umaKit_stamp(void)
{
	return umaKit_active()[UMA_REC_STAMP];
}
#endif
