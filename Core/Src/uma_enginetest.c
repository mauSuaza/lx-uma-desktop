/*
 * uma_enginetest.c - LX_UMA Phase 0 test 0.0c (uma_enginetest.h): three engines on Drum 1-3 with fixed settings,
 * to measure flash, RAM and CPU on the desktop before any menu work.
 */
#include "main.h"

#if UMA_ENGINE_TEST
#include "uma_enginetest.h"
#include "Soundengine/config.h"
#include "Soundengine/DSPAudio/FmDrum.h"
#include "Soundengine/DSPAudio/PlaitsDrum.h"
#include "Soundengine/DSPAudio/MdDrum.h"

#define UMA_TEST_FM		0	// Drum 1: FM Tone
#define UMA_TEST_PLA	1	// Drum 2: Plaits wavetable
#define UMA_TEST_MD		2	// Drum 3: mdk, the first kick machine

static FmDrum umaTest_fm;

__attribute__((noinline)) void umaTest_init(void)
{
	FmDrum* d = &umaTest_fm;
	fmDrum_init(d, REAL_FS);
	fmDrum_setAmpEnv(d, 0);						// the voice's amp EG shapes the volume
	fmDrum_setParam(d, FMD_SWEEP_AMT, 0);		// the voice's pitch EG does the sweep
	fmDrum_setParam(d, FMD_LEVEL, 127);
	fmDrum_setParam(d, FMD_TONE, 1);			// FM Tone
	fmDrum_setParam(d, FMD_ALGO, 0);
	fmDrum_setParam(d, FMD_MOD_A, 70);
	fmDrum_setParam(d, FMD_DEC_A, 70);
	fmDrum_setParam(d, FMD_MOD_B, 50);
	fmDrum_setParam(d, FMD_MOD_B2, 40);
	fmDrum_setParam(d, FMD_SUB_LEVEL, 60);		// the sub on, the heavier case

	plaitsDrum_init(UMA_TEST_PLA, REAL_FS);
	plaitsDrum_setModel(UMA_TEST_PLA, PL_MODEL_WAVETABLE);
	plaitsDrum_setKnobs(UMA_TEST_PLA, 0.5f, 0.5f, 0.5f);
	plaitsDrum_setOutput(UMA_TEST_PLA, PL_OUT_OUT);

	mdDrum_init(UMA_TEST_MD);
	const uint8_t kick = mdDrum_machineOf(0, 0);
	uint8_t controls[MD_CONTROLS];
	mdDrum_defaults(kick, controls);
	mdDrum_setMachine(UMA_TEST_MD, kick);
	for(uint8_t c = 0; c < MD_CONTROLS; c++)
		mdDrum_setControl(UMA_TEST_MD, c, controls[c]);
}

__attribute__((noinline)) void umaTest_trigger(uint8_t voiceNr)
{
	if(voiceNr == UMA_TEST_FM)
		fmDrum_trigger(&umaTest_fm, 1.f);	// velocity is applied by the voice
	else if(voiceNr == UMA_TEST_PLA)
		plaitsDrum_trigger(UMA_TEST_PLA, 0.8f);
	else if(voiceNr == UMA_TEST_MD)
		mdDrum_trigger(UMA_TEST_MD);
}

__attribute__((noinline)) void umaTest_block(uint8_t voiceNr, float freqHz)
{
	if(voiceNr == UMA_TEST_FM)
		fmDrum_setPitch(&umaTest_fm, freqHz);
	else if(voiceNr == UMA_TEST_PLA)
		plaitsDrum_setPitch(UMA_TEST_PLA, freqHz);
}

static void umaTest_toInt16(const float* in, int16_t* out, uint8_t size)
{
	for(uint8_t i = 0; i < size; i++)
	{
		float s = in[i] * 32767.f;
		if(s > 32767.f) s = 32767.f;
		if(s < -32767.f) s = -32767.f;
		out[i] = (int16_t)s;
	}
}

__attribute__((noinline)) uint8_t umaTest_render(uint8_t voiceNr, int16_t* buf, uint8_t size)
{
	if(voiceNr > UMA_TEST_MD || size != 64)
		return 0;
	float block[64];
	if(voiceNr == UMA_TEST_FM)
		fmDrum_render(&umaTest_fm, block, size);
	else if(voiceNr == UMA_TEST_PLA)
		plaitsDrum_render(UMA_TEST_PLA, block);
	else
		mdDrum_render(UMA_TEST_MD, block);
	umaTest_toInt16(block, buf, size);
	return 1;
}

__attribute__((noinline)) uint8_t umaTest_selfEnveloped(uint8_t voiceNr)
{
	return voiceNr == UMA_TEST_MD;
}
#endif
