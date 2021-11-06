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
	struct sndParam {
		unsigned int strLen = 0;
		union {
			Sfxr::Parameters param;
			const char* str;
		};

		sndParam();
		sndParam(Sfxr::Parameters p) { param = p;  }
		sndParam(const char* p, unsigned int len) : sndParam() { str = p; strLen = len; }
	};

	class threadSfxr
	{
	private:
		Sfxr* pSfxr = nullptr;
		thread* pThread = nullptr;
		mutex mutexSfxr;
		mutex mutexList;
		mutex mutexState;
		vector<sndParam*> buildList;
		vector<Sfxr::SoundInfo*> infoList;
		vector<vector<char>> outputList;
		int building = -1;
		bool complete = false;
		bool filling = false;

	public:
		threadSfxr(unsigned int mode);

		void push(Sfxr::Parameters& p);
		void push(Sfxr::Parameters* p);
		void push(const char *str, unsigned int len = 0);

		void begin();
		void end();

		bool isFilling();
		bool isComplete();
		int getBuilding();

		void setComplete(bool s);
		void setFilling(bool s);
	};

	vector<threadSfxr*> threadTable;

	libSfxr(unsigned int threadCount, unsigned int mode);
};

