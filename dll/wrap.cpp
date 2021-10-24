
#include "../cppSfxr.h"

#ifdef _WIN32
#define DLLAPI __declspec(dllexport)
#else
#define DLLAPI
#endif

extern "C" {

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
        void* (*_new)();
        void (*_delete)(void* p);
        void (*seed_uint)(void* p, unsigned long long s);
        void (*seed_str)(void* p, const char* s);	// must be 4 bytes at least or even better 8 bytes!
        void (*reset)(void* p);
        void (*mutate)(void* p);
        void (*randomize)(void* p);
        void (*create_str)(void* p, const char* what);
        void (*create_int)(void* p, int what);
        void (*create)(void* p);
        void (*set_parameters)(void* p, csParameters* x);
        csParameters* (*get_parameters)(void* p);
        // these all are methods to read and save the parameter data
        bool (*load_file)(void* p, const char* fname);
        bool (*write_file)(void* p, const char* fname);
        bool (*load_string)(void* p, const char* data);
        bool (*write_string)(void* p, char* data);
        // this is the method to get the actual output
        bool (*export_buffer)(void* p, unsigned int method, void* pData);	// output to a buffer, use the size)() call to know how large to make it
        // write .wav files, if you are into that kind of thing
        bool (*export_wavefile)(void* p, const char* fname);
        bool (*export_wavefloatfile)(void* p, const char* fname);
        // get the output total size
        unsigned int (*size)(void* p, unsigned int method);
        // this will someday work right, as of right now changing the sample_rate alters the output quite a bit
        void (*set_PCM)(void* p, unsigned int sample_rate, unsigned int bit_depth);
        // using float format
        void (*set_float)(void* p);
        // this is non-trivial because we scan the output for limit and average samples, FYI
        void (*get_info)(void* p, csSoundInfo* info);
        // this is much quicker! and don't return all the extra info
        void (*get_infoq)(void* p, csSoundQuickInfo* info);
        // get an index of a named parameter
        int (*get_paramindex)(void* p, const char* pname);
        float (*get_param)(void* p, int index);
        void (*set_param)(void* p, int index, float f);
    };


#define CP ((Sfxr*)p)

    DLLAPI void* cs_new()
    {
        return new Sfxr();
    }

    DLLAPI void cs_delete(void* p)
    {
        delete CP;
    }

    DLLAPI void cs_reset(void* p)
    {
        CP->reset();
    }

    DLLAPI void cs_mutate(void* p)
    {
        CP->mutate();
    }

    DLLAPI void cs_randomize(void* p)
    {
        CP->randomize();
    }

    DLLAPI void cs_create_str(void* p, const char* what)
    {
        CP->create(what);
    }

    DLLAPI void cs_create_int(void* p, int what)
    {
        CP->create(what);
    }

    DLLAPI void cs_create(void* p)
    {
        CP->create();
    }

    DLLAPI void cs_set_parameters(void* p, csParameters* x)
    {
        CP->setParameters((Sfxr::Parameters*)x);
    }

    // these all are methods to read and save the parameter data
    DLLAPI bool cs_load_file(void* p, const char* fname)
    {
        return CP->loadFile(fname);
    }

    DLLAPI bool cs_write_file(void* p, const char* fname)
    {
        return CP->writeFile(fname);
    }

    DLLAPI bool cs_load_string(void* p, const char* data)
    {
        return CP->loadString(data);
    }

    DLLAPI bool cs_write_string(void* p, char* data)
    {
        return CP->writeString(data);
    }

    // this is the method to get the actual output
    DLLAPI bool cs_export_buffer(void* p, unsigned int method, void* pData)	// output to a buffer, use the size() call to know how large to make it
    {
        Sfxr::ExportFormat f;
        switch (method)
        {
        case SFXR_FORMAT_WAVE_PCM: f = Sfxr::ExportFormat::WAVE_PCM; break;
        case SFXR_FORMAT_WAVE_FLOAT: f = Sfxr::ExportFormat::WAVE_FLOAT; break;
        case SFXR_FORMAT_PCM8: f = Sfxr::ExportFormat::PCM8; break;
        case SFXR_FORMAT_PCM16: f = Sfxr::ExportFormat::PCM16; break;
        case SFXR_FORMAT_PCM24: f = Sfxr::ExportFormat::PCM24; break;
        case SFXR_FORMAT_PCM32: f = Sfxr::ExportFormat::PCM32; break;
        case SFXR_FORMAT_FLOAT: f = Sfxr::ExportFormat::FLOAT; break;
        default: return false;
        }
        return CP->exportBuffer(f, pData);
    }

    // write .wav files, if you are into that kind of thing
    DLLAPI bool cs_export_wavefile(void* p, const char* fname)
    {
        return CP->exportWaveFile(fname);
    }

    DLLAPI bool cs_export_wavefloatfile(void* p, const char* fname)
    {
        return CP->exportWaveFloatFile(fname);
    }

    // get the output total size
    DLLAPI unsigned int cs_size(void* p, unsigned int method)
    {
        Sfxr::ExportFormat f;
        switch (method)
        {
        case SFXR_FORMAT_WAVE_PCM: f = Sfxr::ExportFormat::WAVE_PCM; break;
        case SFXR_FORMAT_WAVE_FLOAT: f = Sfxr::ExportFormat::WAVE_FLOAT; break;
        case SFXR_FORMAT_PCM8: f = Sfxr::ExportFormat::PCM8; break;
        case SFXR_FORMAT_PCM16: f = Sfxr::ExportFormat::PCM16; break;
        case SFXR_FORMAT_PCM24: f = Sfxr::ExportFormat::PCM24; break;
        case SFXR_FORMAT_PCM32: f = Sfxr::ExportFormat::PCM32; break;
        case SFXR_FORMAT_FLOAT: f = Sfxr::ExportFormat::FLOAT; break;
        default: return false;
        }
        return CP->size(f);
    }

    // this will someday work right, as of right now changing the sample_rate alters the output quite a bit
    DLLAPI void cs_set_PCM(void* p, unsigned int sample_rate, unsigned int bit_depth)
    {
        CP->setPCM(sample_rate, bit_depth);
    }

    // using float format
    DLLAPI void cs_set_float(void* p)
    {
        CP->setFloat();
    }

    // this is non-trivial because we scan the output for limit and average samples, FYI
    DLLAPI void cs_get_info(void* p, csSoundInfo* info)
    {
        CP->getInfo((Sfxr::SoundInfo*)info);
    }

    // this is much quicker! and doesn't return all the extra info
    DLLAPI void cs_get_infoq(void* p, csSoundQuickInfo* info)
    {
        CP->getInfo((Sfxr::SoundQuickInfo*)info);
    }

    // get an index of a named parameter
    DLLAPI int cs_get_paramindex(void* p, const char* pname)
    {
        return CP->getParamIndex(pname);
    }

    DLLAPI float cs_get_param(void* p, int index)
    {
        return (*CP)[index];
    }

    DLLAPI void cs_set_param(void* p, int index, float f)
    {
        (*CP)[index] = f;
    }

    DLLAPI void cs_seed_uint(void* p, unsigned long long s)
    {
        CP->seed(s);
    }

    DLLAPI void cs_seed_str(void* p, const char* s)	// must be 4 bytes at least or even better 8 bytes!
    {
        CP->seed(s);
    }

    DLLAPI csParameters* cs_get_parameters(void* p)
    {
        return (csParameters*)CP->getParameters();
    }

    DLLAPI void cs_get(csSfxr* p)
    {
        p->_new = cs_new;
        p->_delete = cs_delete;
        p->reset = cs_reset;
        p->mutate = cs_mutate;
        p->randomize = cs_randomize;
        p->create_str = cs_create_str;
        p->create_int = cs_create_int;
        p->create = cs_create;
        p->set_parameters = cs_set_parameters;
        // these all are methods to read and save the parameter data
        p->load_file = cs_load_file;
        p->write_file = cs_write_file;
        p->load_string = cs_load_string;
        p->write_string = cs_write_string;
        // this is the method to get the actual output
        p->export_buffer = cs_export_buffer;	// output to a buffer, use the size)() call to know how large to make it
        // write .wav files, if you are into that kind of thing
        p->export_wavefile = cs_export_wavefile;
        p->export_wavefloatfile = cs_export_wavefloatfile;
        // get the output total size
        p->size = cs_size;
        // this will someday work right, as of right now changing the sample_rate alters the output quite a bit
        p->set_PCM = cs_set_PCM;
        // using float format
        p->set_float = cs_set_float;
        // this is non-trivial because we scan the output for limit and average samples, FYI
        p->get_info = cs_get_info;
        // this is much quicker! and don't return all the extra info
        p->get_infoq = cs_get_infoq;
        // get an index of a named parameter
        p->get_paramindex = cs_get_paramindex;
        p->get_param = cs_get_param;
        p->set_param = cs_set_param;

        p->seed_uint = cs_seed_uint;
        p->seed_str = cs_seed_str;
        p->get_parameters = cs_get_parameters;
    }

}
