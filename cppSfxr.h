#pragma once

/*
	cppSxfr, from:

		sxfr https://www.drpetter.se/project_sfxr.html
		Copyright (c) 2007 Tomas Pettersson (MIT License)

	Rewritten into an engine only that outputs PCM data.
	Jason A. Petrasko (C) 2021

	-> see Sfxr.cpp for change details

	notes:
		* since WAV file format is little endian, output of writeStreams as the same, and assume operation on a little endian arch.
		* you can change the sample rate, but that is a work in progress, it does not create identical sounds at different rates.
*/

// comment options here to configure at compile time, if you are using one instace of Sfxr, or allocating it on the heap, leave these in
#define SFXR_STATIC_STREAM_BUFFER		// use a static 16kb buffer for generating streams (per instance of Sfxr)
#define SFXR_DISALLOW_SAMPLERATE		// don't allow a sample rate change (undef to play around)

#include <iostream>

// what?
#define SFXR_PICKUP_COIN 0
#define SFXR_LASER_SHOOT 1
#define SFXR_EXPLOSION 2
#define SFXR_POWERUP 3
#define SFXR_HIT_HURT 4
#define SFXR_JUMP 5
#define SFXR_BLIP_SELECT 6

// parameter names
#define SFXRS_WAVE_TYPE			"WAVE TYPE"
#define SFXRS_ENV_ATTACK		"ATTACK TIME"
#define SFXRS_ENV_SUSTAIN		"SUSTAIN TIME"
#define SFXRS_ENV_PUNCH			"SUSTAIN PUNCH"
#define SFXRS_ENV_DECAY			"DECAY TIME"
#define SFXRS_BASE_FREQ			"START FREQUENCY"
#define SFXRS_FREQ_LIMIT		"MIN FREQUENCY"
#define SFXRS_FREQ_RAMP			"SLIDE"
#define SFXRS_FREQ_DRAMP		"DELTA SLIDE"
#define SFXRS_VIB_STRENGTH		"VIBRATO DEPTH"
#define SFXRS_VIB_SPEED			"VIBRATO SPEED"
#define SFXRS_VIB_DELAY			"VIBRATO DELAY"
#define SFXRS_ARP_MOD			"CHANGE AMOUNT"
#define SFXRS_ARP_SPEED			"CHANGE SPEED"
#define SFXRS_DUTY				"SQUARE DUTY"
#define SFXRS_DUTY_RAMP			"DUTY SWEEP"
#define SFXRS_REPEAT_SPEED		"REPEAT SPEED"
#define SFXRS_PHA_OFFSET		"PHASER OFFSET"
#define SFXRS_PHA_RAMP			"PHASER SWEEP"
#define SFXRS_FILTER_ON			"FILTER ON"
#define SFXRS_LPF_FREQ			"LP FILTER CUTOFF"
#define SFXRS_LPF_RAMP			"LP FILTER CUTOFF SWEEP"
#define SFXRS_LPF_RESONANCE		"LP FILTER RESONANCE"
#define SFXRS_HPF_FREQ			"HP FILTER CUTOFF"
#define SFXRS_HPF_RAMP			"HP FILTER CUTOFF SWEEP"
#define SFXRS_DECIMATE			"DECIMATE"
#define SFXRS_COMPRESS			"COMPRESS"

// parameter index
#define SFXRI_WAVE_TYPE			0
#define SFXRI_ENV_ATTACK		1
#define SFXRI_ENV_SUSTAIN		2
#define SFXRI_ENV_PUNCH			3
#define SFXRI_ENV_DECAY			4
#define SFXRI_BASE_FREQ			5
#define SFXRI_FREQ_LIMIT		6
#define SFXRI_FREQ_RAMP			7
#define SFXRI_FREQ_DRAMP		8
#define SFXRI_VIB_STRENGTH		9
#define SFXRI_VIB_SPEED			10
#define SFXRI_VIB_DELAY			11
#define SFXRI_ARP_MOD			12
#define SFXRI_ARP_SPEED			13
#define SFXRI_DUTY				14
#define SFXRI_DUTY_RAMP			15
#define SFXRI_REPEAT_SPEED		16
#define SFXRI_PHA_OFFSET		17
#define SFXRI_PHA_RAMP			18
#define SFXRI_FILTER_ON			19
#define SFXRI_LPF_FREQ			20
#define SFXRI_LPF_RAMP			21
#define SFXRI_LPF_RESONANCE		22
#define SFXRI_HPF_FREQ			23
#define SFXRI_HPF_RAMP			24
#define SFXRI_DECIMATE			25
#define SFXRI_COMPRESS			26

#define SFXR_SAMPLERATE_INVALID	0

#define SFXR_WAVE_SQUARE		0
#define SFXR_WAVE_SAWTOOTH		1
#define SFXR_WAVE_SINE			2
#define SFXR_WAVE_NOISE			3
#define SFXR_WAVE_TRIANGLE		4
#define SFXR_WAVE_PINK			5
#define SFXR_WAVE_TAN	 		6
#define SFXR_WAVE_BREAKER		7
#define SFXR_WAVE_1BIT			8

#define SFXR_PLAIN_MODE			0
#define SFXR_NORMALIZE			1	// normalize the output so limit is always 1.0f
#define SFXR_WORD_MODE			2	// use word size params, 16 bit fixed point: -32.000 to 32.000


// hide a lot of the internal stuff to make this nice and clean
class SfxrCore;

class Sfxr {
public:
	// export formats: PCM and Float WAVE format plus raw PCM and Float
	enum class ExportFormat { WAVE_PCM, WAVE_FLOAT, PCM8, PCM16, PCM24, PCM32, FLOAT };
	// parameters that define the sound, 32 floats = 128 bytes
	struct Parameters {
		float wave_type = 0.0f;
		float env_attack = 0.0f;
		float env_sustain = 0.0f;
		float env_punch = 0.0f;
		float env_decay = 0.0f;
		float base_freq = 0.0f;
		float freq_limit = 0.0f;
		float freq_ramp = 0.0f;
		float freq_dramp = 0.0f;
		float vib_strength = 0.0f;
		float vib_speed = 0.0f;
		float vib_delay = 0.0f;
		float arp_mod = 0.0f;
		float arp_speed = 0.0f;
		float duty = 0.0f;
		float duty_ramp = 0.0f;
		float repeat_speed = 0.0f;
		float pha_offset = 0.0f;
		float pha_ramp = 0.0f;
		float filter_on = 0.0f;
		float lpf_freq = 0.0f;
		float lpf_ramp = 0.0f;
		float lpf_resonance = 0.0f;
		float hpf_freq = 0.0f;
		float hpf_ramp = 0.0f;
		float cs_decimate = 0.0f;
		float cs_compress = 0.0f;
		float unused[5] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	};

	struct SoundInfo
	{
		float duration;
		unsigned int totalSamples;
		unsigned int totalBytes;
		unsigned int memoryUsed;	// includes overhead for buffer structures, etc.
		float overhead;				// ratio of memoryUsed to totalBytes
		float limit;				// largest sample
		float average;				// average sample
		ExportFormat format;		// current selected export format (or ExportFormat::FLOAT if none has been set)
	};

	struct SoundQuickInfo
	{
		float duration;
		unsigned int totalSamples;
		unsigned int totalBytes;
		ExportFormat format;		// current selected export format (or ExportFormat::FLOAT if none has been set)
	};

	const char* from[7] = { "PICKUP/COIN", "LASER/SHOOT", "EXPLOSION", "POWERUP", "HIT/HURT", "JUMP", "BLIP/SELECT" };

	Sfxr(unsigned int sample_rate = 44100, unsigned int bit_depth = 16);

	void reset();
	// all of these function use PCG32, so you might want to seed it to make the exact same sounds if that is a use case?
	void mutate(float amt = 1.0f);
	void randomize();
	void create(const char* what);
	void create(int what);
	// seed functions
	void seed(unsigned long long s);
	void seed(const char* s);	// must be 4 bytes at least or even better 8 bytes!
	// synth the sound!
	void create();
	// set Parameters
	void setParameters(Parameters& p);
	void setParameters(Parameters* p);
	// get Parameters
	Parameters* getParameters();
	// these all are methods to read and save the parameter data
	bool loadFile(const char* fname);
	bool writeFile(const char* fname);
	bool loadString(const char* data);
	bool writeString(char* data);
	bool loadStream(std::istream& ifs);
	bool writeStream(std::ostream& ofs);
	unsigned int writeSize();
	// this is the method to get the actual output
	bool exportBuffer(ExportFormat method, void* pData);	// output to a buffer, use the size() call to know how large to make it
	bool exportStream(ExportFormat method, std::ostream& ofs);
															// output to a std stream
	// write .wav files, if you are into that kind of thing
	bool exportWaveFile(const char* fname);
	bool exportWaveFloatFile(const char* fname);

	// get the output total size
	unsigned int size(ExportFormat method);
	// this will someday work right, as of right now changing the sample_rate alters the output quite a bit
	void setPCM(unsigned int sample_rate = 44100, unsigned int bit_depth = 16);
	// using float format
	void setFloat();
	// set operating mode options (see options above, all bit flagged)
	void setMode(unsigned int m);
	// get operating mode options
	unsigned int getMode();
	// these are non-trivial because we scan the output for limit and average samples, FYI
	void getInfo(SoundInfo& info);
	void getInfo(SoundInfo* info);
	// these are much quicker! and don't return all the extra info
	void getInfo(SoundQuickInfo& info);
	void getInfo(SoundQuickInfo* info);
	// get an index of a named parameter
	int getParamIndex(const char* pname);
	// attach data to this sound, make a copy?
	void setData(void* data, unsigned int size, bool copy = true);
	// get the data attached to this sound
	void* getData();
	// get the size of the data attached to this sound
	unsigned int getDataSize();
	// get/set the parameter at an index, like so: wave_type = mySfxr[SFXRI_WAVE_TYPE]; or mySfxr[SFXRI_WAVE_TYPE] = 2;
	float& operator[](unsigned int i);
	// get/set the parameter at an index, like so: wave_type = mySfxr["WAVE TYPE"]; or mySfxr["WAVE_TYPE"] = 2;
	float& operator[](const char* p);

private:
	SfxrCore* core;
	Parameters paramData;
	bool created = false;
	bool rebuild = false;
	int fromWhat = 0;
	unsigned int totalSamples = 0;
	unsigned int sampleBytes = 2;
	ExportFormat format = ExportFormat::FLOAT;
	unsigned int mode = SFXR_PLAIN_MODE;
	unsigned int dataSize = 0;
	char* dataBytes = nullptr;
	bool dataCopied = false;

	void normalize();
	void lockWordParams();
	void assertSynthed();
	unsigned int sizeWaveString();
	unsigned int sizeWaveFloatString();
	bool exportWaveString(char* data);
	bool exportWaveFloatString(char* data);
	bool exportPCM(char* data);
	bool exportFloat(float* data);
	bool exportWaveStream(std::ostream& ofs, bool check_status = true);
	bool exportWaveFloatStream(std::ostream& ofs, bool check_status = true);
	bool exportPCMStream(std::ostream& ofs, bool check_status = true);
	bool exportFloatStream(std::ostream& ofs, bool check_status = true);
};
