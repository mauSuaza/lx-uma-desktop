/*
 * PlaitsDrum.cpp - UMA: the pla engine (docs/design/plaits-engine.md). Plaits' engines (mi/plaits, MIT, see
 * mi/README.md and mi/UPSTREAM_COMMIT) are used unchanged; this file plays them the way plaits::Voice does
 * (plaits/dsp/voice.cc), minus what the UMA voice does itself: the trigger delay, the internal decay envelope,
 * the low-pass gate and the limiter.
 *
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "Soundengine/DSPAudio/PlaitsDrum.h"
#include "uma_sdfiles.h"
#include <string.h>
#include "plaits/dsp/engine/engine.h"
#include "plaits/dsp/drums/analog_bass_drum.h"
#include "plaits/dsp/drums/synthetic_bass_drum.h"
#include "plaits/dsp/fx/overdrive.h"
#include "plaits/dsp/engine/wavetable_engine.h"
#include "plaits/dsp/engine/fm_engine.h"
#include "plaits/dsp/oscillator/sine_oscillator.h"
#include "plaits/dsp/downsampler/4x_downsampler.h"
#include "plaits/dsp/engine2/six_op_engine.h"	// plaits::FMVoice
#include "plaits/dsp/engine/modal_engine.h"
#include "plaits/dsp/engine2/wave_terrain_engine.h"
#include "plaits/dsp/physical_modelling/string_voice.h"
#include "plaits/dsp/engine/virtual_analog_engine.h"
#include "plaits/dsp/engine/waveshaping_engine.h"
#include "plaits/dsp/engine2/phase_distortion_engine.h"
#include "plaits/dsp/engine/additive_engine.h"
#include "plaits/dsp/engine/chord_engine.h"
#if PL_SPEECH
#include "plaits/dsp/engine/speech_engine.h"
#endif
#include "stmlib/dsp/limiter.h"
#include "plaits/resources.h"
#include "stmlib/dsp/hysteresis_quantizer.h"
#include <algorithm>
#include <math.h>
#include <new>
extern "C" {
#include "Soundengine/DSPAudio/FmDrum.h"	// FM Tone's sub oscillator (0.34e)
}

namespace {

static_assert(PL_CHUNK <= plaits::kMaxBlockSize, "one Render is at most Plaits' kMaxBlockSize");
static_assert(PL_BLOCK % PL_CHUNK == 0, "a block is whole chunks");

// the engines' own memory (BufferAllocator), the largest model's need (section 3.4): the inharmonic string's delay
// lines (1024 + 256 floats, 5 KB). The wavetable asks for 64 wave pointers but fills 256 (4 banks of 8 x 8,
// wavetable_engine.cc LoadUserData): harmless on Plaits, where every engine starts at the base of one 16 KB arena;
// here the voice's arena must hold all 256
const size_t pl_ramSize_ = (1024 + 256) * sizeof(float) + 64;
const size_t pl_ramSize = pl_ramSize_ > 256 * sizeof(const int16_t*) + 32 ? pl_ramSize_ : 256 * sizeof(const int16_t*) + 32;

// the bass drum (0.32a): Plaits' two drums and its overdrive, as plaits::BassDrumEngine::Render
// (dsp/engine/bass_drum_engine.cc) combines them, line for line; but only the drum that is heard is rendered
// (out: the analog one, aux: the synthetic one, mix: both), since the analog drum alone is about 3/4 of the cost
struct PlBassDrum
{
	plaits::AnalogBassDrum analog;
	plaits::SyntheticBassDrum synthetic;
	plaits::Overdrive overdrive;

	void Init()
	{
		analog.Init();
		synthetic.Init();
		overdrive.Init();
	}

	void Render(const plaits::EngineParameters& parameters, float* out, float* aux, size_t size, bool doOut, bool doAux)
	{
		using std::min;
		using std::max;
		const float f0 = plaits::NoteToFrequency(parameters.note);
		const float punch = plaits::ApplyMacro(1.0f, 0.25f, 1.75f, parameters.macro);	// TWIST (Palette): punch
		const float attack_fm_amount = min(parameters.harmonics * 4.0f, 1.0f) * punch;
		const float self_fm_amount = max(min(parameters.harmonics * 4.0f - 1.0f, 1.0f), 0.0f);
		const float drive = max(parameters.harmonics * 2.0f - 1.0f, 0.0f) * max(1.0f - 16.0f * f0, 0.0f);
		const bool sustain = parameters.trigger & plaits::TRIGGER_UNPATCHED;
		if(doOut)
		{
			analog.Render(sustain, parameters.trigger & plaits::TRIGGER_RISING_EDGE, parameters.accent, f0,
				parameters.timbre, parameters.morph, attack_fm_amount, self_fm_amount, out, size);
			overdrive.Process(0.5f + 0.5f * drive, out, size);
		}
		if(doAux)
			synthetic.Render(sustain, parameters.trigger & plaits::TRIGGER_RISING_EDGE, parameters.accent, f0,
				parameters.timbre, parameters.morph,
				sustain ? parameters.harmonics : 0.4f - 0.25f * parameters.morph * parameters.morph,
				min(parameters.harmonics * 2.0f, 1.0f), max(parameters.harmonics * 2.0f - 1.0f, 0.0f), aux, size);
	}
};

// the 2-op FM (0.32f): plaits::FMEngine::Render (dsp/engine/fm_engine.cc) line for line, with Plaits' own SinePM and
// Downsampler, but the sub-oscillator (heard only on aux) is computed only when aux is heard: a third of the sine
// lookups of this 4x oversampled engine. The sub's phase always advances, so the state matches Plaits'
struct PlFm2
{
	uint32_t carrier_phase_, modulator_phase_, sub_phase_;
	float previous_carrier_frequency_, previous_modulator_frequency_, previous_amount_, previous_feedback_;
	float previous_sample_, sub_fir_, carrier_fir_;

	void Init()
	{
		carrier_phase_ = modulator_phase_ = sub_phase_ = 0;
		previous_carrier_frequency_ = previous_modulator_frequency_ = plaits::a0;
		previous_amount_ = previous_feedback_ = previous_sample_ = 0.0f;
		sub_fir_ = carrier_fir_ = 0.0f;	// (FMEngine::Init leaves them as the object was: zero here as there)
	}

	void Render(const plaits::EngineParameters& parameters, float* out, float* aux, size_t size, bool doAux)
	{
		using namespace plaits;
		using stmlib::ParameterInterpolator;
		const float note = parameters.note - 24.0f;	// 4x oversampling
		const float stock_ratio = stmlib::Interpolate(lut_fm_frequency_quantizer, parameters.harmonics, 128.0f);
		// TWIST (Palette, its continuous "stock" tuning): the modulator +-12 semitones around the ratio
		const float ratio = ApplyMacro(stock_ratio, stock_ratio - 12.0f, stock_ratio + 12.0f, parameters.macro);
		float modulator_note = note + ratio;
		float target_modulator_frequency = NoteToFrequency(modulator_note);
		CONSTRAIN(target_modulator_frequency, 0.0f, 0.5f);
		float hf_taming = 1.0f - (modulator_note - 72.0f) * 0.025f;
		CONSTRAIN(hf_taming, 0.0f, 1.0f);
		hf_taming *= hf_taming;
		ParameterInterpolator carrier_frequency(&previous_carrier_frequency_, NoteToFrequency(note), size);
		ParameterInterpolator modulator_frequency(&previous_modulator_frequency_, target_modulator_frequency, size);
		ParameterInterpolator amount_modulation(&previous_amount_,
			2.0f * parameters.timbre * parameters.timbre * hf_taming, size);
		ParameterInterpolator feedback_modulation(&previous_feedback_, 2.0f * parameters.morph - 1.0f, size);
		Downsampler carrier_downsampler(&carrier_fir_);
		Downsampler sub_downsampler(&sub_fir_);
		while(size--)
		{
			const float max_uint32 = 4294967296.0f;
			const float amount = amount_modulation.Next();
			const float feedback = feedback_modulation.Next();
			float phase_feedback = feedback < 0.0f ? 0.5f * feedback * feedback : 0.0f;
			const uint32_t carrier_increment = static_cast<uint32_t>(max_uint32 * carrier_frequency.Next());
			float _modulator_frequency = modulator_frequency.Next();
			for(size_t j = 0; j < kOversampling; ++j)
			{
				modulator_phase_ += static_cast<uint32_t>(max_uint32 * _modulator_frequency * (1.0f + previous_sample_ * phase_feedback));
				carrier_phase_ += carrier_increment;
				sub_phase_ += carrier_increment >> 1;
				float modulator_fb = feedback > 0.0f ? 0.25f * feedback * feedback : 0.0f;
				float modulator = SinePM(modulator_phase_, modulator_fb * previous_sample_);
				float carrier = SinePM(carrier_phase_, amount * modulator);
				ONE_POLE(previous_sample_, carrier, 0.05f);
				carrier_downsampler.Accumulate(j, carrier);
				if(doAux)
					sub_downsampler.Accumulate(j, SinePM(sub_phase_, amount * carrier * 0.25f));
			}
			*out++ = carrier_downsampler.Read();
			*aux++ = doAux ? sub_downsampler.Read() : 0.0f;
		}
	}
};

// the 6-op FM (0.33a, docs/design/plaits-6op.md): one plaits::FMVoice (Plaits' engine plays two, alternating), driven
// as SixOpEngine::Render drives its active voice, at the module's own rate (exact pitch and envelope times). The patch:
// HARMONICS (32 steps with Plaits' hysteresis), unpacked from the bank at the hit when it changed. Rendered in steps of
// 32 samples, as SixOpEngine renders each voice (two of its blocks at a time): the DX7 envelopes advance once a step
alignas(8) char pl_dxAlgorithmsStore[sizeof(plaits::fm::Algorithms<6>)];	// the 32 algorithms, shared (3 KB)
plaits::fm::Algorithms<6>* pl_dxAlgorithms;

// a bank's 32 packed patches: A-C Plaits', then the SD card's (0.34b, uma_sdfiles.c): 0 while a card bank is
// loading, or if it's bad or gone
const uint8_t* pl_dxBankData(uint8_t bank)
{
	if(bank < PL_DX_BUILTIN_BANKS) return plaits::fm_patches_table[bank];
	return umaSdf_data(UMA_SDF_DX7, (uint8_t)(bank - PL_DX_BUILTIN_BANKS));
}

struct PlSixOp
{
	plaits::FMVoice voice;
	plaits::fm::Patch patch[2];	// alternating: FMVoice::LoadPatch ignores the pointer it already has
	uint8_t which;				// the patch[] in use
	uint8_t bank, index;		// what's loaded (0xff: nothing yet; 0xfe: bank A in place of a card bank still loading)
	uint8_t gen;				// the card's scan the bank came from (a new scan can put another file at its place)
	stmlib::HysteresisQuantizer2 patchQuantizer;
	float temp[32 * 3];		// the voice's three scratch buffers (fm::Voice::Render) for a 32-sample step
	float fs;
	float postFilter;		// TWIST's tone filter (Palette's SixOpEngine post_filter_)

	void Init(float fs_)
	{
		fs = fs_;
		if(!pl_dxAlgorithms)
		{
			pl_dxAlgorithms = new(pl_dxAlgorithmsStore) plaits::fm::Algorithms<6>();
			pl_dxAlgorithms->Init();
		}
		voice.Init(pl_dxAlgorithms, fs_);
		patchQuantizer.Init(32, 0.005f, false);
		postFilter = 0.0f;
		which = 0;
		bank = index = 0xff;
		gen = 0;
	}

	// the patch to play, unpacked when it changed. A card bank (0.34b) still loading: the patch the voice has keeps
	// playing (bank A if it has none yet) and the next hit looks again; a bad or missing one plays bank A
	void Select(uint8_t wantBank, int patchIndex)
	{
		const bool card = wantBank >= PL_DX_BUILTIN_BANKS;
		const uint8_t g = umaSdf_generation();
		if(wantBank == bank && patchIndex == index && (!card || g == gen))
			return;
		const uint8_t* d = pl_dxBankData(wantBank);
		uint8_t got = wantBank;
		if(!d)
		{
			const bool loading = umaSdf_status(UMA_SDF_DX7, (uint8_t)(wantBank - PL_DX_BUILTIN_BANKS)) == UMA_SDF_LOADING;
			if(loading && bank != 0xff && bank != 0xfe)
				return;	// keep the patch it has
			d = pl_dxBankData(0);
			if(loading) got = 0xfe;	// bank A meanwhile, looked at again next hit
		}
		which ^= 1;
		patch[which].Unpack(d + patchIndex * plaits::fm::Patch::SYX_SIZE);
		bank = got;
		index = (uint8_t)patchIndex;
		gen = g;
		voice.UnloadPatch();
	}

	// as SixOpEngine::Render for its active voice: on a hit the patch and the LFO; the key held while gate
	void Render(const plaits::EngineParameters& parameters, float note, uint8_t wantBank, float* out, size_t size)
	{
		using namespace plaits;
		const int patchIndex = patchQuantizer.Process(parameters.harmonics * 1.02f);
		if(parameters.trigger & TRIGGER_RISING_EDGE)
		{
			Select(wantBank, patchIndex);
			voice.LoadPatch(&patch[which]);
			voice.mutable_lfo()->Reset();
		}
		fm::Voice<6>::Parameters* p = voice.mutable_parameters();
		p->note = note;
		p->velocity = parameters.accent;
		p->envelope_control = parameters.morph;
		p->brightness = parameters.timbre;
		p->sustain = false;
		p->gate = (parameters.trigger & TRIGGER_HIGH) != 0;
		p->modulator_detune = 24.0f * (parameters.macro - 0.5f);	// TWIST (Palette): the modulators +-12 st
		voice.mutable_lfo()->Step(float(size));
		voice.set_modulations(voice.lfo());
		Out(out, size, parameters.macro);
	}

	// frozen: as SixOpEngine::Render with nothing patched (TRIGGER_UNPATCHED) for its voice 0: the patch follows
	// HARMONICS at once, every envelope held at the moment MORPH picks (fm::Voice in sustain), the LFO scrubbed to it
	void RenderFrozen(const plaits::EngineParameters& parameters, float note, uint8_t wantBank, float* out, size_t size)
	{
		using namespace plaits;
		const int patchIndex = patchQuantizer.Process(parameters.harmonics * 1.02f);
		Select(wantBank, patchIndex);
		const float t = parameters.morph;
		voice.mutable_lfo()->Scrub(2.0f * fs * t);
		voice.LoadPatch(&patch[which]);
		fm::Voice<6>::Parameters* p = voice.mutable_parameters();
		p->sustain = true;
		p->gate = false;
		p->note = note;
		p->velocity = parameters.accent;
		p->brightness = parameters.timbre;
		p->envelope_control = t;
		p->modulator_detune = 24.0f * (parameters.macro - 0.5f);	// TWIST (Palette)
		voice.set_modulations(voice.lfo());
		Out(out, size, parameters.macro);
	}

	// TWIST (Palette's SixOpEngine::RenderMonoOutput): below noon darker (a one-pole low-pass), above noon driven
	// (a soft limiter blended in); at noon as Plaits
	void Out(float* out, size_t size, float macro)
	{
		for(size_t i = 0; i < size * 3; i++) temp[i] = 0.0f;
		voice.Render(temp, size);
		if(macro < 0.5f)
		{
			const float coefficient = 1.0f - (0.5f - macro) * 2.0f * 0.92f;
			for(size_t i = 0; i < size; i++)
			{
				ONE_POLE(postFilter, stmlib::SoftClip(temp[i] * 0.25f), coefficient);
				out[i] = postFilter;
			}
		}
		else if(macro > 0.5f)
		{
			const float saturation = (macro - 0.5f) * 2.0f;
			for(size_t i = 0; i < size; i++)
			{
				const float sample = stmlib::SoftClip(temp[i] * 0.25f);
				postFilter = sample;
				out[i] = sample + (stmlib::SoftLimit(sample * 3.0f) - sample) * saturation;
			}
		}
		else
			for(size_t i = 0; i < size; i++) out[i] = postFilter = stmlib::SoftClip(temp[i] * 0.25f);
	}
};

// the inharmonic string (0.33j): one plaits::StringVoice, driven as StringEngine::Render drives its active string
// (Plaits' engine plays three in turn so tails overlap: three times the memory and CPU). Its delay lines come from
// the voice's arena
struct PlString
{
	plaits::StringVoice voice;
	float temp[plaits::kMaxBlockSize];

	void Init(stmlib::BufferAllocator* allocator)
	{
		voice.Init(allocator);
		voice.Reset();
	}

	void Render(const plaits::EngineParameters& parameters, float* out, float* aux, size_t size)
	{
		using namespace plaits;
		for(size_t i = 0; i < size; i++) out[i] = aux[i] = 0.0f;
		voice.Render(parameters.trigger & TRIGGER_UNPATCHED, parameters.trigger & TRIGGER_RISING_EDGE,
			parameters.accent, NoteToFrequency(parameters.note), parameters.harmonics,
			parameters.timbre * parameters.timbre, parameters.morph, temp, out, aux, size,
			ApplyMacro(1.0f, 0.25f, 4.0f, parameters.macro));	// TWIST (Palette): the pluck's length
	}
};

// one model per voice, built in place (no static constructors, no heap): the selected model only. Models other
// than the bass drum (0.32 onward) are plaits::Engine subclasses in this store too
union PlEngineStore
{
	char bassDrum[sizeof(PlBassDrum)];
	char wavetable[sizeof(plaits::WavetableEngine)];
	char fm2[sizeof(PlFm2)];
	char sixOp[sizeof(PlSixOp)];
	char modal[sizeof(plaits::ModalEngine)];
	char string[sizeof(PlString)];
	char terrain[sizeof(plaits::WaveTerrainEngine)];
	char va[sizeof(plaits::VirtualAnalogEngine)];
	char waveshape[sizeof(plaits::WaveshapingEngine)];
	char pd[sizeof(plaits::PhaseDistortionEngine)];
	char additive[sizeof(plaits::AdditiveEngine)];
	char chords[sizeof(plaits::ChordEngine)];
#if PL_SPEECH
	char speech[sizeof(plaits::SpeechEngine)];
#endif
};

// the silent skip: once a hit's output stayed under -94 dB for 40 ms the voice stops rendering until the next hit
const float pl_quietLevel = 2e-5f;
const uint8_t pl_quietBlocks = 28;	// 28 x 64 samples, 41 ms

struct PlVoice
{
	alignas(8) PlEngineStore store;
	alignas(8) char ram[pl_ramSize];
	plaits::Engine* engine;	// a plaits::Engine model, or 0 (the bass drum: bassDrum, the 2-op FM: fm2)
	PlBassDrum* bassDrum;
	PlFm2* fm2;
	PlSixOp* sixOp;
	PlString* string;
	stmlib::Limiter limiter;	// out of the models Plaits limits (a negative out gain in voice.cc: modal, string, speech)
	bool enveloped;		// the speech engine's last word: a word or a sentence shapes its own volume, a vowel doesn't
	uint8_t dxBank;		// the 6-op's bank (A B C, then the SD card's)
	uint8_t file;		// the wavetable's or terrain's file: 0 built in, then the SD card's (0.34d)
	const uint8_t* user;	// the card file the engine plays from (LoadUserData), 0: the built-in tables
	uint8_t userKind, userIndex;	// which file that is (uma_sdfiles.h)
	FmToneSub sub;		// the sub oscillator on the wavetable and terrain (0.34e, docs/design/plaits-sub.md)
	uint8_t subLevel, subShape, subOct, subFold;	// fmt's settings: 0-127 (0 off), 0-3, 0-3 (-1..-4), 0-127
	uint8_t dxKey;		// the 6-op's key mode (PL_DXK_*)
	bool gate;			// the key (the 6-op): the trigger held
	uint8_t tap;		// chunks the key is held by a hit alone: a trigger pulse (1.5 ms), as Plaits' trigger input
						// sees. A voice loading a new patch spends its first render setting up, so a 1-chunk tap
						// would never be seen as a key press
	uint8_t model;		// the engine built in store
	uint8_t next;		// the model for the next hit
	uint8_t output;		// the setting
	uint8_t playing;	// the output of the hit that sounds (taken at the hit: the other drum isn't running)
	bool silent;		// the last hit has died away (or there was none): nothing is rendered
	uint8_t quiet;		// blocks in a row under pl_quietLevel
	bool rising;		// a hit for the next chunk
	float accent;
	float fs;
	float note;
	float hz;			// the pitch asked for (the 6-op plays it as a MIDI note at its own rate)
	float harmonics, timbre, morph;
	float twist;		// Palette's TWIST (the fourth macro), 0..1, 0.5 = as Plaits
	float outGain, auxGain;	// as plaits::Voice registers the engine (voice.cc), signs dropped (Plaits inverts on the jack)
};
PlVoice pl_voices[PL_VOICES];

// speech (0.33k) needs 14.6 KB of working memory (its word bank's 1024 LPC frames): one speech engine is shared by all
// voices, like a speech chip. The voice whose hit builds it takes it over; the voice that had it goes quiet until its
// own next hit (speech starts each word afresh anyway)
#if PL_SPEECH
alignas(8) char pl_speechRam[15360];
int8_t pl_speechOwner = -1;
#endif

void pl_build(PlVoice& v, uint8_t model)
{
	v.engine = 0;
	v.bassDrum = 0;
	v.fm2 = 0;
	v.sixOp = 0;
	v.string = 0;
	v.limiter.Init();
	switch(model)
	{
	default:
	case PL_MODEL_BASS_DRUM:
		v.bassDrum = new(&v.store) PlBassDrum();
		v.bassDrum->Init();
		v.outGain = 0.8f; v.auxGain = 0.8f;	// as voice.cc registers the bass drum
		model = PL_MODEL_BASS_DRUM;
		break;
	case PL_MODEL_WAVETABLE:
		v.engine = new(&v.store) plaits::WavetableEngine();
		v.outGain = 0.6f; v.auxGain = 0.6f;	// voice.cc
		break;
	case PL_MODEL_DX7:
		v.sixOp = new(&v.store) PlSixOp();
		v.sixOp->Init(v.fs);	// at the module's rate
		v.outGain = 1.0f; v.auxGain = 1.0f;	// voice.cc; out and aux are the same signal
		break;
	case PL_MODEL_MODAL:
		v.engine = new(&v.store) plaits::ModalEngine();
		v.outGain = -1.0f; v.auxGain = 0.8f;	// voice.cc: out through a limiter
		break;
	case PL_MODEL_STRING:
		v.string = new(&v.store) PlString();
		v.outGain = -1.0f; v.auxGain = 0.8f;	// voice.cc: out through a limiter
		break;
	case PL_MODEL_TERRAIN:
		v.engine = new(&v.store) plaits::WaveTerrainEngine();
		v.outGain = 0.7f; v.auxGain = 0.7f;	// voice.cc
		break;
	case PL_MODEL_VA:
		v.engine = new(&v.store) plaits::VirtualAnalogEngine();
		v.outGain = 0.8f; v.auxGain = 0.8f;	// the gains: voice.cc
		break;
	case PL_MODEL_WAVESHAPE:
		v.engine = new(&v.store) plaits::WaveshapingEngine();
		v.outGain = 0.7f; v.auxGain = 0.6f;
		break;
	case PL_MODEL_PD:
		v.engine = new(&v.store) plaits::PhaseDistortionEngine();
		v.outGain = 0.7f; v.auxGain = 0.7f;
		break;
	case PL_MODEL_ADDITIVE:
		v.engine = new(&v.store) plaits::AdditiveEngine();
		v.outGain = 0.8f; v.auxGain = 0.8f;
		break;
	case PL_MODEL_CHORDS:
		v.engine = new(&v.store) plaits::ChordEngine();
		v.outGain = 0.8f; v.auxGain = 0.8f;
		break;
#if PL_SPEECH
	case PL_MODEL_SPEECH:
	{
		const int8_t me = (int8_t)(&v - pl_voices);
		if(pl_speechOwner >= 0 && pl_speechOwner != me)
		{
			PlVoice& o = pl_voices[pl_speechOwner];	// the voice that had it: quiet, rebuilt at its next hit
			if(o.model == PL_MODEL_SPEECH)
			{
				o.engine = 0;
				o.model = PL_MODEL_NONE;
				o.silent = true;
			}
		}
		pl_speechOwner = me;
		plaits::SpeechEngine* sp = new(&v.store) plaits::SpeechEngine();
		v.engine = sp;
		v.outGain = -0.7f; v.auxGain = 0.8f;	// out through the limiter at 0.7
		break;
	}
#endif
	case PL_MODEL_FM2:
		v.fm2 = new(&v.store) PlFm2();	// 4x oversampled inside
		v.fm2->Init();
		v.outGain = 0.6f; v.auxGain = 0.6f;	// voice.cc
		break;
	}
	v.model = model;
	if(v.string)
	{
		stmlib::BufferAllocator allocator(v.ram, sizeof(v.ram));
		v.string->Init(&allocator);
	}
	if(v.engine)
	{
#if PL_SPEECH
		stmlib::BufferAllocator allocator(model == PL_MODEL_SPEECH ? pl_speechRam : v.ram,
			model == PL_MODEL_SPEECH ? sizeof(pl_speechRam) : sizeof(v.ram));
#else
		stmlib::BufferAllocator allocator(v.ram, sizeof(v.ram));
#endif
		v.engine->Init(&allocator);
		v.engine->Reset();
		v.engine->LoadUserData(0);	// as plaits::Voice does: no user data, the built-in tables (the wavetable's map)
	}
	v.user = 0;	// (a card file: pl_pickUser, at the hit)
	umaSdf_setUsing((uint8_t)(&v - pl_voices), UMA_SDF_NONE, 0);
#if PL_SPEECH
	if(model == PL_MODEL_SPEECH)
	{
		// as plaits::Voice with the trigger patched and its FM and MORPH attenuverters at noon: no prosody, the
		// words at their own speed
		plaits::SpeechEngine* sp = static_cast<plaits::SpeechEngine*>(v.engine);
		sp->set_prosody_amount(0.0f);
		sp->set_speed(0.0f);
	}
#endif
	v.enveloped = false;
}

float pl_clamp01(float x) { return x < 0.f ? 0.f : x > 1.f ? 1.f : x; }

// the card files a model plays (0.34d): the wavetable's (bank D) and the terrain's (a 9th terrain)
uint8_t pl_userKind(uint8_t model)
{
	return model == PL_MODEL_WAVETABLE ? UMA_SDF_WAVES : model == PL_MODEL_TERRAIN ? UMA_SDF_TERRAIN : UMA_SDF_NONE;
}

void pl_build(PlVoice& v, uint8_t model);
void pl_setUser(PlVoice& v, const uint8_t* d, uint8_t kind, uint8_t index)
{
	if(!d && v.user && v.model == PL_MODEL_TERRAIN)
	{
		// the terrain glides between terrains (its terrain_ follows HARMONICS): from the card's 9th without it, it
		// would read no terrain at all. Built afresh instead (Plaits never drops a terrain while it plays)
		pl_build(v, v.model);
		return;
	}
	v.engine->LoadUserData(d);
	v.user = d;
	v.userKind = kind;
	v.userIndex = index;
	umaSdf_setUsing((uint8_t)(&v - pl_voices), d ? kind : UMA_SDF_NONE, index);
}

// at a hit: the voice's file if it's loaded, the built-in tables if it's none, bad or gone; while it loads, what
// the voice plays already
void pl_pickUser(PlVoice& v)
{
	const uint8_t kind = pl_userKind(v.model);
	if(kind == UMA_SDF_NONE || !v.engine) return;
	const uint8_t index = (uint8_t)(v.file - 1);
	if(v.file == 0 || index >= umaSdf_count(kind))
	{
		if(v.user) pl_setUser(v, 0, kind, 0);
		return;
	}
	const uint8_t* d = umaSdf_data(kind, index);
	if(d)
	{
		if(d != v.user || v.userKind != kind || v.userIndex != index) pl_setUser(v, d, kind, index);
	}
	else if(umaSdf_status(kind, index) == UMA_SDF_BAD && v.user)
		pl_setUser(v, 0, kind, 0);
}

// every block: the card file the engine reads is still in its slot (a rescan or a replaced slot: the built-in tables)
void pl_checkUser(PlVoice& v)
{
	if(v.user && umaSdf_data(v.userKind, v.userIndex) != v.user) pl_setUser(v, 0, v.userKind, 0);
}

// the models, for the menu (names at most 12 letters, from Plaits' manual)
struct PlModelInfo
{
	const char* shortName;
	const char* name;
	const char* knobs[3];		// HARMONICS, TIMBRE, MORPH
	const char* twist;			// what TWIST does (Palette's fourth macro)
	const char* outputs[3];		// out, aux, mix
	uint8_t selfEnveloped;
};
const PlModelInfo pl_models[PL_NUM_MODELS] =
{
	{"bd", "Bass drum", {"Attack/drive", "Brightness", "Decay"}, "Punch", {"Analog", "Synthetic", "Both"}, 1},
	{"wav", "Wavetable", {"Bank", "Row", "Column"}, "Phase warp", {"Clean", "Lo-fi 5-bit", "Both"}, 0},
	{"fm2", "2-op FM", {"Ratio", "Mod index", "Feedback"}, "Ratio shift", {"FM", "Sub osc", "Both"}, 0},
	{"dx7", "6-op FM", {"Patch", "Brightness", "Env time"}, "Detune/tone", {"Out", "Out", "Out"}, 1},
	{"mod", "Modal reso", {"Material", "Brightness", "Decay"}, "Hardness", {"Resonator", "Exciter", "Both"}, 1},
	{"str", "Inharm str", {"Inharmonic", "Brightness", "Decay"}, "Pluck size", {"String", "Exciter", "Both"}, 1},
	{"ter", "Wave terrain", {"Terrain", "Path size", "Path move"}, "Path Y", {"Terrain", "Path sine", "Both"}, 0},
	{"va", "Virtual anlg", {"Detune", "Square", "Saw"}, "Saw/square", {"Waves", "Sync", "Both"}, 0},
	{"shp", "Waveshaping", {"Shaper", "Fold", "Asymmetry"}, "Symmetry", {"Fold", "Fold 2", "Both"}, 0},
	{"pd", "Phase dist", {"Ratio", "Amount", "Asymmetry"}, "Mod octave", {"Sync PD", "Free PM", "Both"}, 0},
	{"add", "Harmonic", {"Peaks", "Prominent", "Peak shape"}, "Odd/even", {"Additive", "Drawbars", "Both"}, 0},
	{"chd", "Chords", {"Chord", "Inversion", "Waveform"}, "Root level", {"Chord", "Root", "Both"}, 0},
	{"spk", "Speech", {"Synthesis", "Species", "Phoneme"}, "Sharpness", {"Voice", "Vocal cord", "Both"}, 0},
};
const uint8_t pl_drums[] = {PL_MODEL_BASS_DRUM, PL_MODEL_MODAL, PL_MODEL_STRING};
const uint8_t pl_tonal[] = {PL_MODEL_WAVETABLE, PL_MODEL_FM2, PL_MODEL_DX7, PL_MODEL_TERRAIN, PL_MODEL_VA, PL_MODEL_WAVESHAPE,
	PL_MODEL_PD, PL_MODEL_ADDITIVE, PL_MODEL_CHORDS,
#if PL_SPEECH
	PL_MODEL_SPEECH,	// (0.33k-0.33: a kit with it now plays the model before, chords: modelOf clamps)
#endif
	};
const uint8_t pl_bankCounts[PL_NUM_BANKS] = {sizeof(pl_drums), sizeof(pl_tonal)};
const uint8_t* const pl_banks[PL_NUM_BANKS] = {pl_drums, pl_tonal};
const char* const pl_bankShorts[PL_NUM_BANKS] = {"drm", "ton"};
const char* const pl_bankNames[PL_NUM_BANKS] = {"Drums", "Tonal"};
}

extern "C" {

void plaitsDrum_seedNoise(uint32_t seed) { stmlib::Random::Seed(seed); }

uint8_t plaitsDrum_bankCount(uint8_t bank) { return bank < PL_NUM_BANKS ? pl_bankCounts[bank] : 0; }
uint8_t plaitsDrum_modelOf(uint8_t bank, uint8_t index)
{
	if(bank >= PL_NUM_BANKS || !pl_bankCounts[bank]) return PL_MODEL_NONE;
	return pl_banks[bank][index < pl_bankCounts[bank] ? index : pl_bankCounts[bank] - 1];
}
const char* plaitsDrum_bankShort(uint8_t bank) { return pl_bankShorts[bank < PL_NUM_BANKS ? bank : 0]; }
const char* plaitsDrum_bankName(uint8_t bank) { return pl_bankNames[bank < PL_NUM_BANKS ? bank : 0]; }
const char* plaitsDrum_modelShort(uint8_t model) { return model < PL_NUM_MODELS ? pl_models[model].shortName : "---"; }
const char* plaitsDrum_modelName(uint8_t model) { return model < PL_NUM_MODELS ? pl_models[model].name : "Empty bank"; }
const char* plaitsDrum_knobName(uint8_t model, uint8_t knob)
{
	return model < PL_NUM_MODELS && knob < 3 ? pl_models[model].knobs[knob] : "-";
}
const char* plaitsDrum_twistName(uint8_t model) { return model < PL_NUM_MODELS ? pl_models[model].twist : "-"; }
const char* plaitsDrum_outputName(uint8_t model, uint8_t output)
{
	return model < PL_NUM_MODELS && output < PL_NUM_OUTPUTS ? pl_models[model].outputs[output] : "-";
}
uint8_t plaitsDrum_selfEnveloped(uint8_t model) { return model < PL_NUM_MODELS ? pl_models[model].selfEnveloped : 1; }

float plaitsDrum_noteOf(float hz, float fs)
{
	// plaits::NoteToFrequency(note) = a0 / 4 * 2^((note - 9) / 12), a fraction of the rate; a0 = 55 / 47872.34
	const float f = hz / fs;
	if(f <= 0.f) return -119.f;
	return 9.f + 12.f * log2f(f / (plaits::a0 * 0.25f));
}

void plaitsDrum_init(uint8_t v, float fs)
{
	if(v >= PL_VOICES) return;
	PlVoice& p = pl_voices[v];
	p.fs = fs;
	p.next = PL_MODEL_BASS_DRUM;
	p.output = p.playing = PL_OUT_OUT;
	p.rising = false;
	p.silent = true;
	p.quiet = 0;
	p.hz = 65.41f;
	p.dxBank = 0;
	p.file = 0;
	p.subLevel = p.subShape = p.subOct = p.subFold = 0;
	fmTone_subReset(&p.sub);
	p.dxKey = PL_DXK_PATCH;
	p.gate = false;
	p.tap = 0;
	p.accent = 0.8f;
	p.harmonics = p.timbre = p.morph = 0.5f;
	p.twist = 0.5f;
	p.note = 36.f;
	pl_build(p, p.next);
}

void plaitsDrum_setModel(uint8_t v, uint8_t model)
{
	if(v < PL_VOICES && (model < PL_NUM_MODELS || model == PL_MODEL_NONE)) pl_voices[v].next = model;
}

void plaitsDrum_setKnobs(uint8_t v, float harmonics, float timbre, float morph)
{
	if(v >= PL_VOICES) return;
	pl_voices[v].harmonics = pl_clamp01(harmonics);
	pl_voices[v].timbre = pl_clamp01(timbre);
	pl_voices[v].morph = pl_clamp01(morph);
}

void plaitsDrum_setTwist(uint8_t v, float twist)
{
	if(v < PL_VOICES) pl_voices[v].twist = pl_clamp01(twist);
}

void plaitsDrum_setOutput(uint8_t v, uint8_t output)
{
	if(v < PL_VOICES && output < PL_NUM_OUTPUTS) pl_voices[v].output = output;
}

void plaitsDrum_setPitch(uint8_t v, float hz)
{
	if(v >= PL_VOICES) return;
	pl_voices[v].hz = hz;
	float n = plaitsDrum_noteOf(hz, pl_voices[v].fs);
	pl_voices[v].note = n < -119.f ? -119.f : n > 120.f ? 120.f : n;	// voice.cc's range
}

void plaitsDrum_trigger(uint8_t v, float accent)
{
	if(v >= PL_VOICES) return;
	PlVoice& p = pl_voices[v];
	if(p.next == PL_MODEL_NONE)
	{
		p.silent = true;	// an empty bank: nothing plays
		return;
	}
	if(p.next != p.model)
	{
		pl_build(p, p.next);
		pl_pickUser(p);	// (before priming: the wavetable primes on its waves)
		if(p.engine || p.fm2)
		{
			// prime a newly built engine at this note and knobs, not heard. From rest the wavetable sweeps from its
			// first wave to the knobs' (it smooths them per Render call) and differentiates integrated waves on the
			// way: loud spikes at low notes (on Plaits only at power-on). 120 renders of 4 samples (480 samples) let
			// the per-call and per-sample smoothing settle; once per model change
			plaits::EngineParameters e;
			e.note = p.note; e.harmonics = p.harmonics; e.timbre = p.timbre; e.morph = p.morph; e.accent = accent;
			e.macro = p.twist;
			e.trigger = plaits::TRIGGER_LOW;
			float o[4], a[4];
			bool enveloped = false;
			for(int n = 0; n < 120; n++)
			{
				if(p.fm2)
					p.fm2->Render(e, o, a, 4, true);	// (its frequencies glide from a0 on the first render)
				else
					p.engine->Render(e, o, a, 4, &enveloped);
			}
		}
	}
	else
		pl_pickUser(p);
	fmTone_subReset(&p.sub);	// the sub restarts with each hit, as on fmt
	p.accent = pl_clamp01(accent);
	p.rising = true;
	p.tap = PL_BLOCK / PL_CHUNK;
	p.playing = p.output;
	p.silent = false;
	p.quiet = 0;
}

uint8_t plaitsDrum_dxBankCount(void) { return (uint8_t)(PL_DX_BUILTIN_BANKS + umaSdf_count(UMA_SDF_DX7)); }
void plaitsDrum_setDxBank(uint8_t v, uint8_t bank)
{
	if(v >= PL_VOICES) return;
	pl_voices[v].dxBank = bank < plaitsDrum_dxBankCount() ? bank : 0;
	// a card bank: the main loop loads it (uma_sdfiles.c) while the voice is on the 6-op
	const bool card = pl_voices[v].next == PL_MODEL_DX7 && pl_voices[v].dxBank >= PL_DX_BUILTIN_BANKS;
	umaSdf_setWant(v, card ? UMA_SDF_DX7 : UMA_SDF_NONE, (uint8_t)(pl_voices[v].dxBank - PL_DX_BUILTIN_BANKS));
}
void plaitsDrum_setFile(uint8_t v, uint8_t file)
{
	if(v >= PL_VOICES) return;
	PlVoice& p = pl_voices[v];
	const uint8_t kind = pl_userKind(p.next);
	if(kind == UMA_SDF_NONE)
	{
		plaitsDrum_setDxBank(v, file);	// the 6-op's bank (and nothing wanted on the other models)
		return;
	}
	p.file = file < plaitsDrum_userFileCount(p.next) ? file : 0;	// a kit's file that's gone: built in
	umaSdf_setWant(v, p.file ? kind : UMA_SDF_NONE, (uint8_t)(p.file - 1));
}
uint8_t plaitsDrum_userFileCount(uint8_t model)
{
	const uint8_t kind = pl_userKind(model);
	return kind == UMA_SDF_NONE ? 1 : (uint8_t)(1 + umaSdf_count(kind));
}
uint8_t plaitsDrum_dxPatchOf(float harmonics)
{
	const int q = (int)(pl_clamp01(harmonics) * 1.02f * 32.f);	// SixOpEngine's quantizer without its hysteresis
	return (uint8_t)(q > 31 ? 31 : q);
}
void plaitsDrum_dxPatchName(uint8_t bank, uint8_t patch, char* out)
{
	const uint8_t* bankData = pl_dxBankData(bank);
	if(!bankData)	// a card bank (0.34b)
	{
		const char* why = bank >= plaitsDrum_dxBankCount() ? "no file"
			: umaSdf_status(UMA_SDF_DX7, (uint8_t)(bank - PL_DX_BUILTIN_BANKS)) == UMA_SDF_BAD ? "bad file" : "loading";
		strcpy(out, why);
		return;
	}
	const uint8_t* d = bankData + (patch & 31) * plaits::fm::Patch::SYX_SIZE + 118;	// as Patch::Unpack
	uint8_t n = 10;
	for(uint8_t i = 0; i < 10; i++) { const char c = (char)(d[i] & 0x7f); out[i] = c >= 32 && c < 127 ? c : ' '; }
	while(n && out[n - 1] == ' ') n--;
	out[n] = 0;
}
// "1"-"64": the card's files on the screen
static const char* pl_number(uint8_t n)
{
	static char numbers[UMA_SDF_MAX][3];
	if(n < 1 || n > UMA_SDF_MAX) return "---";
	char* t = numbers[n - 1];
	t[0] = n < 10 ? (char)('0' + n) : (char)('0' + n / 10);
	t[1] = n < 10 ? 0 : (char)('0' + n % 10);
	t[2] = 0;
	return t;
}
const char* plaitsDrum_dxBankShort(uint8_t bank)
{
	static const char* const names[PL_DX_BUILTIN_BANKS] = {"A", "B", "C"};
	if(bank < PL_DX_BUILTIN_BANKS) return names[bank];
	if(bank >= plaitsDrum_dxBankCount()) return "---";	// the kit's card bank is gone
	return pl_number((uint8_t)(bank - PL_DX_BUILTIN_BANKS + 1));
}
const char* plaitsDrum_userFileShort(uint8_t model, uint8_t file)
{
	if(file == 0) return "bit";	// built in
	if(file >= plaitsDrum_userFileCount(model)) return "---";	// the kit's file is gone
	return pl_number(file);
}
const char* plaitsDrum_userFileName(uint8_t model, uint8_t file)
{
	if(file == 0) return "Built-in";
	if(file >= plaitsDrum_userFileCount(model)) return "No file";
	static char name[13];	// "GLASS"
	umaSdf_name(pl_userKind(model), (uint8_t)(file - 1), name);
	return name;
}
const char* plaitsDrum_dxBankName(uint8_t bank)
{
	static const char* const names[PL_DX_BUILTIN_BANKS] = {"Plaits 1", "Plaits 2", "Plaits 3"};
	if(bank < PL_DX_BUILTIN_BANKS) return names[bank];
	if(bank >= plaitsDrum_dxBankCount()) return "No file";
	static char name[13];	// "ROMS/ROM1A"
	umaSdf_name(UMA_SDF_DX7, (uint8_t)(bank - PL_DX_BUILTIN_BANKS), name);
	return name;
}
void plaitsDrum_setGate(uint8_t v, uint8_t held)
{
	if(v < PL_VOICES) pl_voices[v].gate = held != 0;
}

uint8_t plaitsDrum_playsEnveloped(uint8_t v)
{
	if(v >= PL_VOICES) return 1;
	if(pl_voices[v].model == PL_MODEL_DX7)	// the 6-op: its own envelopes only in the patch key mode
		return pl_voices[v].dxKey == PL_DXK_PATCH;
	if(pl_voices[v].model == PL_MODEL_SPEECH)	// speech: words shape their own volume, vowels take the Amp EG
		return pl_voices[v].enveloped;
	return plaitsDrum_selfEnveloped(pl_voices[v].model);
}

void plaitsDrum_setDxKeyMode(uint8_t v, uint8_t mode)
{
	if(v < PL_VOICES && mode < PL_NUM_DXK) pl_voices[v].dxKey = mode;
}
const char* plaitsDrum_dxKeyShort(uint8_t mode)
{
	static const char* const n[PL_NUM_DXK] = {"ptc", "frz"};
	return n[mode < PL_NUM_DXK ? mode : 0];
}
const char* plaitsDrum_dxKeyName(uint8_t mode)
{
	static const char* const n[PL_NUM_DXK] = {"Patch env", "Frozen"};
	return n[mode < PL_NUM_DXK ? mode : 0];
}

uint8_t plaitsDrum_silent(uint8_t v)
{
	return v < PL_VOICES ? pl_voices[v].silent : 1;
}

void plaitsDrum_render(uint8_t v, float* out)
{
	if(v >= PL_VOICES || pl_voices[v].silent || pl_voices[v].model == PL_MODEL_NONE)
	{
		for(int i = 0; i < PL_BLOCK; i++) out[i] = 0.f;	// the silent skip: no hit sounding
		return;
	}
	PlVoice& p = pl_voices[v];
	pl_checkUser(p);
	plaits::EngineParameters e;
	e.note = p.note;
	e.harmonics = p.harmonics;
	e.timbre = p.timbre;
	e.morph = p.morph;
	e.macro = p.twist;
	e.accent = p.accent;
	float o[PL_CHUNK], a[PL_CHUNK];
	if(p.sixOp)
	{
		// the 6-op: two 32-sample steps a block; the key held while the gate is, or for the hit's pulse
		const float note = 69.f + 12.f * log2f((p.hz > 1.f ? p.hz : 1.f) / 440.f);	// a MIDI note at p.fs
		float peak = 0.f;
		for(int c = 0; c < PL_BLOCK; c += 32)
		{
			e.trigger = p.rising ? (plaits::TRIGGER_RISING_EDGE | plaits::TRIGGER_HIGH)
			          : p.gate || p.tap ? plaits::TRIGGER_HIGH : plaits::TRIGGER_LOW;
			p.rising = false;
			p.tap = p.tap > 2 ? (uint8_t)(p.tap - 2) : 0;	// (tap counts 16-sample chunks)
			if(p.dxKey == PL_DXK_FROZEN)
				p.sixOp->RenderFrozen(e, note, p.dxBank, out + c, 32);
			else
				p.sixOp->Render(e, note, p.dxBank, out + c, 32);
		}
		for(int i = 0; i < PL_BLOCK; i++)
		{
			out[i] *= p.outGain;
			const float m = out[i] < 0.f ? -out[i] : out[i];
			if(m > peak) peak = m;
		}
		p.quiet = peak < pl_quietLevel && !p.gate && p.dxKey != PL_DXK_FROZEN ? (uint8_t)(p.quiet + 1) : 0;
		if(p.quiet >= pl_quietBlocks)
			p.silent = true;
		return;
	}
	const bool doOut = p.playing != PL_OUT_AUX, doAux = p.playing != PL_OUT_OUT;
	const bool limit = p.outGain < 0.f;	// Plaits limits out (ChannelPostProcessor), then plays it at 1
	const float outGain = limit ? 1.f : p.outGain;
	const float go = !doOut ? 0.f : doAux ? 0.5f * outGain : outGain;
	const float ga = !doAux ? 0.f : doOut ? 0.5f * p.auxGain : p.auxGain;
	float peak = 0.f;
	for(int c = 0; c < PL_BLOCK; c += PL_CHUNK)
	{
		// as plaits::Voice with the trigger patched: the rising edge in the hit's first render only, high while the
		// gate is held (only the 6-op uses it: the DX7 key)
		e.trigger = p.rising ? (plaits::TRIGGER_RISING_EDGE | plaits::TRIGGER_HIGH)
		          : p.gate || p.tap ? plaits::TRIGGER_HIGH : plaits::TRIGGER_LOW;
		p.rising = false;
		if(p.tap) p.tap--;
		if(p.bassDrum)
			p.bassDrum->Render(e, o, a, PL_CHUNK, doOut, doAux);
		else if(p.fm2)
			p.fm2->Render(e, o, a, PL_CHUNK, doAux);
		else if(p.string)
			p.string->Render(e, o, a, PL_CHUNK);
		else
		{
			bool enveloped = plaitsDrum_selfEnveloped(p.model);
			p.engine->Render(e, o, a, PL_CHUNK, &enveloped);
			p.enveloped = enveloped;	// (the speech engine sets it per render)
		}
		if(limit && doOut)
			p.limiter.Process(-p.outGain, o, PL_CHUNK);
		for(int i = 0; i < PL_CHUNK; i++)
		{
			const float x = (doOut ? go * o[i] : 0.f) + (doAux ? ga * a[i] : 0.f);
			out[c + i] = x;
			const float m = x < 0.f ? -x : x;
			if(m > peak) peak = m;
		}
	}
	if(p.subLevel && (p.model == PL_MODEL_WAVETABLE || p.model == PL_MODEL_TERRAIN))
	{
		// the sub (0.34e): fmt's, after Plaits' output, 1-4 octaves below the played pitch; at 127 about the
		// wavetable's own level (voice.cc plays it at 0.6)
		fmTone_subAdd(&p.sub, out, PL_BLOCK, p.hz / (p.fs * (float)(2u << p.subOct)), p.subLevel, p.subShape, p.subFold, 0.6f);
		for(int i = 0; i < PL_BLOCK; i++) { const float m = out[i] < 0.f ? -out[i] : out[i]; if(m > peak) peak = m; }
	}
	p.quiet = peak < pl_quietLevel ? (uint8_t)(p.quiet + 1) : 0;
	if(p.quiet >= pl_quietBlocks)
		p.silent = true;
}

void plaitsDrum_setSub(uint8_t v, uint8_t level, uint8_t shape, uint8_t octave, uint8_t fold)
{
	if(v >= PL_VOICES) return;
	PlVoice& p = pl_voices[v];
	p.subLevel = level > 127 ? 127 : level;
	p.subShape = shape < FMT_NUM_SUB_SHAPES ? shape : 0;
	p.subOct = octave < FMT_NUM_SUB_OCTS ? octave : 0;
	p.subFold = fold > 127 ? 127 : fold;
}

}
