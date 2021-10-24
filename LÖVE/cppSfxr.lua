local ffi = require 'ffi'

ffi.cdef[[
typedef struct _csParameters {
  float wave_type;
  float env_attack;
  float env_sustain;
  float env_punch;
  float env_decay;
  float base_freq;
  float freq_limit;
  float freq_ramp;
  float freq_dramp;
  float vib_strength;
  float vib_speed;
  float vib_delay;
  float arp_mod;
  float arp_speed;
  float duty;
  float duty_ramp;
  float repeat_speed;
  float pha_offset;
  float pha_ramp;
  float filter_on;
  float lpf_freq;
  float lpf_ramp;
  float lpf_resonance;
  float hpf_freq;
  float hpf_ramp;
  float cs_decimate;
  float cs_compress;
  float unused[5];
} csParameters;

typedef struct _csSoundInfo
{
  float duration;
  unsigned int totalSamples;
  unsigned int totalBytes;
  unsigned int memoryUsed;
  float overhead;
  float limit;
  float average;
  unsigned int format;
} csSoundInfo;

typedef struct _csSoundQuickInfo
{
  float duration;
  unsigned int totalSamples;
  unsigned int totalBytes;
  unsigned int format;
} csSoundQuickInfo;

typedef struct _csSfxr {
  void* (*_new)();
  void (*_delete)(void *p);
  void (*seed_uint)(void* p, unsigned long long s);
	void (*seed_str)(void* p, const char* s);
  void (*reset)(void *p);
  void (*mutate)(void *p);
  void (*randomize)(void *p);
  void (*create_str)(void *p, const char* what);
  void (*create_int)(void *p, int what);
  void (*create)(void *p);
  void (*set_parameters)(void *p, csParameters* p);
  csParameters* (*get_parameters)(void *p);
  bool (*load_file)(void *p, const char* fname);
  bool (*write_file)(void *p, const char* fname);
  bool (*load_string)(void *p, const char* data);
  bool (*write_string)(void *p, char* data);
  bool (*export_buffer)(void *p, unsigned int method, void* pData);
  bool (*export_wavefile)(void *p, const char* fname);
  bool (*export_wavefloatfile)(void *p, const char* fname);
  unsigned int (*size)(void *p, unsigned int method);
  void (*set_PCM)(void *p, unsigned int sample_rate, unsigned int bit_depth);
  void (*set_float)(void *p);
  void (*get_info)(void *p, csSoundInfo* info);
  void (*get_infoq)(void *p, csSoundQuickInfo* info);
  int (*get_paramindex)(void *p, const char* pname);
  float (*get_param)(void *p, int index);
  void (*set_param)(void *p, int index, float f);
} csSfxr;

void cs_get(struct _csSfxr* p);
]]

local os = love.system.getOS()
local lib

if os == "Windows" or os == "OS X" or os == "Linux" then
  lib = ffi.load("x64cppSfxr")
else
  error("OS is unsupported, cppSfxr.lua")
end

local pSfxr = ffi.new("struct _csSfxr")
lib.cs_get(pSfxr)

local Sfxr = {}

Sfxr.ExportFormat = {
  WAVE_PCM = 0, WAVE_FLOAT = 1,
  PCM8 = 2, PCM16 = 3, PCM24 = 4, PCM32 = 5, FLOAT = 6 }

Sfxr.SAMPLERATE_INVALID = 0

Sfxr.SFXRI = {
  WAVE_TYPE = 0, ENV_ATTACK = 1, ENV_SUSTAIN = 2, ENV_PUNCH = 3, ENV_DECAY = 4,
  BASE_FREQ = 5, FREQ_LIMIT = 6, FREQ_RAMP = 7, FREQ_DRAMP = 8, VIB_STRENGTH = 9,
  VIB_SPEED = 10, VIB_DELAY = 11, ARP_MOD = 12, ARP_SPEED = 13, DUTY = 14,
  DUTY_RAMP = 15, REPEAT_SPEED = 16, PHA_OFFSET = 17, PHA_RAMP = 18, FILTER_ON = 19,
  LPF_FREQ = 20, LPF_RAMP = 21, LPF_RESONANCE = 22, HPF_FREQ = 23, HPF_RAMP = 24,
  DECIMATE = 25, COMPRESS = 26
}

Sfxr.Sound = {
  PICKUP_COIN = 0, LASER_SHOOT = 1, EXPLOSION = 2, POWERUP = 3, HIT_HURT = 4,
  JUMP = 5, BLIP_SELECT = 6
}

Sfxr.qi = ffi.new("struct _csSoundQuickInfo")
Sfxr.fi = ffi.new("struct _csSoundInfo")
Sfxr.pm = ffi.new("struct _csParameters")
Sfxr.p = nil

function Sfxr:assertp()
  if self.p == nil then error ("pointer not initialized, did you forget :init(), Sfxr::assertp()") end
end

function Sfxr:seed(i)
  self:assertp()
  if type(i) == 'number' then
    pSfxr.seed_uint(self.p,i)
  elseif type(i) == 'string' then
    pSfxr.seed_str(self.p,i)
  end
end

function Sfxr:create(i)
  self:assertp()
  if type(i) == 'number' then
    pSfxr.create_int(self.p,i)
  elseif type(i) == 'string' then
    pSfxr.create_str(self.p,i)
  end
end

function Sfxr:setParams(t)
  self:assertp()
  Sfxr.pm.wave_type = t.wave_type
  Sfxr.pm.env_attack = t.env_attack
  Sfxr.pm.env_sustain = t.env_sustain
  Sfxr.pm.env_punch = t.env_punch
  Sfxr.pm.env_decay = t.env_decay
  Sfxr.pm.base_freq = t.base_freq
  Sfxr.pm.freq_limit = t.freq_limit
  Sfxr.pm.freq_ramp = t.freq_ramp
  Sfxr.pm.freq_dramp = t.freq_dramp
  Sfxr.pm.vib_strength = t.vib_strength
  Sfxr.pm.vib_speed = t.vib_speed
  Sfxr.pm.vib_delay = t.vib_delay
  Sfxr.pm.arp_mod = t.arp_mod
  Sfxr.pm.arp_speed = t.arp_speed
  Sfxr.pm.duty = t.duty
  Sfxr.pm.duty_ramp = t.duty_ramp
  Sfxr.pm.repeat_speed = t.repeat_speed
  Sfxr.pm.pha_offset = t.pha_offset
  Sfxr.pm.pha_ramp = t.pha_ramp
  Sfxr.pm.filter_on = t.filter_on
  Sfxr.pm.lpf_freq = t.lpf_freq
  Sfxr.pm.lpf_ramp = t.lpf_ramp
  Sfxr.pm.lpf_resonance = t.lpf_resonance
  Sfxr.pm.hpf_freq = t.hpf_freq
  Sfxr.pm.hpf_ramp = t.hpf_ramp
  Sfxr.pm.cs_decimate = t.cs_decimate
  Sfxr.pm.cs_compress = t.cs_compress
  pSfxr.set_parameters(self.p,Sfxr.pm)
end

function Sfxr:setPackedParams(t)
  self:assertp()
  Sfxr.pm.wave_type = t[1]
  Sfxr.pm.env_attack = t[2]
  Sfxr.pm.env_sustain = t[3]
  Sfxr.pm.env_punch = t[4]
  Sfxr.pm.env_decay = t[5]
  Sfxr.pm.base_freq = t[6]
  Sfxr.pm.freq_limit = t[7]
  Sfxr.pm.freq_ramp = t[8]
  Sfxr.pm.freq_dramp = t[9]
  Sfxr.pm.vib_strength = t[10]
  Sfxr.pm.vib_speed = t[11]
  Sfxr.pm.vib_delay = t[12]
  Sfxr.pm.arp_mod = t[13]
  Sfxr.pm.arp_speed = t[14]
  Sfxr.pm.duty = t[15]
  Sfxr.pm.duty_ramp = t[16]
  Sfxr.pm.repeat_speed = t[17]
  Sfxr.pm.pha_offset = t[18]
  Sfxr.pm.pha_ramp = t[19]
  Sfxr.pm.filter_on = t[20]
  Sfxr.pm.lpf_freq = t[21]
  Sfxr.pm.lpf_ramp = t[22]
  Sfxr.pm.lpf_resonance = t[23]
  Sfxr.pm.hpf_freq = t[24]
  Sfxr.pm.hpf_ramp = t[25]
  Sfxr.pm.cs_decimate = t[26]
  Sfxr.pm.cs_compress = t[27]
  pSfxr.set_parameters(self.p,Sfxr.pm)
end

function Sfxr:setParamString(str)
  self:assertp()
  ffi.copy(Sfxr.pm,str)
  pSfxr.set_parameters(self.p,Sfxr.pm)
end

function Sfxr:getParams()
  self:assertp()
  ffi.copy(Sfxr.pm,pSfxr.get_parameters(self.p),ffi.sizeof("struct _csParameters"))
  local ret = {}
  ret.wave_type = Sfxr.pm.wave_type
  ret.env_attack = Sfxr.pm.env_attack
  ret.env_sustain = Sfxr.pm.env_sustain
  ret.env_punch = Sfxr.pm.env_punch
  ret.env_decay = Sfxr.pm.env_decay
  ret.base_freq = Sfxr.pm.base_freq
  ret.freq_limit = Sfxr.pm.freq_limit
  ret.freq_ramp = Sfxr.pm.freq_ramp
  ret.freq_dramp = Sfxr.pm.freq_dramp
  ret.vib_strength = Sfxr.pm.vib_strength
  ret.vib_speed = Sfxr.pm.vib_speed
  ret.vib_delay = Sfxr.pm.vib_delay
  ret.arp_mod = Sfxr.pm.arp_mod
  ret.arp_speed = Sfxr.pm.arp_speed
  ret.duty = Sfxr.pm.duty
  ret.duty_ramp = Sfxr.pm.duty_ramp
  ret.repeat_speed = Sfxr.pm.repeat_speed
  ret.pha_offset = Sfxr.pm.pha_offset
  ret.pha_ramp = Sfxr.pm.pha_ramp
  ret.filter_on = Sfxr.pm.filter_on
  ret.lpf_freq = Sfxr.pm.lpf_freq
  ret.lpf_ramp = Sfxr.pm.lpf_ramp
  ret.lpf_resonance = Sfxr.pm.lpf_resonance
  ret.hpf_freq = Sfxr.pm.hpf_freq
  ret.hpf_ramp = Sfxr.pm.hpf_ramp
  ret.cs_decimate = Sfxr.pm.cs_decimate
  ret.cs_compress = Sfxr.pm.cs_compress
  return ret
end

function Sfxr:getParamString()
  self:assertp()
  ffi.copy(Sfxr.pm,pSfxr.get_parameters(self.p),ffi.sizeof("struct _csParameters"))
  return ffi.string(Sfxr.pm,ffi.sizeof("struct _csParameters"))
end

function Sfxr:getPackedParams()
  self:assertp()
  ffi.copy(Sfxr.pm,pSfxr.get_parameters(self.p),ffi.sizeof("struct _csParameters"))
  local ret = {}
  ret[1] = Sfxr.pm.wave_type
  ret[2] = Sfxr.pm.env_attack
  ret[3] = Sfxr.pm.env_sustain
  ret[4] = Sfxr.pm.env_punch
  ret[5] = Sfxr.pm.env_decay
  ret[6] = Sfxr.pm.base_freq
  ret[7] = Sfxr.pm.freq_limit
  ret[8] = Sfxr.pm.freq_ramp
  ret[9] = Sfxr.pm.freq_dramp
  ret[10] = Sfxr.pm.vib_strength
  ret[11] = Sfxr.pm.vib_speed
  ret[12] = Sfxr.pm.vib_delay
  ret[13] = Sfxr.pm.arp_mod
  ret[14] = Sfxr.pm.arp_speed
  ret[15] = Sfxr.pm.duty
  ret[16] = Sfxr.pm.duty_ramp
  ret[17] = Sfxr.pm.repeat_speed
  ret[18] = Sfxr.pm.pha_offset
  ret[19] = Sfxr.pm.pha_ramp
  ret[20] = Sfxr.pm.filter_on
  ret[21] = Sfxr.pm.lpf_freq
  ret[22] = Sfxr.pm.lpf_ramp
  ret[23] = Sfxr.pm.lpf_resonance
  ret[24] = Sfxr.pm.hpf_freq
  ret[25] = Sfxr.pm.hpf_ramp
  ret[26] = Sfxr.pm.cs_decimate
  ret[27] = Sfxr.pm.cs_compress
  return ret
end

function Sfxr:getParamf(i)
  self:assertp()
  return pSfxr.get_param(self.p,i)
end

function Sfxr:setParamf(i,f)
  self:assertp()
  return pSfxr.set_param(self.p,i,f)
end

function Sfxr:mutate()
  self:assertp()
  pSfxr.mutate(self.p)
end

function Sfxr:randomize()
  self:assertp()
  pSfxr.randomize(self.p)
end

function Sfxr:getInfoQ()
  self:assertp()
  pSfxr.get_infoq(self.p,self.qi)
  local ret = {}
  ret.duration = self.pi.duration;
  ret.totalSamples = self.pi.totalSamples;
  ret.totalBytes = self.pi.totalSamples * 2;
  return ret;
end

function Sfxr:getInfo()
  self:assertp()
  pSfxr.get_info(self.p,self.fi)
  local ret = {}
  ret.duration = self.fi.duration;
  ret.totalSamples = self.fi.totalSamples;
  ret.totalBytes = self.fi.totalSamples * 2;
  ret.memoryUsed = self.fi.memoryUsed;
  ret.overhead = self.fi.overhead;
  ret.limit = self.fi.limit;
  ret.average = self.fi.average;
  return ret;
end

function Sfxr:soundData()
  self:assertp()
  pSfxr.get_infoq(self.p,self.qi)
  snd = love.sound.newSoundData(self.qi.totalSamples, 44100, 16, 1)
  pSfxr.export_buffer(self.p,Sfxr.ExportFormat.PCM16,snd:getPointer())
  return snd
end

function Sfxr:release()
  self:assertp()
  pSfxr._delete(self.p)
  self.p = nil
end

function Sfxr:init()
  self.p = pSfxr._new()
end

return Sfxr
