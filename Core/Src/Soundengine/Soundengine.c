/*
 * Soundengine.c
 *
 *  Created on: Mar 24, 2020
 *      Author: julian
 *
 * Copyright (C) 2012-2026 Erica Synths
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * This file is part of the LXR firmware. It is licensed under the GNU General
 * Public License version 3, with the additional permission and terms in
 * NOTICE.md. See LICENSE and NOTICE.md in the root of the repository.
 */
#include "Soundengine/Soundengine.h"
#include "Soundengine/AudioCodecManager.h"
#include "Preset/PresetManager.h"
#include <string.h>
#include "Soundengine/USB/usb_manager.h"
#include "Soundengine/MIDI/Uart.h"

#if UMA_LOAD
// UMA: the audio interrupt is timed with the DWT cycle counter (enabled by DWT_Delay_Init in main.c)
#define UMA_LOAD_WINDOWS 8 // the peak covers the last 8 readouts: 2 s at the menu's 4 per second
static volatile uint32_t umaLoad_sum, umaLoad_max; // cycles since the last readout
static uint32_t umaLoad_peaks[UMA_LOAD_WINDOWS];
static uint8_t umaLoad_window;
static uint32_t umaLoad_lastTake, umaLoad_lastCycles;

void soundengine_addLoad(uint32_t cycles)
{
	umaLoad_sum += cycles;
	if(cycles > umaLoad_max)
		umaLoad_max = cycles;
}

static uint8_t umaLoad_percent(uint64_t cycles, uint64_t total)
{
	const uint64_t pct = total ? (cycles*100 + total/2)/total : 0;
	return pct > 255 ? 255 : (uint8_t)pct;
}

void soundengine_takeLoad(uint8_t* avg, uint8_t* peak)
{
	// one block is OUTPUT_DMA_SIZE stereo samples: about 313000 cycles at 216 MHz
	const uint32_t blockCycles = (uint32_t)((float)SystemCoreClock * OUTPUT_DMA_SIZE / REAL_FS);

	const uint32_t primask = __get_PRIMASK();
	__disable_irq();
	const uint32_t sum = umaLoad_sum, max = umaLoad_max, nowCycles = DWT->CYCCNT;
	umaLoad_sum = umaLoad_max = 0;
	__set_PRIMASK(primask);

	const uint32_t elapsed = nowCycles - umaLoad_lastCycles; // the counter wraps after 19.9 s
	umaLoad_lastCycles = nowCycles;
	const uint32_t now = HAL_GetTick();
	const uint8_t fresh = now - umaLoad_lastTake > 1000; // the readout was just opened: old values don't count
	umaLoad_lastTake = now;
	if(fresh)
		memset(umaLoad_peaks, 0, sizeof(umaLoad_peaks));
	umaLoad_peaks[umaLoad_window] = max;
	umaLoad_window = (umaLoad_window + 1) % UMA_LOAD_WINDOWS;

	uint32_t peakCycles = 0;
	for(uint8_t i=0; i<UMA_LOAD_WINDOWS; i++)
		if(umaLoad_peaks[i] > peakCycles)
			peakCycles = umaLoad_peaks[i];

	*avg = fresh ? 0 : umaLoad_percent(sum, elapsed);
	*peak = umaLoad_percent(peakCycles, blockCycles);
}
#endif

#if UMA_VOICE_LOAD
// LX_UMA test 0.2g: per voice (0-5), the highest block (both its calls added) since the last readout, and 8 readouts
static uint32_t umaVoiceLoad_block[6], umaVoiceLoad_max[6];
static uint32_t umaVoiceLoad_peaks[6][UMA_LOAD_WINDOWS];
static uint8_t umaVoiceLoad_window[6];
static uint8_t umaVoiceLoad_calls[6];
void soundengine_addVoiceLoad(uint8_t voice, uint32_t cycles)
{
	if(voice >= 6) return;
	umaVoiceLoad_block[voice] += cycles;
	if(++umaVoiceLoad_calls[voice] >= 2)	// its settings and its sound: one block done
	{
		if(umaVoiceLoad_block[voice] > umaVoiceLoad_max[voice])
			umaVoiceLoad_max[voice] = umaVoiceLoad_block[voice];
		umaVoiceLoad_block[voice] = 0;
		umaVoiceLoad_calls[voice] = 0;
	}
}
uint8_t soundengine_takeVoiceLoad(uint8_t voice)
{
	if(voice >= 6) return 0;
	const uint32_t blockCycles = (uint32_t)((float)SystemCoreClock * OUTPUT_DMA_SIZE / REAL_FS);
	const uint32_t primask = __get_PRIMASK();
	__disable_irq();
	const uint32_t m = umaVoiceLoad_max[voice];
	umaVoiceLoad_max[voice] = 0;
	__set_PRIMASK(primask);
	umaVoiceLoad_peaks[voice][umaVoiceLoad_window[voice]] = m;
	umaVoiceLoad_window[voice] = (uint8_t)((umaVoiceLoad_window[voice] + 1) % UMA_LOAD_WINDOWS);
	uint32_t peak = 0;
	for(uint8_t i = 0; i < UMA_LOAD_WINDOWS; i++)
		if(umaVoiceLoad_peaks[voice][i] > peak) peak = umaVoiceLoad_peaks[voice][i];
	return umaLoad_percent(peak, blockCycles);
}
#endif

uint8_t soundengine_calcNextSampleBlock()
{
	if(bCurrentSampleValid!= SAMPLE_VALID)
	{
	#if USE_DAC2
		mixer_calcNextSampleBlock((int16_t*)&dma_buffer[bCurrentSampleValid*(OUTPUT_DMA_SIZE*2)],(int16_t*)&dma_buffer2[bCurrentSampleValid*(OUTPUT_DMA_SIZE*2)]);
	#else
		mixer_calcNextSampleBlock(&dma_buffer[bCurrentSampleValid*(OUTPUT_DMA_SIZE*2)],&dma_buffer2[(1-bCurrentSampleValid)*(OUTPUT_DMA_SIZE*2)]);
	#endif
		bCurrentSampleValid = SAMPLE_VALID;

		return 1;
	}

	return 0;
}

void soundengine_init()
{
	mixer_init();



	// start the audio codec
//#ifndef DEBUG
	CodecInit(SYNTH_FS);
//#endif

	parameterArray_init();

	euklid_init();

	int i;
	for(i=0;i<6;i++)
	{
		modNode_init(&velocityModulators[i]);
	}

	//init the seq speed
	seq_init();
	seq_setBpm(120);

	memset(midi_MidiChannels,0,8);
//	memset(midi_NoteOverride,0,7);

	initDrumVoice();
	Snare_init();
	HiHat_init();
	Cymbal_init();

	//precalc the first dma buffer block
	soundengine_calcNextSampleBlock();

	FPU->FPDSCR |= FPU_FPDSCR_FZ_Msk ; //enable flush to zero
}

void soundengine_process()
{
#if CALC_SAMPLES_IN_INTERRUPT
	__HAL_TIM_DISABLE_IT(&htim2, TIM_IT_UPDATE);
	//HAL_TIM_Base_Stop_IT(&htim2);
#if UMA_LOAD
	// UMA: outside SD saves the sound is rendered here, from the main loop, not in the TIM2 interrupt
	const uint32_t umaLoad_start = DWT->CYCCNT;
#endif
	seq_tick();
	soundengine_calcNextSampleBlock();
#if UMA_LOAD
	soundengine_addLoad(DWT->CYCCNT - umaLoad_start);
#endif
	__HAL_TIM_SetCounter(&htim2, 1950);
	//HAL_TIM_Base_Start_IT(&htim2);
	__HAL_TIM_ENABLE_IT(&htim2, TIM_IT_UPDATE);
#endif
	//generate next sample if no valid sample is present
#if !CALC_SAMPLES_IN_INTERRUPT
#if 1//#ifndef DEBUG
		seq_tick();
		soundengine_calcNextSampleBlock();



#endif
#endif
		//process midi on midi port
		uart_processMidi();

		//process the sequencer
	//	seq_tick(); //moved to tim 14?
	usb_tick();
	//process incoming frontpanel data
		//	uart_processFront();






	preset_morph_tick();
	//preset_morph_tick();
	//preset_morph_tick();

}
