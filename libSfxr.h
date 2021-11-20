#pragma once

/*
	library functions and utilities added to cppSfxr

	specifically: libraries of sounds and the ability to render them to PCM/Float data in threads.

  Jason A. Petrasko, muragami, 2021

  Apache-2.0 License:
		https://www.apache.org/licenses/LICENSE-2.0
*/

#include "cppSfxr.h"
#include <vector>
#include <array>
#include <thread>
#include <mutex>

using namespace std;

class libSfxr
{
public:
	// each sound paramater is either a block of data or an already processed param array
	struct sndParam {
		unsigned int strLen = 0;
		union {
			Sfxr::Parameters* pParam = nullptr;
			const char* pStr;
		};

		sndParam(Sfxr::Parameters p) { pParam = new Sfxr::Parameters(p);  }
		sndParam(const char* p, unsigned int len) { pStr = p; strLen = len; }
	};

	struct sndOutput
	{
	public:
		Sfxr::SoundInfo* pInfo = nullptr;
		char* pSample = nullptr;


	};

	// the thread magic that allows the system to load/create multiple sounds at once
	class threadSfxr
	{
	private:
		Sfxr* pSfxr = nullptr;
		Sfxr::ExportFormat eFormat = Sfxr::ExportFormat::PCM16;
		thread* pThread = nullptr;
		mutex mutexSfxr;
		mutex mutexList;
		mutex mutexState;
		vector<sndParam*> buildList;
		vector<sndOutput*> outputList;
		int building = -1;
		bool complete = false;
		bool filling = false;

	public:
		threadSfxr(unsigned int mode = 0, Sfxr::ExportFormat _format = Sfxr::ExportFormat::PCM16);

		void push(Sfxr::Parameters& p);
		void push(Sfxr::Parameters* p);
		void push(const char *str, unsigned int len = 0);

		void begin();
		void end();

		bool isFilling();
		bool isComplete();
		int getBuilding();
		int getBuildTotal();

		void setComplete(bool s);
		void setFilling(bool s);
		void setBuilding(int b);

		void build(int x);
	};

	vector<threadSfxr*> threadTable;

	libSfxr(unsigned int threadCount, unsigned int mode = 0, Sfxr::ExportFormat _format = Sfxr::ExportFormat::PCM16);
};
