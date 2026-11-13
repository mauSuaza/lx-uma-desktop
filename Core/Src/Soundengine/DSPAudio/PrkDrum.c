/*
 * PrkDrum.c - UMA: the prk engine (docs/design/prk-engine.md). Percussion generators after the Perkons Voice's
 * algorithms, each with a variation and two controls; the voice's envelope, filter and drive do the rest.
 *
 * fld cpx wav rbd rsn slp cym ncr mtn tom are adapted from Percolator's voices.c and dsp.h
 * (https://github.com/saustin2010/vst_instruments, originals/percolator):
 *
 *   MIT License, Copyright (c) 2026 Steve A
 *   Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 *   documentation files (the "Software"), to deal in the Software without restriction, including without
 *   limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
 *   Software, and to permit persons to whom the Software is furnished to do so, subject to the following
 *   conditions: The above copyright notice and this permission notice shall be included in all copies or
 *   substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * pls ssn kps, the controls' new mapping to the Perkons Voice's, and the rest: UMA.
 * Copyright (C) 2026 UMA contributors
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "Soundengine/DSPAudio/PrkDrum.h"
#include <math.h>
#include <string.h>

#define SR		44100.0f
#define INV_SR	(1.0f / 44100.0f)
#define PI_F	3.14159265358979f
#define TWO_PI_F	6.28318530717959f

// --- building blocks (Percolator's dsp.h) ---

static inline float clampf(float x, float lo, float hi) { return x < lo ? lo : x > hi ? hi : x; }

// tanh, close enough for saturation
static inline float sat(float x)
{
	if(x > 3.0f) return 1.0f;
	if(x < -3.0f) return -1.0f;
	const float x2 = x * x;
	return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// 2^x for |x| < ~30
static inline float fexp2(float x)
{
	if(x < -30.0f) return 0.0f;
	if(x > 30.0f) x = 30.0f;
	const float fl = floorf(x), f = x - fl;
	union { float f; int32_t i; } u;
	u.i = (int32_t)(fl + 127.0f) << 23;
	return u.f * (1.0f + f * (0.6951786f + f * (0.2261587f + f * 0.0782630f)));
}

// per-sample multiplier that falls 60 dB in t60 seconds
static inline float t60_coef(float t60) { return t60 <= 0.0f ? 0.0f : expf(-6.9077553f / (t60 * SR)); }
static inline float pmul(float tau) { return expf(-1.0f / (tau * SR)); }
static inline float op_coef(float fc) { return 1.0f - expf(-TWO_PI_F * fc * INV_SR); }

#define SIN_N 1024
static float prk_sintab[SIN_N + 1];
static inline float sin_t(float ph)
{
	ph -= floorf(ph);
	const float x = ph * SIN_N;
	int i = (int)x;
	const float f = x - i;
	i &= SIN_N - 1;
	return prk_sintab[i] + (prk_sintab[i + 1] - prk_sintab[i]) * f;
}

// xorshift noise, -1..1
static inline float noise(uint32_t* s)
{
	uint32_t x = *s;
	x ^= x << 13; x ^= x >> 17; x ^= x << 5;
	*s = x;
	return (int32_t)x * (1.0f / 2147483648.0f);
}

// zero-delay-feedback state-variable filter (Zavalishin's TPT form)
typedef struct { float k, a1, a2, a3; } PrkSvfC;
static inline void svf_coef(PrkSvfC* c, float fc, float q)
{
	const float g = tanf(PI_F * clampf(fc, 8.0f, 0.46f * SR) * INV_SR);
	c->k = 1.0f / q;
	c->a1 = 1.0f / (1.0f + g * (g + c->k));
	c->a2 = g * c->a1;
	c->a3 = g * c->a2;
}
static inline float svf_run(PrkSvf* s, const PrkSvfC* c, float x, float* bp, float* hp)
{
	const float v3 = x - s->s2;
	const float v1 = c->a1 * s->s1 + c->a2 * v3;
	const float v2 = s->s2 + c->a2 * s->s1 + c->a3 * v3;
	s->s1 = 2.0f * v1 - s->s1;
	s->s2 = 2.0f * v2 - s->s2;
	if(bp) *bp = v1;
	if(hp) *hp = x - c->k * v1 - v2;
	return v2;
}

// triangle wavefolder: identity inside -1..1, reflected beyond
static inline float fold(float x)
{
	float u = (x + 1.0f) * 0.25f;
	u -= floorf(u);
	return 1.0f - 4.0f * fabsf(u - 0.5f);
}

static inline void rotate(float* zr, float* zi, float turns, float r)
{
	const float c = sin_t(turns + 0.25f), s = sin_t(turns);
	const float a = *zr, b = *zi;
	*zr = r * (c * a - s * b);
	*zi = r * (s * a + c * b);
}

// polyBLEP correction near a jump, t the phase 0..1, dt the phase step
static inline float blep(float t, float dt)
{
	if(t < dt) { t /= dt; return t + t - t * t - 1.f; }
	if(t > 1.f - dt) { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
	return 0.f;
}

// --- tables (Percolator's make_tables), built once ---

#define WT_N 256
static float prk_wt[3][8][WT_N + 1];
#define PCM_N 2048
static float prk_pcm[PCM_N];
static uint8_t prk_tablesBuilt;

static const float F808[6] = {205.3f, 304.4f, 369.6f, 522.7f, 540.0f, 800.0f};
static const float FCYM[6] = {1.0f, 1.4471f, 1.6170f, 1.9265f, 2.5028f, 2.6637f};

static void prk_makeWave(float* w, const float* amp, const float* phs, int nh)
{
	float peak = 1e-9f;
	for(int i = 0; i < WT_N; i++)
	{
		float s = 0, t = (float)i / WT_N;
		for(int h = 1; h <= nh; h++)
			if(amp[h] != 0.0f) s += amp[h] * sinf(TWO_PI_F * (h * t + phs[h]));
		w[i] = s;
		if(fabsf(s) > peak) peak = fabsf(s);
	}
	for(int i = 0; i < WT_N; i++) w[i] /= peak;
	w[WT_N] = w[0];
}

static void prk_buildTables(void)
{
	if(prk_tablesBuilt)
		return;
	for(int i = 0; i <= SIN_N; i++) prk_sintab[i] = sinf(TWO_PI_F * i / SIN_N);
	enum { H = 24 };
	float amp[H + 1], phs[H + 1];
	// table 1, "analog": sine, warm, triangle, tri/saw, saw, square, pulse 25 %, pulse 10 %
	for(int k = 0; k < 8; k++)
	{
		memset(amp, 0, sizeof amp);
		memset(phs, 0, sizeof phs);
		for(int h = 1; h <= H; h++)
		{
			const float sig = h == 1 ? 1.0f : sinf(PI_F * h / (H + 1)) / (PI_F * h / (H + 1));	// Lanczos
			const float tri = (h & 1) ? ((((h - 1) / 2) & 1) ? -1.0f : 1.0f) / (h * (float)h) : 0.0f;
			const float saw = 1.0f / h;
			const float sq = (h & 1) ? 1.0f / h : 0.0f;
			float a = 0;
			switch(k)
			{
			case 0: a = h == 1; break;
			case 1: a = h == 1 ? 1.0f : h == 2 ? 0.25f : h == 3 ? 0.08f : 0.0f; break;
			case 2: a = tri; break;
			case 3: a = 0.5f * tri + 0.5f * saw * (h > 1 ? 0.7f : 1.0f); break;
			case 4: a = saw; break;
			case 5: a = sq; break;
			case 6: a = 2.0f / (PI_F * h) * sinf(PI_F * h * 0.25f); phs[h] = 0.25f; break;
			default: a = 2.0f / (PI_F * h) * sinf(PI_F * h * 0.10f); phs[h] = 0.25f; break;
			}
			amp[h] = a * sig;
		}
		prk_makeWave(prk_wt[0][k], amp, phs, H);
	}
	// table 2, "formant": a resonance that climbs the harmonics
	for(int k = 0; k < 8; k++)
	{
		const float hc = 1.5f + k * 2.2f;
		memset(phs, 0, sizeof phs);
		for(int h = 1; h <= H; h++) amp[h] = expf(-(h - hc) * (h - hc) / (2.0f * 1.6f * 1.6f)) + (h == 1 ? 0.6f : 0.0f);
		prk_makeWave(prk_wt[1][k], amp, phs, H);
	}
	// table 3, "digital": sparse harmonic sets with scattered phases
	for(int k = 0; k < 8; k++)
	{
		memset(amp, 0, sizeof amp);
		memset(phs, 0, sizeof phs);
		const int hs[4] = {1, 3 + k, 6 + 2 * k, 11 + k};
		const float as[4] = {1.0f, 0.8f, 0.6f, 0.45f};
		for(int j = 0; j < 4; j++)
		{
			amp[hs[j]] += as[j];
			phs[hs[j]] = 0.13f * j * (k + 1);
		}
		prk_makeWave(prk_wt[2][k], amp, phs, H);
	}
	uint32_t r = 0x9e3779b9u;
	for(int i = 0; i < PCM_N; i++) prk_pcm[i] = noise(&r);
	prk_tablesBuilt = 1;
}

// --- names ---

static const char* const prk_short[PRK_NUM_ALGS] =
	{"fld", "cpx", "wav", "pls", "rbd", "rsn", "ssn", "slp", "kps", "cym", "ncr", "mtn", "tom"};
static const char* const prk_names[PRK_NUM_ALGS] =
	{"Fold Drum", "Complex Drum", "Wave Drum", "Pulse Drum", "Res Bass Drum", "Res Snare", "Tone Snare", "Clap",
	 "String", "Cymbal", "Noise Crush", "Metal Noise", "Tom"};
static const uint8_t prk_vars[PRK_NUM_ALGS] = {3, 3, 3, 3, 3, 3, 1, 3, 3, 2, 3, 1, 3};
static const char* const prk_varNames[PRK_NUM_ALGS][3] =
{
	{"Clean", "Noise Hit", "Pulse Hit"},
	{"Sine Mod", "Tri Mod", "Square Mod"},
	{"Analog", "Formant", "Digital"},
	{"Pulse", "Square", "Saw"},
	{"Soft", "Punch", "Hard"},
	{"Classic", "Tight", "Metallic"},
	{"-", "-", "-"},
	{"3 Claps", "4 Claps", "5 Claps"},
	{"Noise", "Click", "Pulse"},
	{"Cymbal", "Bell", "-"},
	{"White", "Pulse Stack", "Crush"},
	{"-", "-", "-"},
	{"Flat", "Drop", "Deep Drop"},
};
static const char* const prk_p1[PRK_NUM_ALGS] =
	{"Pitch Env", "Pitch Env", "Pitch Env", "Pitch Env", "Noise", "Tone", "Noise/Tone", "Room", "Noise Tone",
	 "Room", "Room", "Size", "FM"};
static const char* const prk_p2[PRK_NUM_ALGS] =
	{"Fold", "FM Amount", "Wave Pos", "Env Decay", "Tone", "Noise Filter", "Body Tune", "Filter", "Noise Decay",
	 "Brightness", "Crush", "Brightness", "Tone"};

uint8_t prkDrum_varCount(uint8_t alg) { return alg < PRK_NUM_ALGS ? prk_vars[alg] : 1; }
const char* prkDrum_algShort(uint8_t alg) { return prk_short[alg < PRK_NUM_ALGS ? alg : 0]; }
const char* prkDrum_algName(uint8_t alg) { return prk_names[alg < PRK_NUM_ALGS ? alg : 0]; }
const char* prkDrum_varName(uint8_t alg, uint8_t var)
{
	return prk_varNames[alg < PRK_NUM_ALGS ? alg : 0][var < 3 ? var : 0];
}
const char* prkDrum_p1Name(uint8_t alg) { return prk_p1[alg < PRK_NUM_ALGS ? alg : 0]; }
const char* prkDrum_p2Name(uint8_t alg) { return prk_p2[alg < PRK_NUM_ALGS ? alg : 0]; }

float prkDrum_decayTime(uint8_t value)
{
	return 0.02f * powf(500.f, (value > 127 ? 127 : value) / 127.f);	// 20 ms .. 10 s
}

// --- the engine ---

void prkDrum_init(PrkDrum* d)
{
	prk_buildTables();
	memset(d, 0, sizeof(*d));
	d->params[PRK_ALG] = PRK_FLD;
	d->params[PRK_VAR] = 0;
	d->params[PRK_P1] = 50;
	d->params[PRK_P2] = 40;
	d->params[PRK_TUNE] = 45;	// A2, 110 Hz
	d->params[PRK_DECAY] = 50;
	d->params[PRK_LEVEL] = 100;
	d->pitch = 110.f;
	d->ringTime = prkDrum_decayTime(50);
	d->ampEnvOn = 1;
	d->envIn = d->envLast = 1.f;
	d->rng = 0x1234567u;
}

void prkDrum_setParam(PrkDrum* d, uint8_t param, uint8_t value)
{
	if(param >= PRK_NUM_PARAMS) return;
	if(param == PRK_ALG && value >= PRK_NUM_ALGS) value = PRK_NUM_ALGS - 1;
	if(value > 127) value = 127;
	d->params[param] = value;
	if(param == PRK_TUNE) d->pitch = 440.f * powf(2.f, (value - 69) / 12.f);
	if(param == PRK_DECAY && d->ampEnvOn) d->ringTime = prkDrum_decayTime(value);
}

uint8_t prkDrum_getParam(const PrkDrum* d, uint8_t param) { return param < PRK_NUM_PARAMS ? d->params[param] : 0; }
void prkDrum_setAmpEnv(PrkDrum* d, uint8_t on) { d->ampEnvOn = on ? 1 : 0; }
void prkDrum_setPitch(PrkDrum* d, float hz) { d->pitch = hz; }
void prkDrum_setRingTime(PrkDrum* d, float seconds) { d->ringTime = seconds > 0.005f ? seconds : 0.005f; }
void prkDrum_setEnv(PrkDrum* d, float env) { d->envIn = env < 0.f ? 0.f : env > 1.f ? 1.f : env; }

static uint8_t prk_var(const PrkDrum* d)
{
	const uint8_t n = prkDrum_varCount(d->params[PRK_ALG]);
	return d->params[PRK_VAR] < n ? d->params[PRK_VAR] : (uint8_t)(n - 1);
}

static uint8_t prk_usesRoom(uint8_t alg) { return alg == PRK_SLP || alg == PRK_CYM || alg == PRK_NCR; }

static const int CLAPS[3] = {3, 4, 5};
static const int CLAP_GAP[3] = {397, 529, 706};	// 9, 12, 16 ms
static const int COMB_L[4] = {347, 389, 431, 467}, AP_L[2] = {131, 211};
static const int COMB_AT[4] = {0, 347, 736, 1167}, AP_AT[2] = {1634, 1765};	// offsets in buf.room

void prkDrum_trigger(PrkDrum* d, float velocity)
{
	const uint8_t alg = d->params[PRK_ALG], var = prk_var(d);
	const float k2 = d->params[PRK_P2] / 127.f;
	d->level = (d->params[PRK_LEVEL] / 127.f) * velocity;
	d->env = 1.f;
	d->ns = 0;
	d->penv = 1.f;
	d->tr = 1.f;
	d->nenv = 1.f;
	d->bt = 0.f;
	d->bursts = 0;
	d->hold = d->holdc = 0.f;
	d->ph[0] = d->ph[1] = 0.f;
	d->zr[0] = d->zi[0] = d->zr[1] = d->zi[1] = 0.f;
	if(alg == PRK_RBD)
	{
		static const float ER[3] = {0.9f, 0.35f, 0.2f}, EI[3] = {0.0f, 0.85f, 0.95f};
		d->zr[0] = ER[var];
		d->zi[0] = EI[var];
	}
	else if(alg == PRK_RSN)
	{
		d->zr[0] = 0.3f; d->zi[0] = 0.7f;
		d->zr[1] = 0.25f; d->zi[1] = 0.5f;
	}
	else if(alg == PRK_TOM)
	{
		d->zr[0] = 0.85f * (1.0f - k2);
		d->zi[0] = 0.25f + 0.75f * k2;
		d->tr = k2;
	}
	else if(alg == PRK_KPS)
	{
		// the string starts silent; the excitation is fed in over p2's time (rendered)
		if(d->roomUsed) { memset(d->buf.kps, 0, sizeof(d->buf.kps)); d->roomUsed = 0; }
		d->kexc = 1.f + k2 * k2 * 0.04f * SR;	// 1 sample .. 40 ms
	}
	if(prk_usesRoom(alg) && !d->roomUsed)
	{
		memset(d->buf.room, 0, sizeof(d->buf.room));
		memset(d->ci, 0, sizeof(d->ci));
		memset(d->ai, 0, sizeof(d->ai));
		memset(d->damp, 0, sizeof(d->damp));
		d->roomUsed = 1;
	}
}

// Percolator's room: four damped combs into two allpasses, added to the dry sound
static void prk_room(PrkDrum* d, float* o, float amount, uint32_t n)
{
	float* b = d->buf.room;
	for(uint32_t i = 0; i < n; i++)
	{
		const float in = o[i] * amount * 0.3f;
		float acc = 0;
		for(int j = 0; j < 4; j++)
		{
			float* c = b + COMB_AT[j];
			const float y = c[d->ci[j]];
			d->damp[j] += 0.35f * (y - d->damp[j]);
			c[d->ci[j]] = in + d->damp[j] * 0.86f;
			if(++d->ci[j] >= COMB_L[j]) d->ci[j] = 0;
			acc += y;
		}
		for(int j = 0; j < 2; j++)
		{
			float* a = b + AP_AT[j];
			const float wd = a[d->ai[j]], w = acc + 0.6f * wd;
			acc = wd - 0.6f * w;
			a[d->ai[j]] = w;
			if(++d->ai[j] >= AP_L[j]) d->ai[j] = 0;
		}
		o[i] += acc * amount;
	}
}

// One block. et[]: the envelope as the sound's character sees it (fold depth, FM index, clap tail);
// ea[]: what multiplies the output (the own envelope, or 1 when the voice's envelope does it).
void prkDrum_render(PrkDrum* d, float* out, uint32_t numSamples)
{
	while(numSamples)
	{
		const uint32_t n = numSamples < PRK_BLOCK_SIZE ? numSamples : PRK_BLOCK_SIZE;
		const uint8_t alg = d->params[PRK_ALG], var = prk_var(d);
		const float k1 = d->params[PRK_P1] / 127.f, k2 = d->params[PRK_P2] / 127.f;
		const float f0 = clampf(d->pitch, 8.f, 0.45f * SR);
		const float rt = clampf(f0 / PRK_REF_HZ, 0.1f, 8.f);
		const float ring = t60_coef(d->ringTime * (alg == PRK_RBD ? 2.0f : 1.5f));
		float et[PRK_BLOCK_SIZE], ea[PRK_BLOCK_SIZE];
		float* o = out;

		if(d->ampEnvOn)
		{
			const float mul = t60_coef(prkDrum_decayTime(d->params[PRK_DECAY]));
			const int hold = alg == PRK_SLP ? (CLAPS[var] - 1) * CLAP_GAP[var] : 0;
			for(uint32_t i = 0; i < n; i++)
			{
				if((int)(d->ns + i) >= hold) d->env *= mul;
				et[i] = ea[i] = d->env;
			}
		}
		else
		{
			const float step = (d->envIn - d->envLast) / (float)n;
			float e = d->envLast;
			for(uint32_t i = 0; i < n; i++) { e += step; et[i] = e; ea[i] = 1.f; }
			d->envLast = d->envIn;
		}

		switch(alg)
		{
		default:
		case PRK_FLD:	// p1 pitch env, p2 fold; var: the transient
		{
			const float oct = 6.0f * powf(k1, 1.3f), pm = pmul(0.028f), g = 1.0f + 7.0f * powf(k2, 1.5f);
			const float trn = pmul(0.003f), trp = pmul(0.001f);
			for(uint32_t i = 0; i < n; i++)
			{
				const float f = f0 * fexp2(oct * d->penv);
				d->penv *= pm;
				d->ph[0] += f * INV_SR;
				d->ph[0] -= (int)d->ph[0];
				float y = fold(sin_t(d->ph[0]) * g * (0.35f + 0.65f * et[i])) * ea[i];
				if(d->tr > 1e-4f)
				{
					if(var == 1)
					{
						const float nz = noise(&d->rng);
						d->hp1 += 0.25f * (nz - d->hp1);
						y += (nz - d->hp1) * d->tr * 0.7f;
						d->tr *= trn;
					}
					else if(var == 2)
					{
						const uint32_t t = d->ns + i;
						if(t < 40) y += (t < 20 ? 0.9f : -0.9f) * d->tr;
						d->tr *= trp;
					}
					else d->tr = 0;
				}
				o[i] = y * 0.8f;
			}
			break;
		}
		case PRK_CPX:	// p1 pitch env, p2 FM amount and the modulator's ratio; var: its waveform
		{
			const float oct = 4.0f * powf(k1, 1.3f), pm = pmul(0.018f);
			const float ratio = 0.5f + 3.5f * k2, index = 5.0f * powf(k2, 1.2f) / TWO_PI_F;
			for(uint32_t i = 0; i < n; i++)
			{
				const float f = f0 * fexp2(oct * d->penv);
				d->penv *= pm;
				d->ph[0] += f * INV_SR;
				d->ph[0] -= (int)d->ph[0];
				d->ph[1] += f * ratio * INV_SR;
				d->ph[1] -= (int)d->ph[1];
				const float m = var == 0 ? sin_t(d->ph[1]) : var == 1 ? 1.0f - 4.0f * fabsf(d->ph[1] - 0.5f)
				                                                       : sat(4.0f * sin_t(d->ph[1]));
				o[i] = sin_t(d->ph[0] + index * (0.25f + 0.75f * et[i]) * m) * ea[i] * 0.8f;
			}
			break;
		}
		case PRK_WAV:	// p1 pitch env, p2 wavetable position; var: the table
		{
			const float oct = 6.0f * powf(k1, 1.3f), pm = pmul(0.028f);
			const float pos = k2 * 6.999f;
			const int w0 = (int)pos;
			const float wf = pos - w0;
			const float *A = prk_wt[var][w0], *B = prk_wt[var][w0 + 1];
			for(uint32_t i = 0; i < n; i++)
			{
				const float f = f0 * fexp2(oct * d->penv);
				d->penv *= pm;
				d->ph[0] += f * INV_SR;
				d->ph[0] -= (int)d->ph[0];
				const float x = d->ph[0] * WT_N;
				const int j = (int)x;
				const float fr = x - j;
				const float a = A[j] + (A[j + 1] - A[j]) * fr, b = B[j] + (B[j + 1] - B[j]) * fr;
				o[i] = (a + (b - a) * wf) * ea[i] * 0.75f;
			}
			break;
		}
		case PRK_PLS:	// UMA: a pulse, square or saw with a pitch drop; p1 its depth, p2 its time
		{
			const float oct = 6.0f * powf(k1, 1.3f), pm = pmul(0.003f * powf(50.f, k2));
			for(uint32_t i = 0; i < n; i++)
			{
				const float f = f0 * fexp2(oct * d->penv);
				d->penv *= pm;
				const float dt = clampf(f * INV_SR, 0.f, 0.45f);
				d->ph[0] += dt;
				d->ph[0] -= (int)d->ph[0];
				const float t = d->ph[0];
				float y;
				if(var == 2)
					y = 2.f * t - 1.f - blep(t, dt);
				else
				{
					const float width = var == 0 ? 0.25f : 0.5f;
					float u = t + (1.f - width);
					u -= (int)u;
					y = (t < width ? 1.f : -1.f) + blep(t, dt) - blep(u, dt);
					if(var == 0) y += 0.5f;	// a pulse's DC
				}
				o[i] = y * ea[i] * 0.6f;
			}
			break;
		}
		case PRK_RBD:	// p1 noise (a burst heard over the kick, and exciting it), p2 tone (the pitch drop);
		{				// var: the start of the ring
			const float oct = 5.0f * powf(k2, 1.3f), pm = pmul(0.004f + 0.02f * k2);
			const float nm = pmul(0.003f + 0.025f * k1);		// the burst: 3 ms .. 28 ms
			const float level = 0.9f * k1, excite = 0.02f + 0.2f * k1 * k1, hpc = op_coef(700.0f);
			for(uint32_t i = 0; i < n; i++)
			{
				const float f = f0 * fexp2(oct * d->penv);
				d->penv *= pm;
				const float nzv = noise(&d->rng);
				d->hp1 += hpc * (nzv - d->hp1);
				const float burst = (nzv - d->hp1) * d->nenv;
				d->nenv *= nm;
				d->zr[0] += burst * excite;
				rotate(&d->zr[0], &d->zi[0], f * INV_SR, ring);
				float y = d->zi[0];
				if(var == 2) y = sat(y * 2.5f) * 0.7f;
				o[i] = (y * 0.85f + burst * level) * ea[i];
			}
			break;
		}
		case PRK_RSN:	// p1 tone (body to noise), p2 noise filter; var: the body
		{
			PrkSvfC nc;
			svf_coef(&nc, 1200.0f * powf(12.0f, k2), 0.7f);
			const float nm = t60_coef(clampf(d->ringTime * 0.6f, 0.04f, 1.2f)), pm = pmul(0.012f), hpc = op_coef(300.0f);
			const float r2 = var == 2 ? 2.31f : 1.58f;
			const float bodyG = 1.3f - k1, noiseG = (var == 1 ? 1.0f : 0.65f) * (0.3f + 1.0f * k1);
			for(uint32_t i = 0; i < n; i++)
			{
				const float f = f0 * fexp2(0.35f * d->penv);
				d->penv *= pm;
				rotate(&d->zr[0], &d->zi[0], f * INV_SR, ring);
				float body;
				if(var == 1) body = d->zi[0] * 0.6f;
				else
				{
					rotate(&d->zr[1], &d->zi[1], f * r2 * INV_SR, ring);
					body = var == 0 ? 0.45f * d->zi[0] + 0.3f * d->zi[1] : 0.25f * d->zi[0] + 1.1f * d->zi[0] * d->zi[1];
				}
				float nzv = svf_run(&d->f1, &nc, noise(&d->rng), 0, 0);
				d->hp1 += hpc * (nzv - d->hp1);
				nzv -= d->hp1;
				o[i] = body * bodyG * ea[i] + nzv * d->nenv * noiseG * ea[i];
				d->nenv *= nm;
			}
			break;
		}
		case PRK_SSN:	// UMA: a snare body and band noise, p1 between them. The body sits 1-3 octaves above the drum's
		{				// pitch (p2) with its own short decay, a thump at the hit; the noise rattles as long as the
						// sound (the Amp EG's decay). Revised 2026-11-08: the body at the drum's own pitch and as
						// long as the sound made a bass note under the noise
			PrkSvfC bc;
			svf_coef(&bc, 4500.0f, 0.8f);
			const float ratio = fexp2(1.f + 2.f * k2), pm = pmul(0.012f), hpc = op_coef(400.0f);
			const float tm = t60_coef(0.12f);	// the body: -60 dB in 120 ms
			const float nm = t60_coef(clampf(d->ringTime, 0.03f, 4.0f));
			for(uint32_t i = 0; i < n; i++)
			{
				const float f = f0 * ratio * fexp2(0.5f * d->penv);
				d->penv *= pm;
				d->ph[0] += f * INV_SR;
				d->ph[0] -= (int)d->ph[0];
				const float body = sin_t(d->ph[0]) * d->tr;
				d->tr *= tm;
				float b;
				svf_run(&d->f1, &bc, noise(&d->rng), &b, 0);
				d->hp1 += hpc * (b - d->hp1);
				const float nzv = (b - d->hp1) * d->nenv;
				d->nenv *= nm;
				o[i] = ((1.f - k1) * body * 0.9f + k1 * nzv * 1.6f) * ea[i];
			}
			break;
		}
		case PRK_SLP:	// p1 room, p2 filter (resonance); var: the claps
		{
			const int nb = CLAPS[var], gap = CLAP_GAP[var];
			PrkSvfC bc;
			svf_coef(&bc, clampf(1200.0f * rt, 300.f, 9000.f), 0.6f + 5.0f * k2);
			const float bm = pmul(0.0026f);
			for(uint32_t i = 0; i < n; i++)
			{
				const int t = (int)(d->ns + i);
				if(d->bursts < nb && t >= d->bursts * gap + (d->bursts ? (int)(d->rng & 63) : 0))
				{
					d->bt = 1.0f;
					d->bursts++;
				}
				const float a = d->bursts < nb ? d->bt : ea[i];
				d->bt *= bm;
				float b;
				svf_run(&d->f1, &bc, noise(&d->rng), &b, 0);
				o[i] = b * a * 1.6f;
			}
			break;
		}
		case PRK_KPS:	// UMA: Karplus-Strong. p1 the excitation's tone, p2 its length; var: noise, click, pulse
		{
			const float len = clampf(SR / f0 - 0.5f, 2.f, PRK_KPS_LEN - 2.f);	// the averaging adds half a sample
			const float loss = powf(ring, len);	// the loop's loss per pass: rings as long as the resonators
			const float lpc = 0.05f + 0.95f * k1 * k1;	// the excitation's tone
			float* b = d->buf.kps;
			for(uint32_t i = 0; i < n; i++)
			{
				float ex = 0.f;
				if(d->kexc > 0.f)
				{
					const uint32_t t = d->ns + i;
					const float x = var == 0 ? noise(&d->rng) : var == 1 ? (t < 2 ? 1.f : 0.f)
					                                          : ((t % (uint32_t)(len > 4 ? len : 4)) < len * 0.5f ? 0.7f : -0.7f);
					d->lp1 += lpc * (x - d->lp1);
					ex = d->lp1;
					d->kexc -= 1.f;
				}
				// read len samples back (linear interpolation), average two (the string's damping)
				float rp = (float)d->kw - len;
				if(rp < 0.f) rp += PRK_KPS_LEN;
				const int r0 = (int)rp;
				const float fr = rp - r0;
				const int r1 = (r0 + 1) % PRK_KPS_LEN;
				const float y = b[r0] + (b[r1] - b[r0]) * fr;
				const float s = 0.5f * (y + d->kprev) * loss;
				d->kprev = y;
				b[d->kw] = s + ex;
				if(++d->kw >= PRK_KPS_LEN) d->kw = 0;
				o[i] = (y + ex) * ea[i] * 0.9f;
			}
			break;
		}
		case PRK_CYM:	// p1 room, p2 brightness; var: cymbal, bell
		{
			PrkSvfC a, bf;
			const float br = 0.5f * powf(4.f, k2);
			if(var == 0)
			{
				svf_coef(&a, 5000.0f * sqrtf(rt) * br, 0.9f);
				svf_coef(&bf, 3000.0f * sqrtf(rt) * br, 0.7f);
				float inc[6];
				for(int j = 0; j < 6; j++) inc[j] = 320.0f * FCYM[j] * rt * INV_SR;
				for(uint32_t i = 0; i < n; i++)
				{
					float s[6];
					for(int j = 0; j < 6; j++)
					{
						d->ph[j] += inc[j];
						d->ph[j] -= (int)d->ph[j];
						s[j] = d->ph[j] < 0.5f ? 1.0f : -1.0f;
					}
					const float x = (s[0] * s[1] + s[2] * s[3] + s[4] * s[5]) * 0.33f;
					float bp, h;
					svf_run(&d->f1, &a, x, &bp, 0);
					svf_run(&d->f2, &bf, bp, 0, &h);
					o[i] = h * ea[i] * 1.6f;
				}
			}
			else
			{
				svf_coef(&a, 400.0f, 0.7f);
				const float base = 380.0f * rt * INV_SR;
				for(uint32_t i = 0; i < n; i++)
				{
					d->ph[0] += base;
					d->ph[1] += base * 1.4f;
					d->ph[2] += base * 3.17f;
					for(int j = 0; j < 3; j++) d->ph[j] -= (int)d->ph[j];
					const float m2 = sin_t(d->ph[2]);
					const float m1 = sin_t(d->ph[1] + 0.19f * m2);
					const float y = sin_t(d->ph[0] + (0.1f + 0.3f * et[i]) * br * m1);
					float h;
					svf_run(&d->f1, &a, y, 0, &h);
					o[i] = h * ea[i] * 0.7f;
				}
			}
			break;
		}
		case PRK_NCR:	// p1 room, p2 crush; var: white, pulse stack, crush
		{
			PrkSvfC a, bf;
			const float levels = fexp2(9.f - 7.f * k2);	// 512 .. 4 steps
			if(var == 0)
			{
				svf_coef(&a, 4000.0f * rt, 0.7f);
				for(uint32_t i = 0; i < n; i++)
				{
					float h;
					svf_run(&d->f1, &a, noise(&d->rng), 0, &h);
					if(k2 > 0.02f) h = roundf(h * levels) / levels;
					o[i] = h * ea[i] * 0.75f;
				}
			}
			else if(var == 1)
			{
				svf_coef(&a, 7500.0f * sqrtf(rt), 1.2f);
				svf_coef(&bf, 6000.0f * sqrtf(rt), 0.7f);
				float inc[6];
				for(int j = 0; j < 6; j++) inc[j] = F808[j] * 1.7f * rt * INV_SR;
				for(uint32_t i = 0; i < n; i++)
				{
					float s = 0;
					for(int j = 0; j < 6; j++)
					{
						d->ph[j] += inc[j];
						d->ph[j] -= (int)d->ph[j];
						s += d->ph[j] < 0.5f ? 1.0f : -1.0f;
					}
					float bp, h;
					svf_run(&d->f1, &a, s * 0.3f, &bp, 0);
					svf_run(&d->f2, &bf, bp, 0, &h);
					if(k2 > 0.02f) h = roundf(h * levels) / levels;
					o[i] = h * ea[i] * 2.2f;
				}
			}
			else
			{
				const float rate = clampf(3000.0f * rt * INV_SR, 0.f, 1.f);
				const float crush = fexp2(6.f - 4.f * k2);	// 64 .. 4 steps
				svf_coef(&a, 1500.0f, 0.7f);
				for(uint32_t i = 0; i < n; i++)
				{
					d->holdc += rate;
					if(d->holdc >= 1.0f)
					{
						d->holdc -= (int)d->holdc;
						d->hold = roundf(noise(&d->rng) * crush) / crush;
					}
					float h;
					svf_run(&d->f1, &a, d->hold, 0, &h);
					o[i] = h * ea[i] * 0.65f;
				}
			}
			break;
		}
		case PRK_MTN:	// p1 size of the noise loop, p2 brightness
		{
			const int len = 16 + (int)(k1 * k1 * (PCM_N - 16));
			PrkSvfC a;
			svf_coef(&a, 100.0f * powf(30.f, k2), 0.7f);
			const float step = 2.0f * rt;
			for(uint32_t i = 0; i < n; i++)
			{
				d->ph[0] += step;
				if(d->ph[0] >= len) d->ph[0] -= len * (int)(d->ph[0] / len);
				float h;
				svf_run(&d->f1, &a, prk_pcm[(int)d->ph[0]], 0, &h);
				o[i] = h * ea[i] * 0.55f;
			}
			break;
		}
		case PRK_TOM:	// p1 FM (the ring modulates its own pitch), p2 tone (the strike); var: the pitch drop
		{
			static const float OCT[3] = {0.25f, 0.8f, 2.0f};
			const float oct = OCT[var], pm = pmul(0.045f), fm = 0.6f * k1, trm = pmul(0.005f);
			const float hpc = op_coef(800.0f);
			for(uint32_t i = 0; i < n; i++)
			{
				const float f = f0 * fexp2(oct * d->penv) * (1.0f + fm * d->zi[0]);
				d->penv *= pm;
				rotate(&d->zr[0], &d->zi[0], f * INV_SR, ring);
				float y = d->zi[0] * ea[i];
				if(d->tr > 1e-4f)
				{
					const float nz = noise(&d->rng);
					d->hp1 += hpc * (nz - d->hp1);
					y += (nz - d->hp1) * d->tr * 0.4f;
					d->tr *= trm;
				}
				o[i] = y * 0.85f;
			}
			break;
		}
		}

		if(prk_usesRoom(alg) && d->roomUsed && k1 > 0.005f)
			prk_room(d, o, k1, n);
		if(d->ampEnvOn)
			for(uint32_t i = 0; i < n; i++) o[i] *= d->level;
		d->ns += n;
		out += n;
		numSamples -= n;
	}
}

// --- randomise ---

static uint32_t prk_hash(uint32_t x)
{
	x ^= x >> 16;
	x *= 0x7feb352du;
	x ^= x >> 15;
	x *= 0x846ca68bu;
	x ^= x >> 16;
	return x;
}
static uint32_t prk_rngNext(uint32_t* r)
{
	*r = prk_hash(*r + 0x9e3779b9u);
	return *r;
}
static uint8_t prk_rngRange(uint32_t* r, uint8_t lo, uint8_t hi)
{
	return (uint8_t)(lo + prk_rngNext(r) % (uint32_t)(hi - lo + 1));
}
// centre-weighted (average of two draws)
static uint8_t prk_rngMid(uint32_t* r, uint8_t lo, uint8_t hi)
{
	return (uint8_t)(((uint32_t)prk_rngRange(r, lo, hi) + prk_rngRange(r, lo, hi) + 1) / 2);
}
// mostly low: the lower of two draws
// the attack: 75 % instant, else short
static uint8_t prk_rngChance8(uint32_t* r)
{
	return prk_rngNext(r) % 100u < 75 ? 0 : (uint8_t)(1 + prk_rngNext(r) % 20u);
}
static uint8_t prk_rngLow(uint32_t* r, uint8_t lo, uint8_t hi)
{
	const uint8_t a = prk_rngRange(r, lo, hi), b = prk_rngRange(r, lo, hi);
	return a < b ? a : b;
}

static const uint8_t prk_palPrc[] = {PRK_RSN, PRK_SLP, PRK_CYM, PRK_NCR, PRK_MTN};
static const uint8_t prk_palDrm[] = {PRK_FLD, PRK_CPX, PRK_WAV, PRK_PLS, PRK_RBD, PRK_TOM};

uint8_t prkDrum_paletteHas(uint8_t palette, uint8_t alg)
{
	const uint8_t* list = palette == PRK_PAL_PRC ? prk_palPrc : palette == PRK_PAL_DRM ? prk_palDrm : 0;
	const uint8_t n = palette == PRK_PAL_PRC ? sizeof(prk_palPrc) : palette == PRK_PAL_DRM ? sizeof(prk_palDrm) : 0;
	if(!list) return alg < PRK_NUM_ALGS;
	for(uint8_t i = 0; i < n; i++)
		if(list[i] == alg) return 1;
	return 0;
}

uint8_t prkDrum_random(uint8_t group, uint8_t seed, uint8_t value, uint8_t drum, uint8_t palette,
	PrkRandomValue out[PRK_RND_MAX_VALUES])
{
	// a stream of its own (not the FM drum's for the same numbers)
	uint32_t r = prk_hash(0x51ed270bu ^ ((uint32_t)seed | ((uint32_t)value << 8) | ((uint32_t)drum << 16) | ((uint32_t)group << 24)));
	uint8_t n = 0;
	switch(group)
	{
	case 0:	// tone: algorithm from the palette, its variation, p1 and p2
	{
		uint8_t alg;
		if(palette == PRK_PAL_PRC) alg = prk_palPrc[prk_rngNext(&r) % sizeof(prk_palPrc)];
		else if(palette == PRK_PAL_DRM) alg = prk_palDrm[prk_rngNext(&r) % sizeof(prk_palDrm)];
		else alg = (uint8_t)(prk_rngNext(&r) % PRK_NUM_ALGS);
		out[n++] = (PrkRandomValue){PRK_RV_ALG, alg};
		out[n++] = (PrkRandomValue){PRK_RV_VAR, (uint8_t)(prk_rngNext(&r) % prkDrum_varCount(alg))};
		// the room (slp cym ncr p1) mostly small; the crush (ncr p2) mostly light; the rest anywhere, centred
		const uint8_t room = alg == PRK_SLP || alg == PRK_CYM || alg == PRK_NCR;
		out[n++] = (PrkRandomValue){PRK_RV_P1, room ? prk_rngLow(&r, 0, 100) : prk_rngMid(&r, 0, 127)};
		out[n++] = (PrkRandomValue){PRK_RV_P2, alg == PRK_NCR ? prk_rngLow(&r, 0, 110) : prk_rngMid(&r, 0, 127)};
		break;
	}
	case 1:	// the Amp EG: mostly instant attacks, short to mid decays (percussion), any slope
		out[n++] = (PrkRandomValue){PRK_RV_ATK, prk_rngChance8(&r)};
		out[n++] = (PrkRandomValue){PRK_RV_DEC, prk_rngMid(&r, 30, 100)};
		out[n++] = (PrkRandomValue){PRK_RV_SLOPE, prk_rngRange(&r, 20, 110)};
		break;
	case 2:	// the pitch sweep: often none, else small, sometimes deep
	{
		const uint8_t a1 = prk_rngRange(&r, 10, 100), a2 = prk_rngRange(&r, 10, 100);
		out[n++] = (PrkRandomValue){PRK_RV_SWEEP_AMT, prk_rngNext(&r) % 100u < 35 ? 0 : (a1 < a2 ? a1 : a2)};
		out[n++] = (PrkRandomValue){PRK_RV_SWEEP_DEC, prk_rngRange(&r, 15, 80)};
		out[n++] = (PrkRandomValue){PRK_RV_SWEEP_SLOPE, prk_rngRange(&r, 20, 100)};
		break;
	}
	default:	// fine: nothing on prk
		break;
	}
	return n;
}

