/*
	cppSxfr, from:

		sxfr https://www.drpetter.se/project_sfxr.html
		Copyright (c) 2007 Tomas Pettersson (MIT License)

	Now under the Apache-2.0 License (due to included PCG32 code):
		https://www.apache.org/licenses/LICENSE-2.0

	Rewritten into an engine only that outputs PCM data.
	Jason A. Petrasko 2021

	Not an exact replica, some changes made:
		* added local PRNG engine (PCG32, thanks slime), but using fast repeatable xorshift* 64 bit rng for internal noise buffers
		* works totally in memory (generate local float buffer)
		* supports writing to and from streams
		* OOPS! (bit off more than I could chew) currently broken sample_rate support, so still locked at 44100 hz
		* added parameter "COMPRESS": compress the output valid range 0.2f to 0.9f
		* added parameter "DECIMATE": decimate (to bit size) valid range 2.0 to 12.0 (target bits)
		* made the float paramter block size 128 bytes, 32 floats. currently using 27, leaving 5 for the future
		* code accepts versions from streams 1.0f to <2.0f, allowing for expansion (maybe using those extra 5 parameters)
		* added wave types: pink noise, triangle, tan, breaker, and one-bit noise from bfxr here: https://github.com/madeso/bfxr
*/

#define _USE_MATH_DEFINES
#include <cmath>
#include <cstring>
#include <exception>
#include <stdexcept>
#include "cppSfxr.h"
#include <time.h>
#include <stdlib.h>
#include <streambuf>
#include <fstream>
#include <vector>
#include <array>

using namespace std;

// *************************************************************************************
// *Really* minimal PCG32 code / (c) 2014 M.E. O'Neill / pcg-random.org
// Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)

typedef struct { uint64_t state;  uint64_t inc; } pcg32_random_t;

inline uint32_t pcg32_random_r(pcg32_random_t* rng)
{
	uint64_t oldstate = rng->state;
	// Advance internal state
	rng->state = oldstate * 6364136223846793005ULL + (rng->inc | 1);
	// Calculate output function (XSH RR), uses old state for max ILP
	uint32_t xorshifted = (uint32_t)(((oldstate >> 18u) ^ oldstate) >> 27u);
	uint32_t rot = oldstate >> 59u;
#pragma warning(suppress : 4146)
	return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

void pcg32_srandom_r(pcg32_random_t* rng, uint64_t initstate, uint64_t initseq)
{
	rng->state = 0U;
	rng->inc = (initseq << 1u) | 1u;
	pcg32_random_r(rng);
	rng->state += initstate;
	pcg32_random_r(rng);
}
// *************************************************************************************

#pragma pack(2)
// *************************************************************************************
// 40(44) byte .WAV header that preceeds a PCM data stream.
typedef struct WaveFileHeader {
	char riff[4] = { 'R', 'I', 'F', 'F' };
	unsigned int size = 0;
	char wave[4] = { 'W', 'A', 'V', 'E' };
	char fmt[4] = { 'f', 'm', 't', ' ' };
	unsigned int chuck_size = 16;
	unsigned short pcm = 1;
	unsigned short mono = 1;
	unsigned int sample_rate = 44100;
	unsigned int byte_rate = 88200;
	unsigned short block_align = 2;
	unsigned short bits = 16;
	char data[4] = { 'd', 'a', 't', 'a' };
	unsigned int pcm_size = 0;
} WaveFileHeader;
// *************************************************************************************

// *************************************************************************************
// 42(46) byte .WAV header that preceeds a PCM-FLOAT data stream, wtf format? Really?
typedef struct WaveFloatFileHeader {
	char riff[4] = { 'R', 'I', 'F', 'F' };
	unsigned int size = 0;
	char wave[4] = { 'W', 'A', 'V', 'E' };
	char fmt[4] = { 'f', 'm', 't', ' ' };
	unsigned int chuck_size = 18;
	unsigned short pcm_float = 3;
	unsigned short mono = 1;
	unsigned int sample_rate = 44100;
	unsigned int byte_rate = 176400;
	unsigned short block_align = 4;
	unsigned short bits = 32;
	unsigned short extension_size = 0;
	char data[4] = { 'd', 'a', 't', 'a' };
	unsigned int pcm_size = 0;
} WaveFloatFileHeader;
// *************************************************************************************
#pragma pack()

#define rnd(n) (pcg32_random_r(&core->rt)%(n+1))
#define rndc(n) (pcg32_random_r(&rt)%(n+1))
#define frnd(range) ((float)rnd(10000) / 10000 * range)
#define frndc(range) ((float)rndc(10000) / 10000 * range)

#define C(x) core->x
#define CP(x) param->x
#define PV(x) paramData.x


// *************************************************************************************
// simple collection of 16kb float buffers for data
class SfxrFloatBuffer {
public:
	vector<array<float, 4096>*> bTable;
	array<float, 4096>* pBlock;
	unsigned int pos;

#ifdef SFXR_STATIC_STREAM_BUFFER
	char staticBuffer[4096 * 4];
#endif

	SfxrFloatBuffer();

	unsigned int size();
	unsigned int sizeBytes();
	unsigned int memoryBytes();
	void getLimitAverage(float* l, float* a);
	void scale(float x);
	void clear();

	void writeStream(ostream& ofx);		// float streams
	void writeStream8(ostream& ofx);	// UINT8 PCM streams
	void writeStream16(ostream& ofx);	// INT16 PCM streams
	void writeStream24(ostream& ofx);	// INT24 PCM streams
	void writeStream32(ostream& ofx);	// INT32 PCM streams

	void operator<<(float f);
	float operator[](unsigned int index);
};

SfxrFloatBuffer::SfxrFloatBuffer()
{
	pos = 0;
	pBlock = new array<float, 4096>;
	bTable.reserve(128);
#ifdef SFXR_STATIC_STREAM_BUFFER
#pragma omp simd
	for (int i = 0; i < 16384; i++)
		staticBuffer[i] = 0;
#endif
}

unsigned int SfxrFloatBuffer::size()
{
	return (unsigned int)bTable.size() * 4096 + pos;
}

unsigned int SfxrFloatBuffer::sizeBytes()
{
	return (unsigned int)(bTable.size() * 4096 + pos) * sizeof(float);
}

unsigned int SfxrFloatBuffer::memoryBytes()
{
	return (unsigned int)((bTable.size()+1) * sizeof(array<float, 4096>));
}

void SfxrFloatBuffer::getLimitAverage(float* l, float* a)
{
	float limit = 0.0f;
	double average = 0.0f;
	double count = 0.0f;
	for (const auto& block : bTable)
	{
		for (const auto& sample : *block)
		{
			float a = fabs(sample);
			average += (double)a;
			if (a > limit) limit = a;
			count += 1.0;
		}
	}
	for (const auto& sample : *pBlock)
	{
		float a = fabs(sample);
		average += (double)a;
		if (a > limit) limit = a;
		count += 1.0;
	}
	*l = limit;
	*a = (float(average / count));
}

void SfxrFloatBuffer::scale(float x)
{
	for (const auto& block : bTable)
	{
		for (int i = 0; i < 4096; i++)
			(*block)[i] *= x;
	}
	for (int i = 0; i < pBlock->size(); i++)
		(*pBlock)[i] *= x;
}

void SfxrFloatBuffer::operator<<(float f)
{
	(*pBlock)[pos++] = f;
	if (pos == 4096)
	{
		pos = 0;
		bTable.push_back(pBlock);
		pBlock = new array<float, 4096>;
	}
}

void SfxrFloatBuffer::writeStream(ostream& ofx)
{
	for (const auto& block : bTable)
	{
		ofx.write((const char*)block->data(), sizeof(float) * 4096);
	}
	ofx.write((const char*)pBlock->data(), sizeof(float) * pos);
}

void SfxrFloatBuffer::writeStream8(ostream& ofx)
{
#ifndef SFXR_STATIC_STREAM_BUFFER
	int8_t* buffer = new int8_t[4096];
#else
	uint8_t* buffer = (uint8_t*)staticBuffer;
#endif
	for (const auto& block : bTable)
	{
#pragma omp simd
		for (int i = 0; i < 4096; i++)
		{
			uint8_t debug = (int8_t)((*block)[i] * (float)0x7F) + 0x7F;
			buffer[i] = debug;
		}
		ofx.write((const char*)buffer, 4096);
	}
	for (int i = 0; i < 4096; i++)
	{
		uint8_t debug = (int8_t)((*pBlock)[i] * (float)0x7F) + 0x7F;
		buffer[i] = debug;
	}
	ofx.write((const char*)buffer, pos);
#ifndef SFXR_STATIC_STREAM_BUFFER
	delete[] buffer;
#endif
}

void SfxrFloatBuffer::writeStream16(ostream& ofx)
{
#ifndef SFXR_STATIC_STREAM_BUFFER
	int16_t* buffer = new int16_t[4096];
#else
	int16_t* buffer = (int16_t*)staticBuffer;
#endif
	for (const auto& block : bTable)
	{
#pragma omp simd
		for (int i = 0; i < 4096; i++)
		{
			buffer[i] = (int16_t)((*block)[i] * (float)0x7FFE);
		}
		ofx.write((const char*)buffer, 4096 * 2);
	}
	for (unsigned int i = 0; i < pos; i++)
		buffer[i] = (int16_t)((*pBlock)[i] * (float)0x7FFE);
	ofx.write((const char*)buffer, (size_t)pos * 2);
#ifndef SFXR_STATIC_STREAM_BUFFER
	delete[] buffer;
#endif
}

void SfxrFloatBuffer::writeStream24(ostream& ofx)
{
#ifndef SFXR_STATIC_STREAM_BUFFER
	int8_t* buffer = new int8_t[4096];
#else
	uint8_t* buffer = (uint8_t*)staticBuffer;
#endif
	unsigned int bpos = 0;
	for (const auto& block : bTable)
	{
		bpos = 0;
#pragma omp simd
		for (int i = 0; i < 4096; i++)
		{
			uint32_t debug = (int32_t)((*block)[i] * (float)0x7FFFFE);
			buffer[bpos++] = debug & 0xFF;
			buffer[bpos++] = (debug & 0xFF00) >> 8;
			buffer[bpos++] = (debug & 0xFF0000) >> 16;
		}
		ofx.write((const char*)buffer, 4096 * 3);
	}
	bpos = 0;
	for (unsigned int i = 0; i < pos; i++)
	{
		uint32_t debug = (int32_t)((*pBlock)[i] * (float)0x7FFFFE);
		buffer[bpos++] = debug & 0xFF;
		buffer[bpos++] = (debug & 0xFF00) >> 8;
		buffer[bpos++] = (debug & 0xFF0000) >> 16;
	}
	ofx.write((const char*)buffer, (size_t)pos * (size_t)3);
#ifndef SFXR_STATIC_STREAM_BUFFER
	delete[] buffer;
#endif
}

void SfxrFloatBuffer::writeStream32(ostream& ofx)
{
#ifndef SFXR_STATIC_STREAM_BUFFER
	int16_t* buffer = new int16_t[4096];
#else
	int32_t* buffer = (int32_t*)staticBuffer;
#endif
	for (const auto& block : bTable)
	{
#pragma omp simd
		for (int i = 0; i < 4096; i++)
		{
			buffer[i] = (int32_t)((*block)[i] * (float)0x7FFFFFFE);
		}
		ofx.write((const char*)buffer, 4096 * 2);
	}
	for (unsigned int i = 0; i < pos; i++)
		buffer[i] = (int32_t)((*pBlock)[i] * (float)0x7FFFFFFE);
	ofx.write((const char*)buffer, (size_t)pos * 2);
#ifndef SFXR_STATIC_STREAM_BUFFER
	delete[] buffer;
#endif
}

void SfxrFloatBuffer::clear()
{
	for (const auto& block : bTable)
		delete block;
	bTable.clear();
	pos = 0;
}

float SfxrFloatBuffer::operator[](unsigned int index)
{
	unsigned int block = index >> 12; // divide by 4096, shift 12 bits
	unsigned int bpos = index % 4096;
	if (bTable.size() == 0)
	{
		if (block > 0) throw new runtime_error("invalid index into SfxrFloatBuffer");
		return (*pBlock)[bpos];
	}
	else
	{
		if (block > bTable.size() + 1) throw new runtime_error("invalid index into SfxrFloatBuffer");
		if (block == bTable.size())
			return (*pBlock)[bpos];
		else
			return (*bTable[block])[bpos];
	}
}
// *************************************************************************************

// *************************************************************************************
// randxs functions, quick xorshift* prng for noise banks
class RandXS
{
public:
	uint64_t a;

	RandXS(uint64_t seed = 0) { a = seed ^ 0xA0110B01C9774200; }

	inline void seed(uint64_t s) { a = s ^ 0xA0110B01C9774200; }

	inline uint64_t rand()
	{
		uint64_t x = a;
		x ^= x >> 12;
		x ^= x << 25;
		x ^= x >> 27;
		a = x;
		return x * 0x2545F4914F6CDD1DULL;
	}

	inline uint32_t rand32()
	{
		uint64_t x = a;
		x ^= x >> 12;
		x ^= x << 25;
		x ^= x >> 27;
		a = x;
		return (x * 0x2545F4914F6CDD1DULL) & 0xFFFFFFFF;
	}

	inline float randf()
	{
		return (float)rand32() / (float)0xFFFFFFFF;
	}

	inline double randd()
	{
		return (double)rand() / (double)0xFFFFFFFFFFFFFFFF;
	}
};

// *************************************************************************************
// pink noise
class PinkNumber
{
private:
	RandXS rxs;
	int max_key;
	int key;
	unsigned int white_values[5];
	unsigned int range;
public:
	PinkNumber(unsigned int range = 65536)
	{
		max_key = 0x1f; // Five bits set
		this->range = range;
		key = 0;
		for (int i = 0; i < 5; i++)
			white_values[i] = rxs.rand32() % (range / 5);
	}

	int getNextValue()
	{
		int last_key = key;
		unsigned int sum;

		key++;
		if (key > max_key)
			key = 0;

		int diff = last_key ^ key;
		sum = 0;
		for (int i = 0; i < 5; i++)
		{
			if (diff & (1 << i))
				white_values[i] = rxs.rand32() % (range / 5);
			sum += white_values[i];
		}
		return sum;
	}

	float getNextFloat()
	{
		return (float)getNextValue() / (float)range;
	}
};

// *************************************************************************************
class SfxrCore
{
public:
	float phase = 0;
	double fperiod = 0.0;
	double fmaxperiod = 0.0;
	double fslide = 0.0;
	double fdslide = 0.0;
	float period = 0;
	float square_duty = 0.0f;
	float square_slide = 0.0f;
	int env_stage = 0;
	float env_time = 0;
	float env_length[3] = { 0, 0, 0 };
	float env_vol = 0.0f;
	float fphase = 0.0f;
	float fdphase = 0.0f;
	float iphase = 0;
	float phaser_buffer[1024];
	float ipp = 0;
	float noise_buffer[32];
	float pink_noise_buffer[32];
	float fltp = 0.0f;
	float fltdp = 0.0f;
	float fltw = 0.0f;
	float fltw_d = 0.0f;
	float fltdmp = 0.0f;
	float fltphp = 0.0f;
	float flthp = 0.0f;
	float flthp_d = 0.0f;
	float vib_phase = 0.0f;
	float vib_speed = 0.0f;
	float vib_amp = 0.0f;
	float rep_time = 0;
	float rep_limit = 0;
	float arp_time = 0;
	float arp_limit = 0;
	double arp_mod = 0.0;

	int one_bit_noisestate = 0;
	double one_bit_noise = 0.0;

	float sound_vol = 0.5f;
	float master_vol = 0.25f;

	int wav_bits = 16;
	const int wav_freq = 44100;
	int out_freq = 44100;
	float ratio = 1.0f;

	bool playing_sample = false;

	PinkNumber pn;

	RandXS rxs;
	pcg32_random_t rt;
	Sfxr* parent = nullptr;
	Sfxr::Parameters* param = nullptr;
	SfxrFloatBuffer* buffer = nullptr;

	SfxrCore();

	void seed(unsigned long long s);
	void seed(const char* s);

	void resetSample(bool restart);
	void synthSample();
};

#define xsrndf(range)  (rxs.randf() * range)

SfxrCore::SfxrCore()
{
	buffer = new SfxrFloatBuffer();
	// initialize with the same seed to always give same outputs in a program
	// wonder what those seeds are in ascii??? :D
	pcg32_srandom_r(&rt, 0x6350502053667872 ^ 0xABABABAB, 0x6D75726167616D69 ^ 0xBABABABA);
	#pragma omp simd
	for (int i = 0; i < 1024; i++)
		phaser_buffer[i] = 0.0f;
	#pragma omp simd
	for (int i = 0; i < 32; i++)
	{
		noise_buffer[i] = 0.0f;
		pink_noise_buffer[i] = 0.0f;
	}
}

void SfxrCore::seed(unsigned long long s)
{
	pcg32_srandom_r(&rt, 0x6350502053667872 ^ s, 0x6D75726167616D69 & s);
}

void SfxrCore::seed(const char* s)
{
	size_t len = strlen(s);
	if (len < 4) throw new std::runtime_error("string of less than 4 chars sent to SfxrCore::seed()");
	uint64_t A, B = 0;
	A = (uint64_t)(s[0]) + ((uint64_t)(s[1]) << 8) + ((uint64_t)(s[2]) << 16) + ((uint64_t)(s[3]) << 24);
	unsigned int i = 4;
	while ((i < len) && (i < 8))
		B += uint64_t(s[i]) << (i - 4);
	if (B == 0) B = 0xBABABABA;
	pcg32_srandom_r(&rt, 0x6350502053667872 ^ A, 0x6D75726167616D69 & B);
}

void SfxrCore::resetSample(bool restart)
{
	if (!restart) phase = 0;
	fperiod = 100.0 / ((double)CP(base_freq) * (double)CP(base_freq) + 0.001);
	period = trunc((float)fperiod);
	fmaxperiod = 100.0 / ((double)CP(freq_limit) * (double)CP(freq_limit) + 0.001);
	fslide = 1.0 - pow((double)CP(freq_ramp), 3.0) * 0.01;
	fdslide = -pow((double)CP(freq_dramp), 3.0) * 0.000001;
	square_duty = 0.5f - CP(duty) * 0.5f;
	square_slide = -CP(duty_ramp) * 0.00005f;
	if (CP(arp_mod) >= 0.0f)
		arp_mod = 1.0 - pow((double)CP(arp_mod), 2.0) * 0.9;
	else
		arp_mod = 1.0 + pow((double)CP(arp_mod), 2.0) * 10.0;
	arp_time = 0;
	arp_limit = trunc(pow(1.0f - CP(arp_speed), 2.0f) * 20000.0f + 32.0f);
	if (CP(arp_speed) == 1.0f)
		arp_limit = 0;
	if (!restart)
	{
		rxs.seed(0); // reset random buffer generation
		one_bit_noisestate = 1 << 14;
		// reset filter
		fltp = 0.0f;
		fltdp = 0.0f;
		fltw = pow(CP(lpf_freq), 3.0f) * 0.1f;
		fltw_d = 1.0f + CP(lpf_ramp) * 0.0001f;
		fltdmp = 5.0f / (1.0f + pow(CP(lpf_resonance), 2.0f) * 20.0f) * (0.01f + fltw);
		if (fltdmp > 0.8f) fltdmp = 0.8f;
		fltphp = 0.0f;
		flthp = pow(CP(hpf_freq), 2.0f) * 0.1f;
		flthp_d = 1.0f + CP(hpf_ramp) * 0.0003f;
		// reset vibrato
		vib_phase = 0.0f;
		vib_speed = pow(CP(vib_speed), 2.0f) * 0.01f;
		vib_amp = CP(vib_strength) * 0.5f;
		// reset envelope
		env_vol = 0.0f;
		env_stage = 0;
		env_time = 0;
		env_length[0] = trunc(CP(env_attack) * CP(env_attack) * 100000.0f);
		env_length[1] = trunc(CP(env_sustain) * CP(env_sustain) * 100000.0f);
		env_length[2] = trunc(CP(env_decay) * CP(env_decay) * 100000.0f);

		fphase = pow(CP(pha_offset), 2.0f) * 1020.0f;
		if (CP(pha_offset) < 0.0f) fphase = -fphase;
		fdphase = pow(CP(pha_ramp), 2.0f) * 1.0f;
		if (CP(pha_ramp) < 0.0f) fdphase = -fdphase;
		iphase = trunc(fabs(fphase));
		ipp = 0;
		for (int i = 0; i < 1024; i++)
			phaser_buffer[i] = 0.0f;

		for (int i = 0; i < 32; i++)
		{
			noise_buffer[i] = xsrndf(2.0f) - 1.0f;
			pink_noise_buffer[i] = pn.getNextFloat() * 2.0f - 1.0f;
		}

		rep_time = 0;
		rep_limit = trunc(pow(1.0f - CP(repeat_speed), 2.0f) * 20000.0f + 32.0f);
		if (CP(repeat_speed) == 0.0f)
			rep_limit = 0;
	}
}

void SfxrCore::synthSample()
{
	int length = 4096;
	float decimate = 0.0f;
	float compress = CP(cs_compress);

	if (CP(cs_decimate) != 0)
	{
		decimate = (float(1 << (int)CP(cs_decimate)));
	}

	#pragma omp simd
	for (int i = 0; i < length; i++)
	{
		if (!playing_sample)
			break;

		rep_time += ratio;
		if (rep_limit != 0.0f && rep_time >= rep_limit)
		{
			rep_time = 0.0f;
			resetSample(true);
		}

		// frequency envelopes/arpeggios
		arp_time += ratio;
		if (arp_limit != 0.0f && arp_time >= arp_limit)
		{
			arp_limit = 0.0f;
			fperiod *= arp_mod;
		}
		fslide += fdslide * ratio;
		fperiod *= fslide;
		if (fperiod > fmaxperiod)
		{
			fperiod = fmaxperiod;
			if (CP(freq_limit) > 0.0f)
				playing_sample = false;
		}
		float rfperiod = (float)fperiod;
		if (vib_amp > 0.0f)
		{
			vib_phase += vib_speed * ratio;
			rfperiod = (float)(fperiod * (1.0 + sin(((double)vib_phase) * (double)vib_amp)));
		}
		period = (float)trunc(rfperiod);
		if (period < 8) period = 8;
		square_duty += square_slide * ratio;
		if (square_duty < 0.0f) square_duty = 0.0f;
		if (square_duty > 0.5f) square_duty = 0.5f;
		// volume envelope
		env_time += ratio;
		if (env_time > env_length[env_stage])
		{
			env_time = 0.0f;
			env_stage++;
			if (env_stage == 3)
				playing_sample = false;
		}
		if (env_stage == 0)
			env_vol = env_time / env_length[0];
		if (env_stage == 1)
			env_vol = 1.0f + pow(1.0f - env_time / env_length[1], 1.0f) * 2.0f * CP(env_punch);
		if (env_stage == 2)
			env_vol = 1.0f - env_time / env_length[2];

		// phaser step
		fphase += fdphase * ratio;
		iphase = trunc(fabs(fphase));
		if (iphase > 1023.0f) iphase = 1023.0f;

		if (flthp_d != 0.0f)
		{
			flthp *= flthp_d * ratio;	// x Ratio?
			if (flthp < 0.00001f) flthp = 0.00001f;
			if (flthp > 0.1f) flthp = 0.1f;
		}

		float ssample = 0.0f;
		int wave_type = (int)CP(wave_type);
		for (int si = 0; si < 8; si++) // 8x supersampling
		{
			float sample = 0.0f;
			phase += ratio;
			if (phase >= period)
			{
				phase = fmod(phase,period);
				if (wave_type == SFXR_WAVE_NOISE)
					for (int i = 0; i < 32; i++)
						noise_buffer[i] = xsrndf(2.0f) - 1.0f;
				else if (wave_type == SFXR_WAVE_PINK)
				{
					for (int i = 0; i < 32; i++)
						pink_noise_buffer[i] = pn.getNextFloat() * 2.0f - 1.0f;
				}
				else if (wave_type == SFXR_WAVE_1BIT)
				{
					const int feedBit = (one_bit_noisestate >> 1 & 1) ^ (one_bit_noisestate & 1);
					one_bit_noisestate = one_bit_noisestate >> 1 | (feedBit << 14);
					one_bit_noise = double(~one_bit_noisestate & 1) - 0.5;
				}
			}
			// base waveform
			float fp = phase / period;
			double amp;
			switch (wave_type)
			{
			case SFXR_WAVE_SQUARE:
				if (fp < square_duty)
					sample = 0.5f;
				else
					sample = -0.5f;
				break;
			case SFXR_WAVE_SAWTOOTH:
				sample = 1.0f - fp * 2.0f;
				break;
			case SFXR_WAVE_SINE:
				sample = (float)sin((double)fp * 2.0 * M_PI);
				break;
			case SFXR_WAVE_NOISE:
				sample = noise_buffer[(int)(phase * 32.0f / period)];
				break;
			case SFXR_WAVE_TRIANGLE:
				sample = fabs(1.0f - (phase / period) * 2.0f) - 1.0f;
				break;
			case SFXR_WAVE_PINK:
				sample = pink_noise_buffer[(int)(phase * 32.0f / period)];
				break;
			case SFXR_WAVE_TAN:
				sample += tan((float)M_PI * phase / period);
				break;
			case SFXR_WAVE_BREAKER:
				amp = phase / period;
				sample += (float)fabs(1.0 - amp * amp * 2.0) - 1.0f;
				break;
			case SFXR_WAVE_1BIT:
				sample += (float)one_bit_noise;
				break;
			}
			// lp filter
			float pp = fltp;
			fltw *= fltw_d * ratio;
			if (fltw < 0.0f) fltw = 0.0f;
			if (fltw > 0.1f) fltw = 0.1f;
			if (CP(lpf_freq) != 1.0f)
			{
				fltdp += (sample - fltp) * fltw;
				fltdp -= fltdp * fltdmp;
			}
			else
			{
				fltp = sample;
				fltdp = 0.0f;
			}
			fltp += fltdp;
			// hp filter
			fltphp += (fltp - pp);
			fltphp -= fltphp * flthp;
			sample = fltphp;
			// phaser
			phaser_buffer[(int)ipp & 1023] = sample;
			sample += phaser_buffer[((int)ipp - (int)iphase + 1024) & 1023];
			ipp = (float)((int)(ipp + ratio) & 1023);
			// final accumulation and envelope application
			ssample += sample * env_vol;
		}
		ssample = ssample / 8.0f;

		// decimate?
		if (decimate != 0)
			ssample = trunc(ssample * decimate) / decimate;

		// compress?
		if (compress != 0)
			ssample = pow(ssample, compress);

		ssample *= master_vol;
		ssample *= 2.0f * sound_vol;

		if (ssample > 1.0f) ssample = 1.0f;
		if (ssample < -1.0f) ssample = -1.0f;
		(*buffer) << ssample;
	}
}
// *************************************************************************************

// *************************************************************************************
// Sfxr class itself!
Sfxr::Sfxr(unsigned int sample_rate, unsigned int bit_depth)
{
	reset();
	core = new SfxrCore();
	core->parent = this;
	core->param = getParameters();
	setPCM(sample_rate, bit_depth);
}

Sfxr::Parameters* Sfxr::getParameters()
{
	return &paramData;
}

void Sfxr::seed(unsigned long long s)
{
	core->seed(s);
}

void Sfxr::seed(const char* s)
{
	core->seed(s);
}

// set operating mode options (see options above, all bit flagged)
void Sfxr::setMode(unsigned int m)
{
	mode = m;
}

// get operating mode options
unsigned int Sfxr::getMode()
{
	return mode;
}

void Sfxr::setFloat()
{
	// since this is the default internal format, do nothing
	format = ExportFormat::FLOAT;
	// but just to be safe, adjust PCM settings
	core->wav_bits = 32;
	// and make sure sampleBytes is the right size
	sampleBytes = 4;
}

void Sfxr::setPCM(unsigned int sample_rate, unsigned int bit_depth)
{
	if (sample_rate != SFXR_SAMPLERATE_INVALID)
	{
#ifndef SFXR_DISALLOW_SAMPLERATE
		core->out_freq = sample_rate;		// new freq
		rebuild = true;
		core->ratio = (float)core->wav_freq / (float)core->out_freq;
#else
		if (sample_rate != 44100) throw new runtime_error("cannot change sample_rate when SFXR_DISALLOW_SAMPLERATE is set in Sfxr::setPCM()");
#endif
	}
	core->wav_bits = bit_depth;		// new bits
	if (bit_depth % 8 > 0) throw new runtime_error("bit depth must align with 8-bit bytes in Sfxr::setPCM()");
	if (bit_depth > 32)
		throw new runtime_error("bit depth must be 8, 16, 24, or 32 only in Sfxr::setPCM()");
	sampleBytes = core->wav_bits / 8;
	switch (sampleBytes)
	{
	case 1: format = ExportFormat::PCM8; break;
	case 2: format = ExportFormat::PCM16; break;
	case 3: format = ExportFormat::PCM24; break;
	case 4: format = ExportFormat::PCM32; break;
	}
}

unsigned int Sfxr::sizeWaveFloatString()
{
	assertSynthed();
	return 36 + (sampleBytes * totalSamples);
}

unsigned int Sfxr::sizeWaveString()
{
	assertSynthed();
	return 36 + (sampleBytes * totalSamples);
}

unsigned int Sfxr::size(ExportFormat f)
{
	unsigned int sampleSize = 1, headerSize = 0;
	switch (f)
	{
	case ExportFormat::WAVE_PCM: sampleSize = core->wav_bits / 8; headerSize = sizeof(WaveFileHeader); break;
	case ExportFormat::WAVE_FLOAT: sampleSize = 4; headerSize = sizeof(WaveFloatFileHeader); break;
	case ExportFormat::PCM8: sampleSize = 1; break;
	case ExportFormat::PCM16: sampleSize = 2; break;
	case ExportFormat::PCM24: sampleSize = 3; break;
	case ExportFormat::PCM32:
	case ExportFormat::FLOAT: sampleSize = 4; break;
	}
	return sampleSize * core->buffer->size() + headerSize;
}

void Sfxr::assertSynthed()
{
	if (!created) create(fromWhat);
	else if (rebuild) create();
}

void Sfxr::setParameters(Parameters& p)
{
	created = rebuild = true;
	paramData = p;
}

void Sfxr::setParameters(Parameters* p)
{
	created = rebuild = true;
	memcpy(&paramData, p, sizeof(Parameters));
}

void Sfxr::reset()
{
	PV(wave_type) = 0;

	PV(base_freq) = 0.3f;
	PV(freq_limit) = 0.0f;
	PV(freq_ramp) = 0.0f;
	PV(freq_dramp) = 0.0f;
	PV(duty) = 0.0f;
	PV(duty_ramp) = 0.0f;

	PV(vib_strength) = 0.0f;
	PV(vib_speed) = 0.0f;
	PV(vib_delay) = 0.0f;

	PV(env_attack) = 0.0f;
	PV(env_sustain) = 0.3f;
	PV(env_decay) = 0.4f;
	PV(env_punch) = 0.0f;

	PV(filter_on) = false;
	PV(lpf_resonance) = 0.0f;
	PV(lpf_freq) = 1.0f;
	PV(lpf_ramp) = 0.0f;
	PV(hpf_freq) = 0.0f;
	PV(hpf_ramp) = 0.0f;

	PV(pha_offset) = 0.0f;
	PV(pha_ramp) = 0.0f;

	PV(repeat_speed) = 0.0f;

	PV(arp_speed) = 0.0f;
	PV(arp_mod) = 0.0f;
}

void Sfxr::mutate()
{
	if (rnd(1)) PV(base_freq) += frnd(0.1f) - 0.05f;
	if (rnd(1)) PV(freq_ramp) += frnd(0.1f) - 0.05f;
	if (rnd(1)) PV(freq_dramp) += frnd(0.1f) - 0.05f;
	if (rnd(1)) PV(duty) += frnd(0.1f) - 0.05f;
	if (rnd(1)) PV(duty_ramp) += frnd(0.1f) - 0.05f;
	if (rnd(1)) PV(vib_strength) += frnd(0.1f) - 0.05f;
	if (rnd(1)) PV(vib_speed) += frnd(0.1f) - 0.05f;
	if (rnd(1)) PV(vib_delay) += frnd(0.1f) - 0.05f;
	if (rnd(1)) PV(env_attack) += frnd(0.1f) - 0.05f;
	if (rnd(1)) PV(env_sustain) += frnd(0.1f) - 0.05f;
	if (rnd(1)) PV(env_decay) += frnd(0.1f) - 0.05f;
	if (rnd(1)) PV(env_punch) += frnd(0.1f) - 0.05f;
	if (rnd(1)) PV(lpf_resonance) += frnd(0.1f) - 0.05f;
	if (rnd(1)) PV(lpf_freq) += frnd(0.1f) - 0.05f;
	if (rnd(1)) PV(lpf_ramp) += frnd(0.1f) - 0.05f;
	if (rnd(1)) PV(hpf_freq) += frnd(0.1f) - 0.05f;
	if (rnd(1)) PV(hpf_ramp) += frnd(0.1f) - 0.05f;
	if (rnd(1)) PV(pha_offset) += frnd(0.1f) - 0.05f;
	if (rnd(1)) PV(pha_ramp) += frnd(0.1f) - 0.05f;
	if (rnd(1)) PV(repeat_speed) += frnd(0.1f) - 0.05f;
	if (rnd(1)) PV(arp_speed) += frnd(0.1f) - 0.05f;
	if (rnd(1)) PV(arp_mod) += frnd(0.1f) - 0.05f;
}

void Sfxr::randomize()
{
	PV(base_freq) = pow(frnd(2.0f) - 1.0f, 2.0f);
	if (rnd(1))
		PV(base_freq) = pow(frnd(2.0f) - 1.0f, 3.0f) + 0.5f;
	PV(freq_limit) = 0.0f;
	PV(freq_ramp) = pow(frnd(2.0f) - 1.0f, 5.0f);
	if (PV(base_freq) > 0.7f && PV(freq_ramp) > 0.2f)
		PV(freq_ramp) = -PV(freq_ramp);
	if (PV(base_freq) < 0.2f && PV(freq_ramp) < -0.05f)
		PV(freq_ramp) = -PV(freq_ramp);
	PV(freq_dramp) = pow(frnd(2.0f) - 1.0f, 3.0f);
	PV(duty) = frnd(2.0f) - 1.0f;
	PV(duty_ramp) = pow(frnd(2.0f) - 1.0f, 3.0f);
	PV(vib_strength) = pow(frnd(2.0f) - 1.0f, 3.0f);
	PV(vib_speed) = frnd(2.0f) - 1.0f;
	PV(vib_delay) = frnd(2.0f) - 1.0f;
	PV(env_attack) = pow(frnd(2.0f) - 1.0f, 3.0f);
	PV(env_sustain) = pow(frnd(2.0f) - 1.0f, 2.0f);
	PV(env_decay) = frnd(2.0f) - 1.0f;
	PV(env_punch) = pow(frnd(0.8f), 2.0f);
	if (PV(env_attack) + PV(env_sustain) + PV(env_decay) < 0.2f)
	{
		PV(env_sustain) += 0.2f + frnd(0.3f);
		PV(env_decay) += 0.2f + frnd(0.3f);
	}
	PV(lpf_resonance) = frnd(2.0f) - 1.0f;
	PV(lpf_freq) = 1.0f - pow(frnd(1.0f), 3.0f);
	PV(lpf_ramp) = pow(frnd(2.0f) - 1.0f, 3.0f);
	if (PV(lpf_freq) < 0.1f && PV(lpf_ramp) < -0.05f)
		PV(lpf_ramp) = -PV(lpf_ramp);
	PV(hpf_freq) = pow(frnd(1.0f), 5.0f);
	PV(hpf_ramp) = pow(frnd(2.0f) - 1.0f, 5.0f);
	PV(pha_offset) = pow(frnd(2.0f) - 1.0f, 3.0f);
	PV(pha_ramp) = pow(frnd(2.0f) - 1.0f, 3.0f);
	PV(repeat_speed) = frnd(2.0f) - 1.0f;
	PV(arp_speed) = frnd(2.0f) - 1.0f;
	PV(arp_mod) = frnd(2.0f) - 1.0f;
}

bool Sfxr::loadStream(istream& ifs)
{
	if (mode & SFXR_WORD_MODE)
	{
		int16_t version, x;
		int16_t wordTable[32];
		ifs >> version;
		if ((version < 100) || (version >= 199)) return false;
		ifs.read((char*)&wordTable, sizeof(wordTable));
		ifs >> x;
		core->sound_vol = (float)(x * 32000);
		// fill in the actual float values
		float* p = (float*)&paramData;
		for (int i = 0; i < 8; i++)
		{
			int index = i * 4;
			p[i] = (float)wordTable[i] * 32000.0f;
			p[i+1] = (float)wordTable[i+1] * 32000.0f;
			p[i+2] = (float)wordTable[i+2] * 32000.0f;
			p[i+3] = (float)wordTable[i+3] * 32000.0f;
		}
		created = true;
		rebuild = true;
		return true;
	}
	else
	{
		float version = 0;
		ifs >> version;
		if ((version < 1.0f) || (version >= 2.0f)) return false;
		ifs.read((char*)&paramData, sizeof(paramData));
		ifs >> core->sound_vol;
		created = true;
		rebuild = true;
		return true;
	}
}

bool Sfxr::loadFile(const char* fname)
{
	ifstream ifs(fname, ios::binary);
	return loadStream(ifs);
}

bool Sfxr::writeStream(ostream& ofs)
{
	if (mode & SFXR_WORD_MODE)
	{
		int16_t version = 100, x;
		int16_t wordTable[32];
		ofs.write((const char*)&version, 2);
		// build the output word table
		float* p = (float*)&paramData;
		for (int i = 0; i < 8; i++)
		{
			int index = i * 4;
			wordTable[i] = (int16_t)(p[i] * 32000.0f);
			wordTable[i+1] = (int16_t)(p[i+1] * 32000.0f);
			wordTable[i+2] = (int16_t)(p[i+2] * 32000.0f);
			wordTable[i+3] = (int16_t)(p[i+3] * 32000.0f);
		}
		ofs.write((const char*)&wordTable, sizeof(wordTable));
		x = (int16_t)(core->sound_vol * 32000.0f);
		ofs.write((const char*)&x, 2);
	}
	else
	{
		ofs << 1.0f;
		ofs.write((const char*)&paramData, sizeof(paramData));
		ofs << core->sound_vol;
		return true;
	}
}

bool Sfxr::writeFile(const char* fname)
{
	ofstream ofs(fname, ios::binary);
	return writeStream(ofs);
}

bool Sfxr::exportBuffer(ExportFormat method, void* pData)
{
	switch (method)
	{
	case ExportFormat::FLOAT:
		return exportFloat((float*)pData);
		break;
	case ExportFormat::WAVE_PCM:
		return exportWaveString((char*)pData);
		break;
	case ExportFormat::WAVE_FLOAT:
		return exportWaveFloatString((char*)pData);
		break;
	case ExportFormat::PCM8:
		setPCM(SFXR_SAMPLERATE_INVALID, 8);
		return exportPCM((char*)pData);
		break;
	case ExportFormat::PCM16:
		setPCM(SFXR_SAMPLERATE_INVALID, 16);
		return exportPCM((char*)pData);
		break;
	case ExportFormat::PCM24:
		setPCM(SFXR_SAMPLERATE_INVALID, 24);
		return exportPCM((char*)pData);
		break;
	case ExportFormat::PCM32:
		setPCM(SFXR_SAMPLERATE_INVALID, 32);
		return exportPCM((char*)pData);
		break;
	default:
		throw new runtime_error("invalid export method passed to Sfxr::exportBuffer()");
	}
	return false;
}

bool Sfxr::exportStream(ExportFormat method, std::ostream& ofs)
{
	switch (method)
	{
	case ExportFormat::FLOAT:
		return exportFloatStream(ofs);
		break;
	case ExportFormat::WAVE_PCM:
		return exportWaveStream(ofs);
		break;
	case ExportFormat::WAVE_FLOAT:
		return exportWaveFloatStream(ofs);
		break;
	case ExportFormat::PCM8:
		setPCM(SFXR_SAMPLERATE_INVALID, 8);
		return exportPCMStream(ofs);
		break;
	case ExportFormat::PCM16:
		setPCM(SFXR_SAMPLERATE_INVALID, 16);
		return exportPCMStream(ofs);
		break;
	case ExportFormat::PCM24:
		setPCM(SFXR_SAMPLERATE_INVALID, 24);
		return exportPCMStream(ofs);
		break;
	case ExportFormat::PCM32:
		setPCM(SFXR_SAMPLERATE_INVALID, 32);
		return exportPCMStream(ofs);
		break;
	default:
		throw new runtime_error("invalid export method passed to Sfxr::exportBuffer()");
	}
	return false;
}

bool Sfxr::exportWaveFloatStream(std::ostream& ofs, bool check_status)
{
	// create sample data, if we need to check
	if (check_status)
		assertSynthed();

	unsigned int sampleTotalBytes = sizeof(float) * totalSamples;

	// create wav header
	WaveFloatFileHeader hdr;
	hdr.size = (unsigned int)(sampleTotalBytes + sizeof(WaveFloatFileHeader) - 8);	// file size
	hdr.sample_rate = (unsigned int)core->out_freq;		// sample rate
	hdr.byte_rate = (unsigned int)(core->out_freq * sizeof(float));
	// bytes/sec
	hdr.block_align = (unsigned short)(sizeof(float));	// block align
	hdr.bits = (unsigned short)32;						// bits per sample
	hdr.pcm_size = (unsigned int)sampleTotalBytes;		// chunk size

	// write it out
	ofs.write((const char*)&hdr, sizeof(WaveFloatFileHeader));

	// now the PCM data
	return exportFloatStream(ofs, false);
}

bool Sfxr::exportWaveStream(std::ostream& ofs, bool check_status)
{
	// create sample data, if we need to check
	if (check_status)
		assertSynthed();

	unsigned int sampleTotalBytes = sampleBytes * totalSamples;

	// create wav header
	WaveFileHeader hdr;
	hdr.size = (unsigned int)(sampleTotalBytes + sizeof(WaveFileHeader) - 8);	// file size
	hdr.sample_rate = (unsigned int)core->out_freq;		// sample rate
	hdr.byte_rate = (unsigned int)(core->out_freq * sampleBytes);
														// bytes/sec
	hdr.block_align = (unsigned short)(sampleBytes);	// block align
	hdr.bits = (unsigned short)core->wav_bits;			// bits per sample
	hdr.pcm_size = (unsigned int)sampleTotalBytes;		// chunk size

	// write it out
	ofs.write((const char*)&hdr, sizeof(WaveFileHeader));

	// now the PCM data
	return exportPCMStream(ofs, false);
}

bool Sfxr::exportFloatStream(std::ostream& ofs, bool check_status)
{
	// create sample data, if we need to check
	if (check_status)
		assertSynthed();

	core->buffer->writeStream(ofs);
	return true;
}

bool Sfxr::exportPCMStream(std::ostream& ofs, bool check_status)
{
	// create sample data, if we need to check
	if (check_status)
		assertSynthed();

	// now the PCM data
	switch (sampleBytes)
	{
	case 1:
		core->buffer->writeStream8(ofs);
		break;
	case 2:
		core->buffer->writeStream16(ofs);
		break;
	case 3:
		core->buffer->writeStream24(ofs);
		break;
	case 4:
		core->buffer->writeStream32(ofs);
		break;
	default:
		throw new runtime_error("bad size for sampleBytes in Sfxr::exportWaveStream()");
	}
	return true;
}

bool Sfxr::exportWaveFile(const char* fname)
{
	ofstream ofs(fname, ios::binary);
	return exportWaveStream(ofs);
}

bool Sfxr::exportWaveFloatFile(const char* fname)
{
	ofstream ofs(fname, ios::binary);
	return exportWaveFloatStream(ofs);
}

struct sxfrInputBuffer : public std::streambuf
{
	sxfrInputBuffer(const char* s, std::size_t n)
	{
		setg((char*)s, (char*)s, (char*)s + n);
	}
};

struct sxfrOutputBuffer : public std::streambuf
{
	sxfrOutputBuffer(const char* s, std::size_t n)
	{
		setp((char*)s, (char*)s, (char*)s + n);
	}
};

bool Sfxr::loadString(const char* data)
{
	if (mode & SFXR_WORD_MODE)
	{
		size_t dataSize = sizeof(data);
		if (dataSize < 34 * sizeof(uint16_t)) return false;
		sxfrInputBuffer osrb(data, dataSize);
		std::istream istr(&osrb);
		return loadStream(istr);
	}
	else
	{
		size_t dataSize = sizeof(data);
		if (dataSize < 2 * sizeof(float) + sizeof(paramData)) return false;
		sxfrInputBuffer osrb(data, dataSize);
		std::istream istr(&osrb);
		return loadStream(istr);
	}
	
}

bool Sfxr::writeString(char* data)
{
	if (mode & SFXR_WORD_MODE)
	{
		sxfrOutputBuffer osrb(data, 34 * sizeof(uint16_t));
		std::ostream ostr(&osrb);
		return writeStream(ostr);
	}
	else
	{
		sxfrOutputBuffer osrb(data, 2 * sizeof(float) + sizeof(paramData));
		std::ostream ostr(&osrb);
		return writeStream(ostr);
	}
}

bool Sfxr::exportWaveFloatString(char* data)
{
	sxfrOutputBuffer osrb(data, sizeWaveString());
	std::ostream ostr(&osrb);
	return exportWaveFloatStream(ostr);
}

bool Sfxr::exportWaveString(char* data)
{
	sxfrOutputBuffer osrb(data, sizeWaveString());
	std::ostream ostr(&osrb);
	return exportWaveStream(ostr);
}

bool Sfxr::exportPCM(char* data)
{
	sxfrOutputBuffer osrb(data, (size_t)sampleBytes * (size_t)totalSamples);
	std::ostream ostr(&osrb);
	return exportPCMStream(ostr);
}

bool Sfxr::exportFloat(float* data)
{
	sxfrOutputBuffer osrb((char*)data, sizeof(float) * totalSamples);
	std::ostream ostr(&osrb);
	return exportFloatStream(ostr);
}

void Sfxr::create(const char* what)
{
	for (int i = 0; i < 7; i++)
	{
		if (!strcmp(what,from[i]))
		{
			create(i);
			return;
		}
	}
	throw new runtime_error("invalid argument sent to Sfxr::create(const char*)");
}

void Sfxr::create(int what)
{
	// a clean slate!
	reset();
	// setup the config!
	switch (what)
	{
	case SFXR_PICKUP_COIN:
		PV(base_freq) = 0.4f + frnd(0.5f);
		PV(env_attack) = 0.0f;
		PV(env_sustain) = frnd(0.1f);
		PV(env_decay) = 0.1f + frnd(0.4f);
		PV(env_punch) = 0.3f + frnd(0.3f);
		if (rnd(1))
		{
			PV(arp_speed) = 0.5f + frnd(0.2f);
			PV(arp_mod) = 0.2f + frnd(0.4f);
		}
		break;
	case SFXR_LASER_SHOOT:
		PV(wave_type) = (float)rnd(2);
		if (PV(wave_type) == 2.0f && rnd(1))
			PV(wave_type) = (float)rnd(1);
		PV(base_freq) = 0.5f + frnd(0.5f);
		PV(freq_limit) = PV(base_freq) - 0.2f - frnd(0.6f);
		if (PV(freq_limit) < 0.2f) PV(freq_limit) = 0.2f;
		PV(freq_ramp) = -0.15f - frnd(0.2f);
		if (rnd(2) == 0)
		{
			PV(base_freq) = 0.3f + frnd(0.6f);
			PV(freq_limit) = frnd(0.1f);
			PV(freq_ramp) = -0.35f - frnd(0.3f);
		}
		if (rnd(1))
		{
			PV(duty) = frnd(0.5f);
			PV(duty_ramp) = frnd(0.2f);
		}
		else
		{
			PV(duty) = 0.4f + frnd(0.5f);
			PV(duty_ramp) = -frnd(0.7f);
		}
		PV(env_attack) = 0.0f;
		PV(env_sustain) = 0.1f + frnd(0.2f);
		PV(env_decay) = frnd(0.4f);
		if (rnd(1))
			PV(env_punch) = frnd(0.3f);
		if (rnd(2) == 0)
		{
			PV(pha_offset) = frnd(0.2f);
			PV(pha_ramp) = -frnd(0.2f);
		}
		if (rnd(1))
			PV(hpf_freq) = frnd(0.3f);
		break;
	case SFXR_EXPLOSION:
		PV(wave_type) = 3;
		if (rnd(1))
		{
			PV(base_freq) = 0.1f + frnd(0.4f);
			PV(freq_ramp) = -0.1f + frnd(0.4f);
		}
		else
		{
			PV(base_freq) = 0.2f + frnd(0.7f);
			PV(freq_ramp) = -0.2f - frnd(0.2f);
		}
		PV(base_freq) *= PV(base_freq);
		if (rnd(4) == 0)
			PV(freq_ramp) = 0.0f;
		if (rnd(2) == 0)
			PV(repeat_speed) = 0.3f + frnd(0.5f);
		PV(env_attack) = 0.0f;
		PV(env_sustain) = 0.1f + frnd(0.3f);
		PV(env_decay) = frnd(0.5f);
		if (rnd(1) == 0)
		{
			PV(pha_offset) = -0.3f + frnd(0.9f);
			PV(pha_ramp) = -frnd(0.3f);
		}
		PV(env_punch) = 0.2f + frnd(0.6f);
		if (rnd(1))
		{
			PV(vib_strength) = frnd(0.7f);
			PV(vib_speed) = frnd(0.6f);
		}
		if (rnd(2) == 0)
		{
			PV(arp_speed) = 0.6f + frnd(0.3f);
			PV(arp_mod) = 0.8f - frnd(1.6f);
		}
		break;
	case SFXR_POWERUP:
		if (rnd(1))
			PV(wave_type) = 1;
		else
			PV(duty) = frnd(0.6f);
		if (rnd(1))
		{
			PV(base_freq) = 0.2f + frnd(0.3f);
			PV(freq_ramp) = 0.1f + frnd(0.4f);
			PV(repeat_speed) = 0.4f + frnd(0.4f);
		}
		else
		{
			PV(base_freq) = 0.2f + frnd(0.3f);
			PV(freq_ramp) = 0.05f + frnd(0.2f);
			if (rnd(1))
			{
				PV(vib_strength) = frnd(0.7f);
				PV(vib_speed) = frnd(0.6f);
			}
		}
		PV(env_attack) = 0.0f;
		PV(env_sustain) = frnd(0.4f);
		PV(env_decay) = 0.1f + frnd(0.4f);
		break;
	case SFXR_HIT_HURT:
		PV(wave_type) = (float)rnd(2);
		if (PV(wave_type) == 2.0f)
			PV(wave_type) = 3.0f;
		if (PV(wave_type) == 0.0f)
			PV(duty) = frnd(0.6f);
		PV(base_freq) = 0.2f + frnd(0.6f);
		PV(freq_ramp) = -0.3f - frnd(0.4f);
		PV(env_attack) = 0.0f;
		PV(env_sustain) = frnd(0.1f);
		PV(env_decay) = 0.1f + frnd(0.2f);
		if (rnd(1))
			PV(hpf_freq) = frnd(0.3f);
		break;
	case SFXR_JUMP:
		PV(wave_type) = 0;
		PV(duty) = frnd(0.6f);
		PV(base_freq) = 0.3f + frnd(0.3f);
		PV(freq_ramp) = 0.1f + frnd(0.2f);
		PV(env_attack) = 0.0f;
		PV(env_sustain) = 0.1f + frnd(0.3f);
		PV(env_decay) = 0.1f + frnd(0.2f);
		if (rnd(1))
			PV(hpf_freq) = frnd(0.3f);
		if (rnd(1))
			PV(lpf_freq) = 1.0f - frnd(0.6f);
		break;
	case SFXR_BLIP_SELECT:
		PV(wave_type) = (float)rnd(1);
		if (PV(wave_type) == 0.0f)
			PV(duty) = (float)frnd(0.6f);
		PV(base_freq) = 0.2f + frnd(0.4f);
		PV(env_attack) = 0.0f;
		PV(env_sustain) = 0.1f + frnd(0.1f);
		PV(env_decay) = frnd(0.2f);
		PV(hpf_freq) = 0.1f;
		break;
	default:
		// if we pass a bad option, just randomize it all
		randomize();
	}
	// actually work the magic!
	create();
}

void Sfxr::create()
{
	totalSamples = 0;
	sampleBytes = core->wav_bits / 8;

	// if we are in word more, lock params to work values
	if (mode & SFXR_WORD_MODE) lockWordParams();
	
	core->resetSample(false);
	core->playing_sample = true;
	core->buffer->clear();
	while (core->playing_sample)
		core->synthSample();

	totalSamples = core->buffer->size();
	created = true;
	rebuild = false;
}

#define GPI(opt) if (!strcmp(pname,SFXRS_ ## opt)) return SFXRI_ ## opt
int Sfxr::getParamIndex(const char* pname)
{
	GPI(WAVE_TYPE);
	GPI(ENV_ATTACK);
	GPI(ENV_SUSTAIN);
	GPI(ENV_PUNCH);
	GPI(ENV_DECAY);
	GPI(BASE_FREQ);
	GPI(FREQ_LIMIT);
	GPI(FREQ_RAMP);
	GPI(FREQ_DRAMP);
	GPI(VIB_STRENGTH);
	GPI(VIB_SPEED);
	GPI(VIB_DELAY);
	GPI(ARP_MOD);
	GPI(ARP_SPEED);
	GPI(DUTY);
	GPI(DUTY_RAMP);
	GPI(REPEAT_SPEED);
	GPI(PHA_OFFSET);
	GPI(PHA_RAMP);
	GPI(FILTER_ON);
	GPI(LPF_FREQ);
	GPI(LPF_RAMP);
	GPI(LPF_RESONANCE);
	GPI(HPF_FREQ);
	GPI(HPF_RAMP);
	GPI(DECIMATE);
	GPI(COMPRESS);
	throw new runtime_error("bad name passed to Sfxr::getParamIndex");
}

float& Sfxr::operator[](unsigned int i)
{
	if (i > 31) throw new runtime_error("invalid index into Sfxr[]");
	float* pos = (float*)&paramData;
	return pos[i];
}

float& Sfxr::operator[](const char *p)
{
	int i = getParamIndex(p);
	if (i > 31) throw new runtime_error("invalid index into Sfxr[]");
	float* pos = (float*)&paramData;
	return pos[i];
}

void Sfxr::getInfo(SoundInfo& info) { getInfo(&info); }
void Sfxr::getInfo(SoundInfo* info)
{
	info->format = this->format;
	info->totalBytes = core->buffer->sizeBytes();
	info->totalSamples = core->buffer->size();
	info->duration = (float)info->totalSamples / (float)core->out_freq;
	info->memoryUsed = core->buffer->memoryBytes();
	info->overhead = (float)info->memoryUsed / (float)info->totalBytes;
	core->buffer->getLimitAverage(&(info->limit), &(info->average));
}
// these are much quicker! and don't return all the extra info
void Sfxr::getInfo(SoundQuickInfo& info) { getInfo(&info);  }
void Sfxr::getInfo(SoundQuickInfo* info)
{
	info->format = this->format;
	info->totalBytes = core->buffer->sizeBytes();
	info->totalSamples = core->buffer->size();
	info->duration = (float)info->totalSamples / (float)core->out_freq;
}

void Sfxr::normalize()
{
	SoundInfo si;

	getInfo(si);
	core->buffer->scale(1.0f / si.limit);
}

void Sfxr::lockWordParams()
{
	float* param = (float*)&paramData;
	int index = 0;
	for (int i = 0; i < 8; i++)
	{
		int index = i * 4;
		param[index] = (float)trunc((int)param[index] * 32000);
		param[index+1] = (float)trunc((int)param[index+1] * 32000);
		param[index+2] = (float)trunc((int)param[index+2] * 32000);
		param[index+3] = (float)trunc((int)param[index+3] * 32000);
	}
}

