
#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#define DLLAPI __declspec(dllexport)
#else
#define DLLAPI
#endif

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

#define SFXR_FORMAT_WAVE_PCM    0
#define SFXR_FORMAT_WAVE_FLOAT  1
#define SFXR_FORMAT_PCM8        2
#define SFXR_FORMAT_PCM16       3
#define SFXR_FORMAT_PCM24       4
#define SFXR_FORMAT_PCM32       5
#define SFXR_FORMAT_FLOAT       6

struct csParameters {
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

struct csSoundInfo
{
  float duration;
  unsigned int totalSamples;
  unsigned int totalBytes;
  unsigned int memoryUsed;	// includes overhead for buffer structures, etc.
  float overhead;				// ratio of memoryUsed to totalBytes
  float limit;				// largest sample
  float average;				// average sample
  unsigned int format;		// current selected export format (or ExportFormat::FLOAT if none has been set)
};

struct csSoundQuickInfo
{
  float duration;
  unsigned int totalSamples;
  unsigned int totalBytes;
  unsigned int format;		// current selected export format (or ExportFormat::FLOAT if none has been set)
};

struct csSfxr {
  void* (*new)();
  void (*delete)(void *p);
  void (*seed_uint)(void* p, unsigned long long s);
	void (*seed_str)(void* p, const char* s);	// must be 4 bytes at least or even better 8 bytes!
  void (*reset)(void *p);
  void (*mutate)(void *p);
  void (*randomize)(void *p);
  void (*create_str)(void *p, const char* what);
  void (*create_int)(void *p, int what);
  void (*create)(void *p);
  void (*set_parameters)(void *p, csParameters* p);
  csParameters* (*get_parameters)(void *p);
  // these all are methods to read and save the parameter data
  bool (*load_file)(void *p, const char* fname);
  bool (*write_file)(void *p, const char* fname);
  bool (*load_string)(void *p, const char* data);
  bool (*write_string)(void *p, char* data);
  // this is the method to get the actual output
  bool (*export_buffer)(void *p, unsigned int method, void* pData);	// output to a buffer, use the size)() call to know how large to make it
  // write .wav files, if you are into that kind of thing
  bool (*export_wavefile)(void *p, const char* fname);
  bool (*export_wavefloatfile)(void *p, const char* fname);
  // get the output total size
  unsigned int (*size)(void *p, unsigned int method);
  // this will someday work right, as of right now changing the sample_rate alters the output quite a bit
  void (*set_PCM)(void *p, unsigned int sample_rate, unsigned int bit_depth);
  // using float format
  void (*set_float)(void *p);
  // this is non-trivial because we scan the output for limit and average samples, FYI
  void (*get_info)(void *p, csSoundInfo* info);
  // this is much quicker! and don't return all the extra info
  void (*get_infoq)(void *p, csSoundQuickInfo* info);
  // get an index of a named parameter
  int (*get_paramindex)(void *p, const char* pname);
  float (*get_param)(void *p, int index);
  void (*set_param)(void *p, int index, float f);
};

DLLAPI void cs_get(csSfxr* p);
DLLAPI void* cs_new();
DLLAPI void cs_delete(void *p);
DLLAPI void cs_seed_uint(void* p, unsigned long long s);
DLLAPI void cs_seed_str(void* p, const char* s);	// must be 4 bytes at least or even better 8 bytes!
DLLAPI void cs_reset(void *p);
DLLAPI void cs_mutate(void *p);
DLLAPI void cs_randomize(void *p);
DLLAPI void cs_create_str(void *p, const char* what);
DLLAPI void cs_create_int(void *p, int what);
DLLAPI void cs_create(void *p);
DLLAPI void cs_set_parameters(void *p, csParameters* p);
DLLAPI csParameters* cs_get_parameters(void *p);
// these all are methods to read and save the parameter data
DLLAPI bool cs_load_file(void *p, const char* fname);
DLLAPI bool cs_write_file(void *p, const char* fname);
DLLAPI bool cs_load_string(void *p, const char* data);
DLLAPI bool cs_write_string(void *p, char* data);
// this is the method to get the actual output
DLLAPI bool cs_export_buffer(void *p, unsigned int method, void* pData);	// output to a buffer, use the size() call to know how large to make it
// write .wav files, if you are into that kind of thing
DLLAPI bool cs_export_wavefile(void *p, const char* fname);
DLLAPI bool cs_export_wavefloatfile(void *p, const char* fname);
// get the output total size
DLLAPI unsigned int cs_size(void *p, unsigned int method);
// this will someday work right, as of right now changing the sample_rate alters the output quite a bit
DLLAPI void cs_set_PCM(void *p, unsigned int sample_rate, unsigned int bit_depth);
// using float format
DLLAPI void cs_set_float(void *p);
// this is non-trivial because we scan the output for limit and average samples, FYI
DLLAPI void cs_get_info(void *p, csSoundInfo* info);
// this is much quicker! and don't return all the extra info
DLLAPI void cs_get_infoq(void *p, csSoundQuickInfo* info);
// get an index of a named parameter
DLLAPI int cs_get_paramindex(void *p, const char* pname);
DLLAPI float cs_get_param(void *p, int index);
DLLAPI void cs_set_param(void *p, int index, float f);

#ifdef __cplusplus
}
#endif
