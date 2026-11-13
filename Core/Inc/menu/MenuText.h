/*
 * MenuText.h
 *
 * Created: 04.11.2012 08:52:54
 *  Author: Julian
 *
 * Copyright (C) 2012-2026 Erica Synths
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * This file is part of the LXR firmware. It is licensed under the GNU General
 * Public License version 3, with the additional permission and terms in
 * NOTICE.md. See LICENSE and NOTICE.md in the root of the repository.
 */ 


#ifndef MENUTEXT_H_
#define MENUTEXT_H_

//-----------------------------------------------------------------
#define MENU_FILTER			1
#define MENU_WAVEFORM		2
#define MENU_AUDIO_OUT		3
#define MENU_ROLL_RATES		4
#define MENU_SYNC_RATES		5
#define MENU_LFO_WAVES		6
#define MENU_RETRIGGER		7
#define MENU_NEXT_PATTERN	8
#define MENU_SEQ_QUANT		9
#define MENU_TRANS			10
#define MENU_MIDI			11
#define MENU_MIDI_ROUTING	12
#define MENU_MIDI_FILTERING 13
#define MENU_PPQ_IN		 	14
#define MENU_MIX_FM		 	15
#define MENU_PPQ_OUT	 	16
#define MENU_FOLLOW		 	17
#define MENU_FX			 	18
#define MENU_DIST		 	19
#define MENU_RATIO		 	20
#define MENU_DELAY		 	21
#define MENU_REVERB		 	22
#define MENU_PAT_MODE	 	23
#define MENU_STEP_FLAM	 	24
#define MENU_STEP_SHIFT	 	25
#define MENU_AUDIO_OUT_FX	26
// LX_UMA: the Eurorack's UMA lists (its numbers - 2; only those with names below are drawn so far)
#define MENU_UMA_ENGINE	27	// UMA
#define MENU_UMA_ALGO	28	// UMA
#define MENU_UMA_RATIO	29	// UMA
#define MENU_UMA_TM_CLOCK	30	// UMA
#define MENU_UMA_GATE	31	// UMA: body envelope trg / gat
#define MENU_UMA_DRUM_FILTER	32	// UMA: Drum 1-3's filter types, with the low-pass gate
#define MENU_UMA_FOLD_TYPE	33	// UMA: wavefolder sin / tri
#define MENU_UMA_MORPH_LISTS	34	// UMA: how lists morph: step / switch
#define MENU_UMA_CLOCK	35	// UMA: clock source off / jck
#define MENU_UMA_CLOCK_DIV	36	// UMA: what a clock pulse is worth
#define MENU_UMA_SUB_SHAPE	37	// UMA: FM Tone's sub shape
#define MENU_UMA_SUB_OCT	38	// UMA: FM Tone's sub octave
#define MENU_UMA_PALETTE	39	// UMA: randomise's ratio palette
#define MENU_UMA_PRK_ALG	40	// UMA: the prk engine's algorithms
#define MENU_UMA_PRK_VAR	41	// UMA: its variations (named per algorithm on the edit screen)
#define MENU_UMA_PRK_PAL	42	// UMA: its randomise palette
#define MENU_UMA_POL	43	// UMA: a mod slot's polarity
#define MENU_UMA_MD_TYP	44	// UMA: the drum-machine engine's sound types
#define MENU_UMA_MD_MCH	45	// UMA: a type's machines (shown by name, menu_umaShortValue)
#define MENU_UMA_MD_PAL	46	// UMA: the drum-machine randomise's palette
#define MENU_UMA_VENGINE	47	// UMA: the snare's, cymbal's and hi-hat's engine: lxr prk mdk
#define MENU_UMA_MC	48	// UMA: the CC a MIDI CC source follows: off, 1-127
#define MENU_UMA_MC_CHAN	49	// UMA: the MIDI CC sources' channel: glb, 1-16, off
#define MENU_UMA_OCT	50	// UMA: a voice's octave, -4..+4 (uma_quant.h)
#define MENU_UMA_KEY	51	// UMA: a voice's key, C..B (names from uma_quant.c)
#define MENU_UMA_SCALE	52	// UMA: a voice's scale or chord (names from uma_quant.c)
#define MENU_UMA_PLA_BNK	53	// UMA: the Plaits engine's bank: drm ton (names from PlaitsDrum.cpp)
#define MENU_UMA_PLA_MDL	54	// UMA: a bank's models (shown by name, menu_umaShortValue)
#define MENU_UMA_PLA_OUT	55	// UMA: Plaits' outputs: out aux mix
#define MENU_UMA_PLA_DXB	56	// UMA: the 6-op FM's DX7 bank (`fil`): A B C, then the SD card's (PlaitsDrum.cpp)
#define MENU_UMA_PLA_KMD	57	// UMA: the 6-op FM's key mode: ptc frz
#define MENU_UMA_CVJ	58	// UMA: the CV calibration's jack cv1-cv5
#define MENU_UMA_CVP	59	// UMA: its point 0V-5V
#define MENU_UMA_CVS	60	// UMA: --- / set (store)
#define MENU_UMA_CVR	61	// UMA: --- / rst (reset)
#define MENU_UMA_PLA_WVF	62	// UMA: the wavetable's file: bit (built in), then the SD card's WAVES (PlaitsDrum.cpp)
#define MENU_UMA_PLA_TRF	63	// UMA: the wave terrain's file: bit, then the card's TERRAIN
#define MENU_UMA_SRC	64	// LX_UMA desktop: a mod slot's source (the Eurorack's numbers; the ones without a desktop source skipped)
#define MENU_UMA_LFOB_RTG	65	// LX_UMA desktop: LFO B's retrigger (umaLfoBRtgNames)
#define MENU_UMA_LFOB_SYNC	66	// LX_UMA desktop: LFO B's sync (umaLfoBSyncNames)

//-----------------------------------------------------------------
// Shared texts. Reduce mem usage by pooling common text
// all of these need to be 3 chars
const char menuText_ok[] 	= 	"ok ";
const char menuText_off[] 	= "off";
const char menuText_on[] 	= 	"on ";
const char menuText_mix[] 	= "mix";
const char menuText_fm[] 	= 	"fm ";
const char menuText_dash[] 	="---";
const char menuText_blank[] ="   ";
const char menuText_any[]  	= "any";
//-----------------------------------------------------------------
// LX_UMA: a drum's engine, the Eurorack's numbers: 3 (prk) isn't on the desktop and is skipped when editing
// (menu_umaAfterEdit); pla (5) comes with 0.2e
const char umaEngineNames[][4] =
{
	{6},		//number of entries
	{"lxr"},
	{"fmd"},	// FM Drum
	{"fmt"},	// FM Tone
	{"---"},	// (prk, not on the desktop)
	{"mdk"},	// drum machines (MdDrum.cpp)
	{"pla"},	// Plaits (PlaitsDrum.cpp)
};

const char umaAlgoNames[][4] =
{
	{8},		//number of entries (FM Drum uses the first 7, menu_umaListEntries)
	{"1  "},
	{"2  "},
	{"3  "},
	{"4  "},
	{"5  "},
	{"6  "},
	{"7  "},
	{"8  "},
};

const char umaRatioNames[][4] =
{
	{51},		//number of entries: FmDrum's ratio table (fmd_ratios)
	{"1/4"},
	{"1/3"},
	{"1/2"},
	{"2/3"},	// fifth below
	{".71"},	// 1/sqrt 2
	{"3/4"},	// fourth below
	{"1  "},
	{"6/5"},	// minor third
	{"5/4"},	// major third
	{"4/3"},	// fourth
	{"1.4"},	// sqrt 2
	{"3/2"},	// fifth
	{"8/5"},	// minor sixth
	{"5/3"},	// major sixth
	{"1.7"},	// sqrt 3
	{"7/4"},	// harmonic seventh
	{"9/5"},	// minor seventh
	{"1.9"},	// major seventh 15/8
	{"2  "},
	{"9/4"},	// ninth
	{"2.4"},	// 1 + sqrt 2
	{"5/2"},
	{"8/3"},	// eleventh
	{"2.8"},	// 2 sqrt 2
	{"3  "},
	{"3.1"},	// pi
	{"3.3"},	// thirteenth 10/3
	{"3.4"},	// 2 sqrt 3
	{"7/2"},
	{"4  "},
	{"4.2"},	// 3 sqrt 2
	{"9/2"},
	{"4.7"},	// 3 pi / 2
	{"5  "},
	{"5.2"},	// 3 sqrt 3
	{"5.7"},	// 4 sqrt 2
	{"6  "},
	{"6.3"},	// 2 pi
	{"6.9"},	// 4 sqrt 3
	{"7  "},
	{"7.9"},	// 5 pi / 2
	{"8  "},
	{"8.5"},	// 6 sqrt 2
	{"9  "},
	{"9.4"},	// 3 pi
	{"10 "},
	{"11 "},
	{"12 "},
	{"13 "},
	{"14 "},
	{"16 "},
};

const char umaSubShapeNames[][4] =
{
	{4},		//number of entries
	{"sin"},
	{"tri"},
	{"saw"},
	{"sqr"},
};

const char umaSubOctNames[][4] =
{
	{4},		//number of entries
	{"-1 "},
	{"-2 "},
	{"-3 "},
	{"-4 "},
};
// LX_UMA (0.2a): the body envelope's mode, the drums' filter types with the low-pass gate, the fold type
const char umaGateNames[][4] =
{
	{2},		//number of entries
	{"trg"},	// attack - hold - decay
	{"gat"},	// attack - hold - decay - sustain - release while a gate is held
};
const char umaDrumFilterTypes[][8]  =
{
	{9},		//number of entries
	{"LP"},
	{"HP"},
	{"BP"},
	{"UBP"},
	{"Nch"},
	{"Pek"},
	{"LP2"},
	{"off"},
	{"lpg"},
};
const char umaFoldTypeNames[][4] =
{
	{2},		//number of entries
	{"sin"},	// smooth, round (Buchla-style)
	{"tri"},	// sharper, edgier (Serge-style)
};
// LX_UMA (0.2b): randomise's ratio palette
const char umaPaletteNames[][4] =
{
	{4},		//number of entries
	{"all"},
	{"bas"},
	{"hrm"},
	{"met"},
};
// LX_UMA (0.2c): the drum-machine engine
const char umaMdTypNames[][4] =
{
	{6},		//number of entries
	{"kik"},
	{"snr"},
	{"clp"},
	{"hat"},
	{"cym"},
	{"prc"},
};
const char umaMdPalNames[][4] =
{
	{3},		//number of entries
	{"typ"},	// the home machine's type
	{"fam"},	// its type and family
	{"all"},	// any machine
};
const char umaMdMchNames[][4] =
{
	{13},		//number of entries: the most a type has
	{"1  "}, {"2  "}, {"3  "}, {"4  "}, {"5  "}, {"6  "}, {"7  "}, {"8  "}, {"9  "}, {"10 "}, {"11 "}, {"12 "}, {"13 "},
};
// LX_UMA (0.2d): the snare's, cymbal's and hi-hat's engine, stored 0-3 as on the Eurorack: 1 (prk) isn't on the
// desktop and is skipped when editing; pla (3) comes with 0.2e
const char umaVEngineNames[][4] =
{
	{4},		//number of entries
	{"lxr"},	// the voice's own sound
	{"---"},	// (prk, not on the desktop)
	{"mdk"},
	{"pla"},
};
// LX_UMA (0.2f): the pitch screen's octave (key and scale names: uma_quant.c)
const char umaOctNames[][4] = {{"-4"}, {"-3"}, {"-2"}, {"-1"}, {"0"}, {"+1"}, {"+2"}, {"+3"}, {"+4"}};
// LX_UMA (0.3a): a mod slot's source, the Eurorack's list (its numbers: a slot's source means the same on both); the
// ones without a desktop source (cv, accent jacks, MIDI CC) are skipped when editing (uma_modmatrix.c)
const char umaSrcNames[][4] =
{
	{73},		//number of entries
	{"cv1"},
	{"cv2"},
	{"cv3"},
	{"cv4"},
	{"cv5"},
	{"LA1"},
	{"LA2"},
	{"LA3"},
	{"LA4"},
	{"LA5"},
	{"LA6"},
	{"ac1"},
	{"ac2"},
	{"ac3"},
	{"ac4"},
	{"ac5"},
	{"ac6"},
	{"T1A"},
	{"T1B"},
	{"T1C"},
	{"T1D"},
	{"T2A"},
	{"T2B"},
	{"T2C"},
	{"T2D"},
	{"T3A"},
	{"T3B"},
	{"T3C"},
	{"T3D"},
	{"T4A"},
	{"T4B"},
	{"T4C"},
	{"T4D"},
	{"T5A"},
	{"T5B"},
	{"T5C"},
	{"T5D"},
	{"T6A"},
	{"T6B"},
	{"T6C"},
	{"T6D"},
	{"LB1"},
	{"LB2"},
	{"LB3"},
	{"LB4"},
	{"LB5"},
	{"LB6"},
	{"EN1"},
	{"EN2"},
	{"EN3"},
	{"EN4"},
	{"EN5"},
	{"EN6"},
	{"EI1"},
	{"EI2"},
	{"EI3"},
	{"EI4"},
	{"EI5"},
	{"EI6"},
	{"VL1"},
	{"VL2"},
	{"VL3"},
	{"VL4"},
	{"VL5"},
	{"VL6"},
	{"MC1"},
	{"MC2"},
	{"MC3"},
	{"MC4"},
	{"MC5"},
	{"MC6"},
	{"MC7"},
	{"MC8"},
};
const char umaPolNames[][4] =
{
	{2},		//number of entries
	{"bi "},	// around the knob
	{"uni"},	// from the knob up
};
// LX_UMA (0.3b): LFO B's retrigger and sync (the Eurorack's lists; LFO A keeps the stock ones)
const char umaLfoBRtgNames[][4] =
{
	{11},		//number of entries: off, v1-v6, every 1 2 4 8 bars
	{"off"},
	{"v1"},
	{"v2"},
	{"v3"},
	{"v4"},
	{"v5"},
	{"v6"},
	{"1b "},
	{"2b "},
	{"4b "},
	{"8b "},
};
// LX_UMA (0.3c): a Turing Machine's clock (the Eurorack's list, its byte values): its own voice's hits, another
// voice's, or the sequencer's sixteenths (the Eurorack's `clk`, UMA_TM_CLOCK_TICK)
// LX_UMA (0.3e): how list settings morph (the Eurorack's)
const char umaMorphListNames[][4] =
{
	{2},		//number of entries
	{"stp"},	// step through the values in between
	{"swt"},	// switch at 50 %
};
const char umaTmClockNames[][4] =
{
	{8},		//number of entries
	{"own"},
	{"dr1"},
	{"dr2"},
	{"dr3"},
	{"snr"},
	{"cym"},
	{"hat"},
	{"seq"},
};
const char umaLfoBSyncNames[][4] =
{
	{16},		//number of entries: off and 15 divisions of the tempo
	{"off"},
	{"4/1"},
	{"2/1"},
	{"1/1"},
	{"1/2"},
	{"4. "},
	{"1/3"},
	{"1/4"},
	{"8. "},
	{"1/6"},
	{"1/8"},
	{"16."},
	{"12 "},
	{"16 "},
	{"24 "},
	{"32 "},
};

const char umaMcNames[][4] =
{
	{128},		//number of entries: off, then CC 1-127
	{"off"},
	{"1"},
	{"2"},
	{"3"},
	{"4"},
	{"5"},
	{"6"},
	{"7"},
	{"8"},
	{"9"},
	{"10"},
	{"11"},
	{"12"},
	{"13"},
	{"14"},
	{"15"},
	{"16"},
	{"17"},
	{"18"},
	{"19"},
	{"20"},
	{"21"},
	{"22"},
	{"23"},
	{"24"},
	{"25"},
	{"26"},
	{"27"},
	{"28"},
	{"29"},
	{"30"},
	{"31"},
	{"32"},
	{"33"},
	{"34"},
	{"35"},
	{"36"},
	{"37"},
	{"38"},
	{"39"},
	{"40"},
	{"41"},
	{"42"},
	{"43"},
	{"44"},
	{"45"},
	{"46"},
	{"47"},
	{"48"},
	{"49"},
	{"50"},
	{"51"},
	{"52"},
	{"53"},
	{"54"},
	{"55"},
	{"56"},
	{"57"},
	{"58"},
	{"59"},
	{"60"},
	{"61"},
	{"62"},
	{"63"},
	{"64"},
	{"65"},
	{"66"},
	{"67"},
	{"68"},
	{"69"},
	{"70"},
	{"71"},
	{"72"},
	{"73"},
	{"74"},
	{"75"},
	{"76"},
	{"77"},
	{"78"},
	{"79"},
	{"80"},
	{"81"},
	{"82"},
	{"83"},
	{"84"},
	{"85"},
	{"86"},
	{"87"},
	{"88"},
	{"89"},
	{"90"},
	{"91"},
	{"92"},
	{"93"},
	{"94"},
	{"95"},
	{"96"},
	{"97"},
	{"98"},
	{"99"},
	{"100"},
	{"101"},
	{"102"},
	{"103"},
	{"104"},
	{"105"},
	{"106"},
	{"107"},
	{"108"},
	{"109"},
	{"110"},
	{"111"},
	{"112"},
	{"113"},
	{"114"},
	{"115"},
	{"116"},
	{"117"},
	{"118"},
	{"119"},
	{"120"},
	{"121"},
	{"122"},
	{"123"},
	{"124"},
	{"125"},
	{"126"},
	{"127"},
};
//-----------------------------------------------------------------
const char stepFlamNames[][4] =
{
	{6},		//number of entries
	{"off"},
	{"x2"},
	{"x3"},
	{"x4"},
	{"x6"},
	{"x8"}

	//{"+8"},

};
//-----------------------------------------------------------------
const char stepShiftNames[][4] =
{
	{15},		//number of entries
	{"-7"},
	{"-6"},
	{"-5"},
	{"-4"},
	{"-3"},
	{"-2"},
	{"-1"},
	{"0"},
	{"+1"},
	{"+2"},
	{"+3"},
	{"+4"},
	{"+5"},
	{"+6"},
	{"+7"},
	//{"+8"},

};
//-----------------------------------------------------------------
const char patModeNames[][4] =
{
	{2},		//number of entries
	{"off"},
	{"Aut"},

};
//-----------------------------------------------------------------
const char reverbNames[][4] =
{
	{1},		//number of entries
	{"Plt"},

};
//-----------------------------------------------------------------
const char delayNames[][4] =
{
	{2},		//number of entries
	{"Del"},
	{"Pp"},

};
//-----------------------------------------------------------------
const char ratioNames[][4] =
{
	{6},		//number of entries
	{"1:1"},
	{"2:1"},
	{"3:1"},
	{"4:1"},
	{"6:1"},
	{"8:1"}
};
//-----------------------------------------------------------------
const char fxNames[][4] =
{
	{5},		//number of entries
	{"off"},
	{"drv"},
	{"rng"},
	{"cmp"},
	{"del"},
	//{"rev"}
};
//-----------------------------------------------------------------
const char distNames[][4] =
{
	{3},		//number of entries
	{"tub"},
	{"fld"},
	{"clp"}
};
//-----------------------------------------------------------------
const char ppqInNames[][4] =
{
	{2},		//number of entries
	{"4"},
	//{"8"},
	//{"16"},
	{"24"}

};
//-----------------------------------------------------------------
const char followNames[][4] =
{
	{4},		//number of entries
	{"off"},
	{"bar"},
	{"pat"},
	{"p+b"}
};
//-----------------------------------------------------------------
const char ppqOutNames[][4] =
{
	{6},		//number of entries
	{"2"},
	{"4"},
	{"8"},
	{"16"},
	{"24"},
	{"48"},
};
//-----------------------------------------------------------------
const char midiModes[][4]  =
{
	{2},		//number of entries
	{"trg"},
	{"nte"},
};
//-----------------------------------------------------------------
const char quantisationNames[][4]  =
{
	{5},		//number of entries
	{"off"},
	{"8"},
	{"16"},
	{"32"},
	{"64"},
};
//-----------------------------------------------------------------
const char transientNames[][4]  =
{
	{14},		//number of entries
	{"Snp"},
	{"Ofs"},
	{"Clk"},
	{"Ck2"},
	{"Tik"},
	{"Kik"},
	{"Rim"},
	{"Drp"},
	{"Hat"},
	{"Clp"},
	{"Kk2"},
	{"Snr"},
	{"Tom"},
	{"Sp2"},
	
};


//-----------------------------------------------------------------
const char nextPatternNames[][4]  =
{
	{15},		//number of entries
	{"p1"},
	{"p2"},
	{"p3"},
	{"p4"},
	{"p5"},
	{"p6"},
	{"p7"},
	{"p8"},
	{"r2"},
	{"r3"},
	{"r4"},
	{"r5"},
	{"r6"},
	{"r7"},
	{"r8"},
};
//-----------------------------------------------------------------
const char retriggerNames[][4]  =
{
	{7},		//number of entries
	{"off"},
	{"v1"},
	{"v2"},
	{"v3"},
	{"v4"},
	{"v5"},
	{"v6"},
};
//-----------------------------------------------------------------
const char lfoWaveNames[][4]  =
{
	{8},		//number of entries
	{"sin"},
	{"tri"},
	{"sup"},
	{"sdn"},
	{"sqr"},
	{"rnd"},
	{"xup"},
	{"xdn"},
};
//-----------------------------------------------------------------
const char rollRateNames[][4]  =
{
	{6},		//number of entries
	{"One"},		
	{"1/1"},		
	{"1/2"},
	{"1/4"},		
	{"1/8"},		
	{"16"},		
	{"24"},		
	{"32"},		
	{"48"},		
	{"64"},		
	{"128"},
	{"1/3"},
	{"1/6"},
	{"12"},
};
//-----------------------------------------------------------------
const char syncRateNames[][4]  =
{
	{12},		//number of entries
	{"off"},
	{"4/1"},
	{"2/1"},
	{"1/1"},
	{"1/2"},
	{"1/3"},
	{"1/4"},
	{"1/6"},
	{"1/8"},
	{"12"},
	{"16"},
	{"32"},
};
//-----------------------------------------------------------------
const char waveformNames[][4]  =
{
	{6},		//number of entries
	{"Sin"}	,
	{"Tri"}	,
	{"Saw"}	,
	{"Rec"}	,
	{"Noi"}	,
	{"PWM"}	,

};
//-----------------------------------------------------------------
const char outputNames[][7]  =
{
	{7},		//number of entries
	{"St1"}	,	//Stereo Out DAC 1
	{"St2"}	,	//Stereo Out DAC 2
	{"L1"}	,	//Mono Out Left DAC 1
	{"R1"}	,	//Mono Out Right DAC 1
	{"L2"}	,	//Mono Out Left DAC 2
	{"R2"}	,	//Mono Out Right DAC 2
	{"FX"}	,	//to FX Bus
};
//-----------------------------------------------------------------
//output routing for FX processor, without FX target
const char outputNamesFx[][7]  =
{
	{6},		//number of entries
	{"St1"}	,	//Stereo Out DAC 1
	{"St2"}	,	//Stereo Out DAC 2
	{"L1"}	,	//Mono Out Left DAC 1
	{"R1"}	,	//Mono Out Right DAC 1
	{"L2"}	,	//Mono Out Left DAC 2
	{"R2"}	,	//Mono Out Right DAC 2

};
//-----------------------------------------------------------------
const char filterTypes[][8]  =
{
	{8},		//number of entries
	{"LP"}	,	//1 - 001
	{"HP"}	,	//2 - 010
	{"BP"}	,	//3 - 011
	{"UBP"}	,	//4 - 100
	{"Nch"}	,	//5 - 101
	{"Pek"}	,	//6 - 110
	{"LP2"}	,	//1 - 001
	{"off"}	,	//7 - 111
	//{"All"}	,	//7 - 111
};

//-----------------------------------------------------------------
const char midiRoutingNames[][6] =
{
		{6},	// number of entries
		{"off"},
		{"U2M"},
		{"M2M"},
		{"A2M"},
		{"M2U"},
		{"M2A"},
};
//-----------------------------------------------------------------
const char mixFmNames[][6] =
{
		{2},	// number of entries
		{"FM"},
		{"Mix"},

};
//-----------------------------------------------------------------
const char midiFilterNames[][16] =
{
	{16},	// number of entries
	{"off"},
	{"N"},
	{"R"},
	{"RN"},
	{"C"},
	{"CN"},
	{"CR"},
	{"CRN"},
	{"P"},
	{"PN"},
	{"PR"},
	{"PRN"},
	{"PC"},
	{"PCN"},
	{"PCR"},
	{"all"},
};
//-----------------------------------------------------------------
// these must correspond with shortNamesEnum
const char shortNames[][4]  =
{
	{""},
	{"coa"},
	{"fin"},
	{"atk"},
	{"dec"},
	{"eg2"},
	{"mod"},
	{"amt"},
	{"frq"},
	{"drv"},
	{"vol"},
	{"pan"},
	{"noi"},
	{"rpt"},
	{"mix"},	
	{"res"},
	{"typ"},
	{"f1"},
	{"f2"},
	{"g1"},
	{"g2"},
	{"wav"},
	{"dst"},
	{"snc"},
	{"rtg"},
	{"ofs"},
	{"voi"},
	{"slp"},
	{"d1"},		
	{"d2"},
	{"eqg"},
	{"eqf"},
#if USE_CODEC_EQ
	{"eq "},
	{"lim"},
	{"rel"},
	{"max"},
	{"min"},
#endif
	{"rol"},
	{"mrp"},
	{"nte"},
	{"prb"},
	{"stp"},
	{"len"},
	{"rot"},
	{"bpm"},
	{"ch"},
	{"out"},
	{"srt"},
	{"nxt"},
	{"mod"},
	{"vel"},
	{"fch"},
	{"flw"},
	{"qnt"},
	{"trk"},
	{"val"},
	{"shu"},
	{"ssv"},
	{"x"},
	{"y"},
	{"flx"},
	{"mid"},
	{"mrt"}, // midi routing
	{"mtx"}, // midi tx
	{"mrx"},  // midi rx
	{"cki"},  // trigger clock in ppq
	{"cko"},  // trigger clock out1 ppq
	{"co2"},  // trigger clock out2 ppq
	{"pcr"}, // pattern change resets bar counter
	{"pwm"},
	{"col"},
	{"d/w"},
	{"rat"},
	{"tre"},
	{"ton"},
	{"gai"},
	{"fbk"},
	{"rng"},
	{"tim"},
	{"pre"},
	{"ref"},
	{"p1"},
	{"p2"},
	{"p3"},
	{"p4"},
	{"kcm"},
	{"flm"},
	{"shf"},

	// LX_UMA: FM Drum and FM Tone
	{"eng"},	// LX_UMA SHORT_UMA_ENGINE
	{"alg"},	// LX_UMA SHORT_UMA_ALGO
	{"fdb"},	// LX_UMA SHORT_UMA_FEEDBACK
	{"rta"},	// LX_UMA SHORT_UMA_RATIO_A
	{"rtb"},	// LX_UMA SHORT_UMA_RATIO_B
	{"end"},	// LX_UMA SHORT_UMA_END
	{"fna"},	// LX_UMA SHORT_UMA_FINE_A
	{"fnb"},	// LX_UMA SHORT_UMA_FINE_B
	{"wvc"},	// LX_UMA SHORT_UMA_WAVE_C
	{"wvm"},	// LX_UMA SHORT_UMA_WAVE_AB
	{"rtc"},	// LX_UMA SHORT_UMA_RATIO_C
	{"rb2"},	// LX_UMA SHORT_UMA_RATIO_B2
	{"dtn"},	// LX_UMA SHORT_UMA_DETUNE
	{"mix"},	// LX_UMA SHORT_UMA_MIX_XY
	{"sub"},	// LX_UMA SHORT_UMA_SUB
	{"sbs"},	// LX_UMA SHORT_UMA_SUB_SHAPE
	{"sbo"},	// LX_UMA SHORT_UMA_SUB_OCT
	{"sbf"},	// LX_UMA SHORT_UMA_SUB_FOLD
	{"hld"},	// LX_UMA SHORT_UMA_HOLD
	{"sus"},	// LX_UMA SHORT_UMA_SUSTAIN
	{"rel"},	// LX_UMA SHORT_UMA_RELEASE
	{"gat"},	// LX_UMA SHORT_UMA_GATE
	{"env"},	// LX_UMA SHORT_UMA_FILTER_ENV
	{"dcy"},	// LX_UMA SHORT_UMA_LPG_DCY
	{"col"},	// LX_UMA SHORT_UMA_LPG_COL
	{"exc"},	// LX_UMA SHORT_UMA_LPG_EXC
	{"fld"},	// LX_UMA SHORT_UMA_FOLD
	{"fty"},	// LX_UMA SHORT_UMA_FOLD_TYPE
	{"sym"},	// LX_UMA SHORT_UMA_FOLD_SYM
	{"lev"},	// LX_UMA SHORT_UMA_BODY_LEV
	{"rpl"},	// LX_UMA SHORT_UMA_RND_PALETTE
	{"rto"},	// LX_UMA SHORT_UMA_RND_TONE
	{"rev"},	// LX_UMA SHORT_UMA_RND_ENV
	{"rsw"},	// LX_UMA SHORT_UMA_RND_SWEEP
	{"rsd"},	// LX_UMA SHORT_UMA_RND_SEED
	{"rfn"},	// LX_UMA SHORT_UMA_RND_FINE
	{"typ"},	// LX_UMA SHORT_UMA_MD_TYP
	{"mch"},	// LX_UMA SHORT_UMA_MD_MCH
	{"k1 "},	// LX_UMA SHORT_UMA_MD_K1
	{"k2 "},	// LX_UMA SHORT_UMA_MD_K2
	{"k3 "},	// LX_UMA SHORT_UMA_MD_K3
	{"k4 "},	// LX_UMA SHORT_UMA_MD_K4
	{"k5 "},	// LX_UMA SHORT_UMA_MD_K5
	{"k6 "},	// LX_UMA SHORT_UMA_MD_K6
	{"k7 "},	// LX_UMA SHORT_UMA_MD_K7
	{"k8 "},	// LX_UMA SHORT_UMA_MD_K8
	{"rmc"},	// LX_UMA SHORT_UMA_MD_RMC
	{"rkn"},	// LX_UMA SHORT_UMA_MD_RKN
	{"bnk"},	// LX_UMA SHORT_UMA_PLA_BNK
	{"mdl"},	// LX_UMA SHORT_UMA_PLA_MDL
	{"out"},	// LX_UMA SHORT_UMA_PLA_OUT
	{"hrm"},	// LX_UMA SHORT_UMA_PLA_HRM
	{"tmb"},	// LX_UMA SHORT_UMA_PLA_TMB
	{"mor"},	// LX_UMA SHORT_UMA_PLA_MOR
	{"twi"},	// LX_UMA SHORT_UMA_PLA_TWI
	{"fil"},	// LX_UMA SHORT_UMA_PLA_DXB
	{"kmd"},	// LX_UMA SHORT_UMA_PLA_KMD
	{"oct"},	// LX_UMA SHORT_UMA_OCT
	{"key"},	// LX_UMA SHORT_UMA_KEY
	{"scl"},	// LX_UMA SHORT_UMA_SCALE
	{"v/o"},	// LX_UMA SHORT_1VOCT
	{"fdA"},	// LX_UMA SHORT_UMA_LFO_FD_A
	{"fdB"},	// LX_UMA SHORT_UMA_LFO_FD_B
	{"frB"},	// LX_UMA SHORT_UMA_LFO_FR_B
	{"ofB"},	// LX_UMA SHORT_UMA_LFO_OF_B
	{"rtB"},	// LX_UMA SHORT_UMA_LFO_RT_B
	{"snB"},	// LX_UMA SHORT_UMA_LFO_SN_B
	{"wvB"},	// LX_UMA SHORT_UMA_LFO_WV_B
	{"al "},	// LX_UMA SHORT_UMA_T_ALGO
	{"aA "},	// LX_UMA SHORT_UMA_T_ATK_A
	{"aB "},	// LX_UMA SHORT_UMA_T_ATK_B
	{"a2 "},	// LX_UMA SHORT_UMA_T_ATK_B2
	{"dA "},	// LX_UMA SHORT_UMA_T_DEC_A
	{"dB "},	// LX_UMA SHORT_UMA_T_DEC_B
	{"d2 "},	// LX_UMA SHORT_UMA_T_DEC_B2
	{"eA "},	// LX_UMA SHORT_UMA_T_END_A
	{"eB "},	// LX_UMA SHORT_UMA_T_END_B
	{"e2 "},	// LX_UMA SHORT_UMA_T_END_B2
	{"fb "},	// LX_UMA SHORT_UMA_T_FEEDBACK
	{"rpm"},	// LX_UMA SHORT_UMA_T_MD_PAL
	{"mA "},	// LX_UMA SHORT_UMA_T_MOD_A
	{"mB "},	// LX_UMA SHORT_UMA_T_MOD_B
	{"m2 "},	// LX_UMA SHORT_UMA_MOD_B2
	{"pal"},	// LX_UMA SHORT_UMA_T_PRK_ALG
	{"pp1"},	// LX_UMA SHORT_UMA_T_PRK_P1
	{"pp2"},	// LX_UMA SHORT_UMA_T_PRK_P2
	{"rpk"},	// LX_UMA SHORT_UMA_T_PRK_PAL
	{"pvr"},	// LX_UMA SHORT_UMA_T_PRK_VAR
	{"rA "},	// LX_UMA SHORT_UMA_T_RATIO_A
	{"rB "},	// LX_UMA SHORT_UMA_T_RATIO_B
	{"RE "},	// LX_UMA SHORT_UMA_T_RND_ENV
	{"RF "},	// LX_UMA SHORT_UMA_T_RND_FINE
	{"RS "},	// LX_UMA SHORT_UMA_T_RND_SEED
	{"RW "},	// LX_UMA SHORT_UMA_T_RND_SWEEP
	{"RT "},	// LX_UMA SHORT_UMA_T_RND_TONE
	{"a1 "},	// LX_UMA SHORT_UMA_T_SLOT_AMT1
	{"vel"},	// LX_UMA SHORT_UMA_VEL
	{"a2 "},	// LX_UMA SHORT_UMA_T_SLOT_AMT2
	{"a3 "},	// LX_UMA SHORT_UMA_T_SLOT_AMT3
	{"a4 "},	// LX_UMA SHORT_UMA_T_SLOT_AMT4
	{"a5 "},	// LX_UMA SHORT_UMA_T_SLOT_AMT5
	{"a6 "},	// LX_UMA SHORT_UMA_T_SLOT_AMT6
	{"a7 "},	// LX_UMA SHORT_UMA_T_SLOT_AMT7
	{"a8 "},	// LX_UMA SHORT_UMA_T_SLOT_AMT8
	{"src"},	// LX_UMA SHORT_SRC
	{"pol"},	// LX_UMA SHORT_UMA_SLOT_POL
	{"lnA"},	// LX_UMA SHORT_UMA_TM_LEN_A
	{"prA"},	// LX_UMA SHORT_UMA_TM_PRB_A
	{"ckA"},	// LX_UMA SHORT_UMA_TM_CLK_A
	{"dvA"},	// LX_UMA SHORT_UMA_TM_DIV_A
	{"lnB"},	// LX_UMA SHORT_UMA_TM_LEN_B
	{"prB"},	// LX_UMA SHORT_UMA_TM_PRB_B
	{"ckB"},	// LX_UMA SHORT_UMA_TM_CLK_B
	{"dvB"},	// LX_UMA SHORT_UMA_TM_DIV_B
	{"lnC"},	// LX_UMA SHORT_UMA_TM_LEN_C
	{"prC"},	// LX_UMA SHORT_UMA_TM_PRB_C
	{"ckC"},	// LX_UMA SHORT_UMA_TM_CLK_C
	{"dvC"},	// LX_UMA SHORT_UMA_TM_DIV_C
	{"lnD"},	// LX_UMA SHORT_UMA_TM_LEN_D
	{"prD"},	// LX_UMA SHORT_UMA_TM_PRB_D
	{"ckD"},	// LX_UMA SHORT_UMA_TM_CLK_D
	{"dvD"},	// LX_UMA SHORT_UMA_TM_DIV_D
	{"lst"},	// LX_UMA SHORT_UMA_MORPH_LISTS
};
//-----------------------------------------------------------------
// These correspond with the catNamesEnum in menu.h
const char catNames[][16] =
{
	{""},
	//01234567
	{"Oscilltr"},
	{"Veloc EG"},
	{"Mod EG"},
	{"PitchMod"},
	{"FM"},
	{"Voice"},
	{"Noise"},
	{"Nois/Osc"},
	{"Filter"},
	{"Mod Osc"},
	{"LFO"},
	{"Transnt"},
	{"EQ"},
#if USE_CODEC_EQ		
	{"Limiter"},
#endif
	{"Pattern"},
	{"Sound"},
	{"Step"},
	{"Euklid"},		
	{"Global"},	
	{"Velocity"},
	{"Parametr"},
	{"Sequencr"},
	{"Generatr"},
	{"MIDI"},
	{"Clock"},
	{"Fx"},
	// LX_UMA: FM Drum and FM Tone
	{"Op A"},	// LX_UMA CAT_UMA_OP_A
	{"Op B"},	// LX_UMA CAT_UMA_OP_B
	{"Op C"},	// LX_UMA CAT_UMA_OP_C
	{"Op B2"},	// LX_UMA CAT_UMA_OP_B2
	{"Sub"},	// LX_UMA CAT_UMA_SUB
	{"LPG"},	// LX_UMA CAT_UMA_LPG
	{"Random"},	// LX_UMA CAT_UMA_RANDOM
	{"Machine"},	// LX_UMA CAT_UMA_MD
	{"Plaits"},	// LX_UMA CAT_UMA_PLA
	{"Mod"},	// LX_UMA CAT_MOD
	{"LFO B"},	// LX_UMA CAT_UMA_LFO_B
	{"Prk"},	// LX_UMA CAT_UMA_PRK
	{"ModSlot"},	// LX_UMA CAT_UMA_MOD_SLOT
	{"TuringA"},	// LX_UMA CAT_UMA_TM_A
	{"TuringB"},	// LX_UMA CAT_UMA_TM_B
	{"TuringC"},	// LX_UMA CAT_UMA_TM_C
	{"TuringD"},	// LX_UMA CAT_UMA_TM_D
};
//-----------------------------------------------------------------
// these must correspond to longNamesEnum
const char longNames[][16] =
{
	{""},
	//01234567
	{"Coarse"},
	{"Fine"},
	{"Attack"},
	{"Decay"},
	{"Amount"},
	{"Frequncy"},
	{"Overdriv"},
	{"Volume"},
	{"Panning"},
	{"Mix"},
	{"RepeatCt"},
	{"Resnance"},
	{"Type"},
	{"Gain"},
	{"Waveform"},
	{"DstParam"},
	{"ClockSnc"},
	{"Retriggr"},
	{"Offset"},		
	{"DstVoice"},
	{"Slope"},
	{"Dcy Clsd"},
	{"Dcy Open"},
#if USE_CODEC_EQ		
	{"Enable"},
	{"Release"},
	{"Threshold max"},
	{"Threshold min"},
#endif		
	{"RollRate"},
	{"Morph"},
	{"Note"},
	{"Prbablty"},
	{"Number"},
	{"Length"},
	{"Steps"},
	{"Rotation"},
	{"Tempo"},
	{"AudioOut"},
	{"Channel"},
	{"SampleRt"},
	{"NextPatt"},
	{"Phase"},
	{"Mode"},
	{"Vol mod"},
	{"Fetch"},	
	{"Follow"},	
	{"Quantize"},
	{"AutTrack"},
	{"Aut Dest"},
	{"AutValue"},
	{"Shuffle"},
	{"Screensv"},
	{"X Positn"},
	{"Y Positn"},
	{"Flux"},
	{"Velocity"},
	{"Freqcy 1"},
	{"Freqcy 2"},
	{"Gain 1"},
	{"Gain 2"},
	{"Routing"}, // midi routing
	{"Transmit"}, // midi tx filtering
	{"Receive"}, // midi rx filtering
	{"In PPQ"},
	{"Out1 PPQ"},
	{"Out2 PPQ"},
	{"GateMode"},
	{"PCReset" }, // reset bar counter on manual pattern change
	{"PulsWdth"},
	{"Color"},
	{"Dry/Wet"},
	{"Ratio"},
	{"Threshld"},
	{"Tone"},
	{"Feedback"},
	{"Range"},
	{"Time"},
	{"PreDelay"},
	{"Reflxion"},
	{"Param 1"},
	{"Param 2"},
	{"Param 3"},
	{"Param 4"},
	{"KitChMde"},
	{"Flam"},
	{"Shift"},
	// LX_UMA: FM Drum and FM Tone
	{"Engine"},	// LX_UMA LONG_UMA_ENGINE
	{"Algorthm"},	// LX_UMA LONG_UMA_ALGO
	{"Feedback"},	// LX_UMA LONG_UMA_FEEDBACK
	{"Ratio"},	// LX_UMA LONG_UMA_RATIO
	{"Mod"},	// LX_UMA LONG_UMA_MOD
	{"End lvl"},	// LX_UMA LONG_UMA_END
	{"Fine rt"},	// LX_UMA LONG_UMA_FINE_RATIO
	{"Wave C"},	// LX_UMA LONG_UMA_WAVE_C
	{"Wave AB"},	// LX_UMA LONG_UMA_WAVE_AB
	{"Detune"},	// LX_UMA LONG_UMA_DETUNE
	{"Mix X/Y"},	// LX_UMA LONG_UMA_MIX_XY
	{"Level"},	// LX_UMA LONG_UMA_LEVEL
	{"Shape"},	// LX_UMA LONG_UMA_SHAPE
	{"Octave"},	// LX_UMA LONG_UMA_OCTAVE
	{"Fold"},	// LX_UMA LONG_UMA_FOLD
	{"Hold"},	// LX_UMA LONG_UMA_HOLD
	{"Sustain"},	// LX_UMA LONG_UMA_SUSTAIN
	{"Release"},	// LX_UMA LONG_UMA_RELEASE
	{"Trg/Gate"},	// LX_UMA LONG_UMA_EG_MODE
	{"Env"},	// LX_UMA LONG_UMA_ENV
	{"Colour"},	// LX_UMA LONG_UMA_COLOUR
	{"Excite"},	// LX_UMA LONG_UMA_EXCITE
	{"FoldType"},	// LX_UMA LONG_UMA_FOLD_TYPE
	{"Symmetry"},	// LX_UMA LONG_UMA_SYMMETRY
	{"Body lvl"},	// LX_UMA LONG_UMA_BODY_LEV
	{"Palette"},	// LX_UMA LONG_UMA_PALETTE
	{"Sweep"},	// LX_UMA LONG_UMA_SWEEP
	{"Seed"},	// LX_UMA LONG_UMA_SEED
	{"Type"},	// LX_UMA LONG_UMA_TYPE
	{"Machine"},	// LX_UMA LONG_UMA_MACHINE
	{"Knob 1"},	// LX_UMA LONG_UMA_KNOB1
	{"Knob 2"},	// LX_UMA LONG_UMA_KNOB2
	{"Knob 3"},	// LX_UMA LONG_UMA_KNOB3
	{"Knob 4"},	// LX_UMA LONG_UMA_KNOB4
	{"Knob 5"},	// LX_UMA LONG_UMA_KNOB5
	{"Knob 6"},	// LX_UMA LONG_UMA_KNOB6
	{"Knob 7"},	// LX_UMA LONG_UMA_KNOB7
	{"Knob 8"},	// LX_UMA LONG_UMA_KNOB8
	{"Knobs"},	// LX_UMA LONG_UMA_KNOBS
	{"Bank"},	// LX_UMA LONG_UMA_PLA_BNK
	{"Model"},	// LX_UMA LONG_UMA_PLA_MDL
	{"Output"},	// LX_UMA LONG_UMA_PLA_OUT
	{"Harmonics"},	// LX_UMA LONG_UMA_PLA_HRM
	{"Timbre"},	// LX_UMA LONG_UMA_PLA_TMB
	{"Morph"},	// LX_UMA LONG_UMA_PLA_MOR
	{"Twist"},	// LX_UMA LONG_UMA_PLA_TWI
	{"File"},	// LX_UMA LONG_UMA_PLA_DXB
	{"Key mode"},	// LX_UMA LONG_UMA_PLA_KMD
	{"Key"},	// LX_UMA LONG_UMA_KEY
	{"Scale/chord"},	// LX_UMA LONG_UMA_SCALE
	{"1V/Oct"},	// LX_UMA LONG_1VOCT
	{"Param 1"},	// LX_UMA LONG_UMA_PARAM1
	{"Param 2"},	// LX_UMA LONG_UMA_PARAM2
	{"Variation"},	// LX_UMA LONG_UMA_VARIATION
	{"Amount 1"},	// LX_UMA LONG_UMA_AMOUNT1
	{"Velocity"},	// LX_UMA LONG_UMA_VELOCITY
	{"Amount 2"},	// LX_UMA LONG_UMA_AMOUNT2
	{"Amount 3"},	// LX_UMA LONG_UMA_AMOUNT3
	{"Amount 4"},	// LX_UMA LONG_UMA_AMOUNT4
	{"Amount 5"},	// LX_UMA LONG_UMA_AMOUNT5
	{"Amount 6"},	// LX_UMA LONG_UMA_AMOUNT6
	{"Amount 7"},	// LX_UMA LONG_UMA_AMOUNT7
	{"Amount 8"},	// LX_UMA LONG_UMA_AMOUNT8
	{"Source"},	// LX_UMA LONG_SRC
	{"Polarity"},	// LX_UMA LONG_UMA_POLARITY
	{"Prob"},	// LX_UMA LONG_UMA_PROB
	{"Clock"},	// LX_UMA LONG_UMA_CLOCK
	{"Div"},	// LX_UMA LONG_UMA_DIV
	{"MorphLst"},	// LX_UMA LONG_UMA_MORPH_LISTS
};


#endif /* MENUTEXT_H_ */
