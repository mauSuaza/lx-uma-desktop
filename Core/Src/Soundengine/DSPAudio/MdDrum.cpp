/*
 * MdDrum.cpp - UMA: the drum-machine engine over OP-XY-MNMD's machines (docs/design/md-engine.md). The machines
 * (mnmd/, MIT, see mnmd/LICENSE and mnmd/UPSTREAM_COMMIT) are used unchanged, through their core.h; this file is
 * the engine.cpp dispatcher rewritten to take a pitch (the upstream engine plays every machine at middle C), with
 * plain C functions for the firmware and the test app.
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "Soundengine/DSPAudio/MdDrum.h"
#include "mnmd/core.h"
#include <math.h>

namespace {
using opxy::i32;
using opxy::u32;

struct MdVoice
{
	mdkit::Voice voice;
	mdkit::Shared shared;
	uint8_t controls[MD_CONTROLS];
	u32 pitch;		// the phase increment the machine is played at (middle C: as voiced)
	u32 next;		// the machine for the next hit
};
MdVoice md_voices[MD_VOICES];

// as engine.cpp: a sound has ended once it stayed 12 dB under -90 dB for 90 ms, nothing still to come
const i32 quiet_under = (1 << 16) >> 2;
const u32 quiet_blocks = 62;

void md_begin(MdVoice& v)
{
	switch(v.voice.machine)
	{
    case 0: md::trx_bd::reset(v.voice.u.m0, v.shared.common, v.voice.owner); md::trx_bd::trigger(v.voice.u.m0, v.shared.common); break;
    case 1: md::trx_sd::reset(v.voice.u.m1, v.shared.common, v.voice.owner); md::trx_sd::trigger(v.voice.u.m1, v.shared.common); break;
    case 2: md::trx_xt::reset(v.voice.u.m2, v.shared.common, v.voice.owner); md::trx_xt::trigger(v.voice.u.m2, v.shared.common); break;
    case 3: md::trx_cp::reset(v.voice.u.m3, v.shared.common, v.voice.owner); md::trx_cp::trigger(v.voice.u.m3, v.shared.common); break;
    case 4: md::trx_rs::reset(v.voice.u.m4, v.shared.common, v.voice.owner); md::trx_rs::trigger(v.voice.u.m4, v.shared.common); break;
    case 5: md::trx_cb::reset(v.voice.u.m5, v.shared.common, v.voice.owner); md::trx_cb::trigger(v.voice.u.m5, v.shared.common); break;
    case 6: md::trx_ch::reset(v.voice.u.m6, v.shared.common, v.voice.owner); md::trx_ch::trigger(v.voice.u.m6, v.shared.common); break;
    case 7: md::trx_oh::reset(v.voice.u.m7, v.shared.common, v.voice.owner); md::trx_oh::trigger(v.voice.u.m7, v.shared.common); break;
    case 8: md::trx_cy::reset(v.voice.u.m8, v.shared.common, v.voice.owner); md::trx_cy::trigger(v.voice.u.m8, v.shared.common); break;
    case 9: md::trx_ma::reset(v.voice.u.m9, v.shared.common, v.voice.owner); md::trx_ma::trigger(v.voice.u.m9, v.shared.common); break;
    case 10: md::trx_cl::reset(v.voice.u.m10, v.shared.common, v.voice.owner); md::trx_cl::trigger(v.voice.u.m10, v.shared.common); break;
    case 11: md::trx_xc::reset(v.voice.u.m11, v.shared.common, v.voice.owner); md::trx_xc::trigger(v.voice.u.m11, v.shared.common); break;
    case 12: md::trx_b2::reset(v.voice.u.m12, v.shared.common, v.voice.owner); md::trx_b2::trigger(v.voice.u.m12, v.shared.common); break;
    case 13: md::efm_bd::reset(v.voice.u.m13, v.shared.common, v.voice.owner); md::efm_bd::trigger(v.voice.u.m13, v.shared.common); break;
    case 14: md::efm_sd::reset(v.voice.u.m14, v.shared.common, v.voice.owner); md::efm_sd::trigger(v.voice.u.m14, v.shared.common); break;
    case 15: md::efm_xt::reset(v.voice.u.m15, v.shared.common, v.voice.owner); md::efm_xt::trigger(v.voice.u.m15, v.shared.common); break;
    case 16: md::efm_cp::reset(v.voice.u.m16, v.shared.common, v.voice.owner); md::efm_cp::trigger(v.voice.u.m16, v.shared.common); break;
    case 17: md::efm_rs::reset(v.voice.u.m17, v.shared.common, v.voice.owner); md::efm_rs::trigger(v.voice.u.m17, v.shared.common); break;
    case 18: md::efm_cb::reset(v.voice.u.m18, v.shared.common, v.voice.owner); md::efm_cb::trigger(v.voice.u.m18, v.shared.common); break;
    case 19: md::efm_hh::reset(v.voice.u.m19, v.shared.common, v.voice.owner); md::efm_hh::trigger(v.voice.u.m19, v.shared.common); break;
    case 20: md::efm_cy::reset(v.voice.u.m20, v.shared.common, v.voice.owner); md::efm_cy::trigger(v.voice.u.m20, v.shared.common); break;
    case 21: md::pi_bd::reset(v.voice.u.m21, v.shared.common, v.voice.owner); md::pi_bd::trigger(v.voice.u.m21, v.shared.common); break;
    case 22: md::pi_sd::reset(v.voice.u.m22, v.shared.common, v.voice.owner); md::pi_sd::trigger(v.voice.u.m22, v.shared.common); break;
    case 23: md::pi_mt::reset(v.voice.u.m23, v.shared.common, v.voice.owner); md::pi_mt::trigger(v.voice.u.m23, v.shared.common); break;
    case 24: md::pi_ml::reset(v.voice.u.m24, v.shared.common, v.voice.owner); md::pi_ml::trigger(v.voice.u.m24, v.shared.common); break;
    case 25: md::pi_ma::reset(v.voice.u.m25, v.shared.common, v.voice.owner); md::pi_ma::trigger(v.voice.u.m25, v.shared.common); break;
    case 26: md::pi_rs::reset(v.voice.u.m26, v.shared.common, v.voice.owner); md::pi_rs::trigger(v.voice.u.m26, v.shared.common); break;
    case 27: md::pi_rc::reset(v.voice.u.m27, v.shared.common, v.voice.owner); md::pi_rc::trigger(v.voice.u.m27, v.shared.common); break;
    case 28: md::pi_cc::reset(v.voice.u.m28, v.shared.common, v.voice.owner); md::pi_cc::trigger(v.voice.u.m28, v.shared.common); break;
    case 29: md::pi_hh::reset(v.voice.u.m29, v.shared.common, v.voice.owner); md::pi_hh::trigger(v.voice.u.m29, v.shared.common); break;
    default: break;
	}
}

bool md_waiting(const MdVoice& v)
{
	switch(v.voice.machine)
	{
    case 3: return md::trx_cp::waiting(v.voice.u.m3);
    case 9: return md::trx_ma::waiting(v.voice.u.m9);
    case 10: return md::trx_cl::waiting(v.voice.u.m10);
    case 16: return md::efm_cp::waiting(v.voice.u.m16);
    case 27: return md::pi_cymbal::waiting(v.voice.u.m27);
    case 28: return md::pi_cymbal::waiting(v.voice.u.m28);
    case 29: return md::pi_cymbal::waiting(v.voice.u.m29);
    default: return false;
	}
}

void md_sound(MdVoice& v, opxy::Output out, const i32* own, opxy::Pitch pitch)
{
	switch(v.voice.machine)
	{
    case 0: md::trx_bd::render(v.voice.u.m0, v.shared.common, out, own, pitch); break;
    case 1: md::trx_sd::render(v.voice.u.m1, v.shared.common, out, own, pitch); break;
    case 2: md::trx_xt::render(v.voice.u.m2, v.shared.common, out, own, pitch); break;
    case 3: md::trx_cp::render(v.voice.u.m3, v.shared.common, out, own, pitch); break;
    case 4: md::trx_rs::render(v.voice.u.m4, v.shared.common, out, own, pitch); break;
    case 5: md::trx_cb::render(v.voice.u.m5, v.shared.common, out, own, pitch); break;
    case 6: md::trx_ch::render(v.voice.u.m6, v.shared.common, out, own, pitch); break;
    case 7: md::trx_oh::render(v.voice.u.m7, v.shared.common, out, own, pitch); break;
    case 8: md::trx_cy::render(v.voice.u.m8, v.shared.common, out, own, pitch); break;
    case 9: md::trx_ma::render(v.voice.u.m9, v.shared.common, out, own, pitch); break;
    case 10: md::trx_cl::render(v.voice.u.m10, v.shared.common, out, own, pitch); break;
    case 11: md::trx_xc::render(v.voice.u.m11, v.shared.common, out, own, pitch); break;
    case 12: md::trx_b2::render(v.voice.u.m12, v.shared.common, out, own, pitch); break;
    case 13: md::efm_bd::render(v.voice.u.m13, v.shared.common, out, own, pitch); break;
    case 14: md::efm_sd::render(v.voice.u.m14, v.shared.common, out, own, pitch); break;
    case 15: md::efm_xt::render(v.voice.u.m15, v.shared.common, out, own, pitch); break;
    case 16: md::efm_cp::render(v.voice.u.m16, v.shared.common, out, own, pitch); break;
    case 17: md::efm_rs::render(v.voice.u.m17, v.shared.common, out, own, pitch); break;
    case 18: md::efm_cb::render(v.voice.u.m18, v.shared.common, out, own, pitch); break;
    case 19: md::efm_hh::render(v.voice.u.m19, v.shared.common, out, own, pitch); break;
    case 20: md::efm_cy::render(v.voice.u.m20, v.shared.common, out, own, pitch); break;
    case 21: md::pi_bd::render(v.voice.u.m21, v.shared.common, out, own, pitch); break;
    case 22: md::pi_sd::render(v.voice.u.m22, v.shared.common, out, own, pitch); break;
    case 23: md::pi_mt::render(v.voice.u.m23, v.shared.common, out, own, pitch); break;
    case 24: md::pi_ml::render(v.voice.u.m24, v.shared.common, out, own, pitch); break;
    case 25: md::pi_ma::render(v.voice.u.m25, v.shared.common, out, own, pitch); break;
    case 26: md::pi_rs::render(v.voice.u.m26, v.shared.common, out, own, pitch); break;
    case 27: md::pi_rc::render(v.voice.u.m27, v.shared.common, out, own, pitch); break;
    case 28: md::pi_cc::render(v.voice.u.m28, v.shared.common, out, own, pitch); break;
    case 29: md::pi_hh::render(v.voice.u.m29, v.shared.common, out, own, pitch); break;
    default: for(unsigned i = 0; i < opxy::block_frames; ++i) out[i] = 0; break;
	}
}

// sound types: each type's machines, the families in order (T, F, P)
const uint8_t md_kick[] = {0, 12, 13, 21};
const uint8_t md_snare[] = {1, 14, 22};
const uint8_t md_clap[] = {3, 16};
const uint8_t md_hat[] = {6, 7, 19, 29};
const uint8_t md_cymbal[] = {8, 20, 27, 28};
const uint8_t md_perc[] = {2, 15, 23, 11, 4, 17, 26, 5, 18, 10, 9, 25, 24};	// toms, conga, rims, cowbells, claves, shakers, bar
const uint8_t* const md_types[MD_NUM_TYPES] = {md_kick, md_snare, md_clap, md_hat, md_cymbal, md_perc};
const uint8_t md_typeCounts[MD_NUM_TYPES] = {sizeof(md_kick), sizeof(md_snare), sizeof(md_clap), sizeof(md_hat), sizeof(md_cymbal), sizeof(md_perc)};
const char* const md_typeShorts[MD_NUM_TYPES] = {"kik", "snr", "clp", "hat", "cym", "prc"};
const char* const md_typeNames[MD_NUM_TYPES] = {"Kick", "Snare", "Clap", "Hat", "Cymbal", "Perc"};

const char* const md_shorts[MD_MACHINES] =
{
	"tbd", "tsd", "txt", "tcp", "trs", "tcb", "tch", "toh", "tcy", "tma", "tcl", "txc", "tb2",
	"fbd", "fsd", "fxt", "fcp", "frs", "fcb", "fhh", "fcy",
	"pbd", "psd", "pmt", "pml", "pma", "prs", "prc", "pcc", "phh",
};
const char* const md_names[MD_MACHINES] =
{
	"TRX BD", "TRX SD", "TRX XT", "TRX CP", "TRX RS", "TRX CB", "TRX CH", "TRX OH", "TRX CY", "TRX MA", "TRX CL",
	"TRX XC", "TRX B2",
	"EFM BD", "EFM SD", "EFM XT", "EFM CP", "EFM RS", "EFM CB", "EFM HH", "EFM CY",
	"P-I BD", "P-I SD", "P-I MT", "P-I ML", "P-I MA", "P-I RS", "P-I RC", "P-I CC", "P-I HH",
};

// a control's name in 3 letters: the first letter and the next two consonants, a few by hand
struct MdShortName { const char* label; const char* shown; };
const MdShortName md_controlShorts[] =
{
	{"PEAK", "pek"}, {"ROOM", "rom"}, {"CLSN", "csn"}, {"NOIS", "nos"}, {"TONE", "ton"}, {"RATE", "rat"},
	{"DUAL", "dul"}, {"SIZE", "siz"}, {"TUNE", "tun"},
};
int strcmp_(const char* a, const char* b)
{
	while(*a && *a == *b) { a++; b++; }
	return (unsigned char)*a - (unsigned char)*b;
}
unsigned len_(const char* s) { unsigned n = 0; while(s[n]) n++; return n; }
char md_shortBuf[MD_MACHINES][MD_CONTROLS][4];
bool md_shortsBuilt;

void md_buildShorts()
{
	for(uint8_t m = 0; m < MD_MACHINES; m++)
		for(uint8_t c = 0; c < MD_CONTROLS; c++)
		{
			const char* l = mdkit::kit::model[m].labels[c];
			char* o = md_shortBuf[m][c];
			o[0] = o[1] = o[2] = o[3] = 0;
			if(!l[0]) continue;
			bool done = false;
			for(const MdShortName& s : md_controlShorts)
				if(!strcmp_(s.label, l)) { o[0] = s.shown[0]; o[1] = s.shown[1]; o[2] = s.shown[2]; done = true; }
			if(done) continue;
			unsigned n = 0;
			for(unsigned i = 0; l[i] && n < 3; i++)
			{
				const char ch = (char)(l[i] >= 'A' && l[i] <= 'Z' ? l[i] - 'A' + 'a' : l[i]);
				const bool vowel = ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o' || ch == 'u';
				if(i == 0 || !vowel || len_(l) <= 3) o[n++] = ch;
			}
			for(unsigned i = 1; l[i] && n < 3; i++)	// too few consonants: fill from the start
				o[n++] = (char)(l[i] >= 'A' && l[i] <= 'Z' ? l[i] - 'A' + 'a' : l[i]);
		}
	md_shortsBuilt = true;
}
}

extern "C" {

uint8_t mdDrum_typeCount(uint8_t type) { return type < MD_NUM_TYPES ? md_typeCounts[type] : 0; }
uint8_t mdDrum_machineOf(uint8_t type, uint8_t index)
{
	if(type >= MD_NUM_TYPES) return 0;
	return md_types[type][index < md_typeCounts[type] ? index : md_typeCounts[type] - 1];
}
const char* mdDrum_typeShort(uint8_t type) { return md_typeShorts[type < MD_NUM_TYPES ? type : 0]; }
const char* mdDrum_typeName(uint8_t type) { return md_typeNames[type < MD_NUM_TYPES ? type : 0]; }
const char* mdDrum_machineShort(uint8_t machine) { return md_shorts[machine < MD_MACHINES ? machine : 0]; }
const char* mdDrum_machineName(uint8_t machine) { return md_names[machine < MD_MACHINES ? machine : 0]; }
uint8_t mdDrum_controlCount(uint8_t machine) { return mdkit::kit::model[machine < MD_MACHINES ? machine : 0].count; }
const char* mdDrum_controlName(uint8_t machine, uint8_t c)
{
	return c < MD_CONTROLS ? mdkit::kit::model[machine < MD_MACHINES ? machine : 0].labels[c] : "";
}
const char* mdDrum_controlShort(uint8_t machine, uint8_t c)
{
	if(!md_shortsBuilt) md_buildShorts();
	return c < MD_CONTROLS ? md_shortBuf[machine < MD_MACHINES ? machine : 0][c] : "";
}

// each machine's starting sound, from OP-XY-MNMD's own tables (2026-11-08): the TRX family's tools/presets.py
// defaults; EFM and P-I the design's defaults (docs/design/Main.dc.html DEFN) under their default kits' settings
// (tools/catalog.py KITS mdefm, mdpi)
static const uint8_t md_defaults[MD_MACHINES][MD_CONTROLS] =
{
	{ 40,  52,  44,  36,   0,  12,   0,   0},	// TRX-BD PTCH DEC RAMP RDEC STRT NOIS HARM CLIP
	{ 48,  50,  40,  30,  50,  64,  40,   0},	// TRX-SD PTCH DEC BUMP BENV SNAP TONE TUNE CLIP
	{ 90,  50,  40,  40,  16,   0,   0,   0},	// TRX-XT PTCH DEC RAMP RDEC DAMP DIST DTYP -
	{ 80,  64,  64,  40,  64,  40,  50,  64},	// TRX-CP CLPY TONE HARD RICH RATE ROOM RSIZ RTUN
	{ 64,  40,   0,   0,   0,   0,   0,   0},	// TRX-RS PTCH DEC DIST - - - - -
	{ 64,  60,  40,   0,  64,   0,   0,   0},	// TRX-CB PTCH DEC ENH DAMP TONE BUMP - -
	{  0,  64,   0, 127,  64,   0,   0,   0},	// TRX-CH GAP DEC HPF LPF MTAL - - -
	{ 32,  64,   0, 127,  64,   0,   0,   0},	// TRX-OH GAP DEC HPF LPF MTAL - - -
	{ 64,  80,  72,  64, 100,  84,   0,   0},	// TRX-CY RICH DEC TOP TTUN SIZE PEAK - -
	{  0,  40,   0,   0,  40,   0,  64,  64},	// TRX-MA ATT SUS REV DAMP RATL RTYP TONE HARD
	{ 64,  40,   0,  40,   0,   0,   0,   0},	// TRX-CL PTCH DEC DUAL ENH TUNE CLIC - -
	{ 80,  36,  30,  30,  70,   0,   0,   0},	// TRX-XC PTCH DEC RAMP RDEC DAMP DIST DTYP -
	{ 64,  60,  48,  20,  40,  10,   0,   0},	// TRX-B2 PTCH DEC RAMP HOLD TICK NOIS DIRT DIST
	{ 40,  60,  56,  36,  24,  32,  40,   0},	// EFM-BD PTCH DEC RAMP RDEC MOD MFRQ MDEC MFB
	{ 60,  40,  80,  56,  20,  48,  40,  16},	// EFM-SD PTCH DEC NOIS NDEC MOD MFRQ MDEC HPF
	{100,  56,  30,  36,  16,  32,  40,  40},	// EFM-XT PTCH DEC RAMP RDEC MOD MFRQ MDEC CLIC
	{ 72,  48,  32,  60,  64,  64,  64,  32},	// EFM-CP PTCH DEC CLPS CDEC MOD MFRQ MDEC HPF
	{ 72,  30,  40,  24,  40,  64,  30,  40},	// EFM-RS PTCH DEC MOD HPF SNAR SPTC SDEC SMOD
	{ 82,  44,  40,  24,   8,  64,  50,   0},	// EFM-CB PTCH DEC SNAP FB MOD MFRQ MDEC -
	{ 64,  24,   0,  64,  48,  96,  64,   0},	// EFM-HH PTCH DEC TREM TFRQ MOD MFRQ MDEC FB
	{ 64,  90,   0,  40,  40,  80,  80,   0},	// EFM-CY PTCH DEC FB HPF MOD MFRQ MDEC -
	{ 64,  36,  64,  64,  56,   0,   0,   0},	// PI-BD PTCH DEC HARD HAMR TENS DAMP - -
	{ 64,  50,  64,   0,  40, 100,  96,   0},	// PI-SD PTCH DEC HARD RING TENS RVOL RDEC -
	{ 92,  40,  64,  64,  64,   0,  44,   0},	// PI-MT PTCH DEC HARD HAMR TUNE DAMP SIZE POS
	{ 76,  36,  64,  24,   0,   0,   0,   0},	// PI-ML PTCH DEC HARD TENS - - - -
	{ 64,  50,  40,   0,  64,  64,   0,   0},	// PI-MA GRNS DEC GLEN - SIZE HARD - -
	{ 64,  50,  64,   0,  64,  40,   0,   0},	// PI-RS PTCH DEC HARD RING RVOL RDEC - -
	{ 64, 100,  88,   0,  64,   0,  44, 127},	// PI-RC PTCH DEC HARD RING AG AU BR GRAB
	{ 64, 104,  80,   0,  64,   0,  64, 127},	// PI-CC PTCH DEC HARD RING AG AU BR GRAB
	{ 64,  50,   0,   0,  64,   0,  64, 127},	// PI-HH PTCH DEC CLSN RING AG AU BR CLOS
};

void mdDrum_defaults(uint8_t machine, uint8_t controls[MD_CONTROLS])
{
	for(uint8_t c = 0; c < MD_CONTROLS; c++) controls[c] = md_defaults[machine < MD_MACHINES ? machine : 0][c];
}

uint8_t mdDrum_typeOf(uint8_t machine)
{
	for(uint8_t t = 0; t < MD_NUM_TYPES; t++)
		for(uint8_t i = 0; i < md_typeCounts[t]; i++)
			if(md_types[t][i] == machine) return t;
	return 0;
}

uint8_t mdDrum_family(uint8_t machine) { return machine < 13 ? 0 : machine < 21 ? 1 : 2; }

static u32 md_hash(u32 x)
{
	x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16;
	return x;
}
static u32 md_next(u32* r) { *r = md_hash(*r + 0x9e3779b9u); return *r; }
static int md_range(u32* r, int lo, int hi) { return lo + (int)(md_next(r) % (u32)(hi - lo + 1)); }

uint8_t mdDrum_randomMachine(uint8_t seed, uint8_t value, uint8_t drum, uint8_t palette, uint8_t home)
{
	u32 r = md_hash(0x6d61636bu ^ ((u32)seed | ((u32)value << 8) | ((u32)drum << 16)));
	uint8_t pool[MD_MACHINES], n = 0;
	const uint8_t type = mdDrum_typeOf(home), fam = mdDrum_family(home);
	for(uint8_t m = 0; m < MD_MACHINES; m++)
		if(palette == MD_PAL_ALL || (mdDrum_typeOf(m) == type && (palette != MD_PAL_FAM || mdDrum_family(m) == fam)))
			pool[n++] = m;
	return n ? pool[md_next(&r) % n] : home;
}

void mdDrum_randomKnobs(uint8_t seed, uint8_t value, uint8_t drum, uint8_t machine, uint8_t knobs[MD_CONTROLS])
{
	u32 r = md_hash(0x6b6e6f62u ^ ((u32)seed | ((u32)value << 8) | ((u32)drum << 16)));
	mdDrum_defaults(machine, knobs);
	for(uint8_t c = 0; c < MD_CONTROLS; c++)
	{
		const char* l = mdDrum_controlName(machine, c);
		const int a = md_range(&r, 0, 1000), b = md_range(&r, 0, 1000);	// two draws: a centred spread
		if(!l[0]) continue;
		int v;
		if(!strcmp_(l, "DEC"))
			// mostly short: 70 % 0-32, 20 % 33-80, 10 % 81-115 (127 holds some machines)
			v = a < 700 ? b * 32 / 1000 : a < 900 ? 33 + b * 47 / 1000 : 81 + b * 34 / 1000;
		else
		{
			const int spread = !strcmp_(l, "PTCH") ? 24 : 48;	// the pitch near its default, the rest wider
			v = knobs[c] + (a + b - 1000) * spread / 1000;
		}
		knobs[c] = (uint8_t)(v < 0 ? 0 : v > 127 ? 127 : v);
	}
}

void mdDrum_init(uint8_t v)
{
	if(v >= MD_VOICES) return;
	MdVoice& d = md_voices[v];
	d.voice = mdkit::Voice();
	d.shared = mdkit::Shared();
	d.voice.machine = d.next = 0;
	d.voice.owner = v;
	d.voice.done = 1;	// silent until the first hit
	d.pitch = md::kit::middle_c;
	mdDrum_defaults(0, d.controls);
}

void mdDrum_setMachine(uint8_t v, uint8_t machine)
{
	if(v >= MD_VOICES) return;
	md_voices[v].next = machine < MD_MACHINES ? machine : 0;
}

void mdDrum_setControl(uint8_t v, uint8_t c, uint8_t value)
{
	if(v >= MD_VOICES || c >= MD_CONTROLS) return;
	md_voices[v].controls[c] = value > 127 ? 127 : value;
}

void mdDrum_setTranspose(uint8_t v, float semitones)
{
	if(v >= MD_VOICES) return;
	if(semitones > 60.f) semitones = 60.f;
	if(semitones < -60.f) semitones = -60.f;
	const float inc = (float)md::kit::middle_c * powf(2.f, semitones / 12.f);
	md_voices[v].pitch = inc > 2.0e9f ? 2000000000u : (u32)inc;
}

void mdDrum_trigger(uint8_t v)
{
	if(v >= MD_VOICES) return;
	MdVoice& d = md_voices[v];
	d.voice.machine = d.next;
	d.voice.quiet = d.voice.done = 0;
	md_begin(d);
}

uint8_t mdDrum_finished(uint8_t v) { return v < MD_VOICES ? md_voices[v].voice.done != 0 : 1; }

void mdDrum_render(uint8_t v, float* out)
{
	if(v >= MD_VOICES) return;
	MdVoice& d = md_voices[v];
	if(d.voice.done)
	{
		for(unsigned i = 0; i < MD_BLOCK; i++) out[i] = 0.f;
		return;
	}
	i32 values[MD_CONTROLS], own[MD_CONTROLS], buf[opxy::block_frames];
	u32 pitch[opxy::block_frames];
	for(unsigned c = 0; c < MD_CONTROLS; c++) values[c] = (i32)((u32)d.controls[c] << 24);
	mdkit::kit::controls(d.voice.machine, values, own);
	for(unsigned i = 0; i < opxy::block_frames; i++) pitch[i] = d.pitch;
	md_sound(d, buf, own, pitch);
	unsigned under = 0;
	while(under < opxy::block_frames && buf[under] < quiet_under && buf[under] > -quiet_under) ++under;
	d.voice.quiet = under < opxy::block_frames ? 0 : d.voice.quiet + 1;
	if(d.voice.quiet >= quiet_blocks && !md_waiting(d)) d.voice.done = 1;
	const float scale = 1.f / 1073741824.f;	// the SDK's ceiling (half_scale) is 1
	for(unsigned i = 0; i < MD_BLOCK; i++) out[i] = (float)buf[i] * scale;
}

}
