/*
 * uma_sdreadtest.c - LX_UMA test 0.1a (uma_sdreadtest.h). Does what option A's prefetch will do, twice a second:
 * find the kit file "NN-*.snd" in the project folder (as preset_makeFileNameLoad), open it, read 1,055 bytes (the
 * stock 255 + ~800 of UMA's; today's files hold only the first 255), close it. The audio runs from the TIM2
 * interrupt meanwhile, as during a save (USE_INTERRUPT_ONLY_DURING_SAVE_PROJ). Kits 00-63 in turn.
 */
#include "main.h"

#if UMA_SDREAD_TEST
#include "uma_sdreadtest.h"
#include "fatfs.h"
#include <stdio.h>
#include <string.h>

#define UMA_RD_WINDOWS 4	// the readout covers the last 4 reads: 2 s

static uint32_t umaRd_last;
static uint8_t umaRd_kit;
static uint32_t umaRd_times[UMA_RD_WINDOWS];	// cycles
static uint8_t umaRd_window;
static uint8_t umaRd_buf[1055];

void umaTest_sdReadTick(uint8_t shown)
{
	const uint32_t now = HAL_GetTick();
	if(!shown || now - umaRd_last < 500)
		return;
	umaRd_last = now;

	char pattern[13];
	sprintf(pattern, "%02u-*.snd", (unsigned)umaRd_kit);
	umaRd_kit = (uint8_t)((umaRd_kit + 1) % 64);

	HAL_TIM_Base_Start_IT(&htim2);	// audio from the interrupt while the card is busy
	const uint32_t start = DWT->CYCCNT;
	DIR dj;
	FILINFO fno;
	FIL file;
	UINT bytesRead = 0;
	FRESULT res = f_findfirst(&dj, &fno, "", pattern);
	if(res == FR_OK && fno.fname[0])
	{
		if(f_open(&file, fno.fname, FA_OPEN_EXISTING | FA_READ) == FR_OK)
		{
			f_read(&file, umaRd_buf, sizeof(umaRd_buf), &bytesRead);
			f_close(&file);
		}
	}
	f_closedir(&dj);
	const uint32_t cycles = DWT->CYCCNT - start;
	HAL_TIM_Base_Stop_IT(&htim2);

	umaRd_times[umaRd_window] = cycles;
	umaRd_window = (uint8_t)((umaRd_window + 1) % UMA_RD_WINDOWS);
}

uint8_t umaTest_sdReadTime(void)
{
	uint32_t worst = 0;
	for(uint8_t i = 0; i < UMA_RD_WINDOWS; i++)
		if(umaRd_times[i] > worst) worst = umaRd_times[i];
	const uint32_t tenthsMs = worst / (SystemCoreClock / 10000u);
	return tenthsMs > 255 ? 255 : (uint8_t)tenthsMs;
}
#endif
