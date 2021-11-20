/*
	library functions and utilities added to cppSfxr
  Jason A. Petrasko, muragami, 2021

  Apache-2.0 License:
		https://www.apache.org/licenses/LICENSE-2.0
*/

#define _USE_MATH_DEFINES
#include <cmath>

#include "libSfxr.h"

#include <complex>
#include <vector>
#include <iostream>
#include <iomanip>
#include <exception>
#include <stdexcept>
#include <cstring>
#include <memory.h>
#include <stdlib.h>

using namespace std;

unsigned int libSfxr_threadSfxr(void* p);

// *******************************************************************************
// some filter functions for imaging

void HannWindow(size_t N, vector<float>& output)
{
	output.clear();
	output.resize(N, 0.0f);

	float NF = static_cast<float>(N);
	size_t rnds = N >> 3;
	for (size_t r = 0; r < rnds; r++)
	{
		float n = (float)(r << 3);
		size_t i = r << 3;
		output[i] = 0.5f * (1.0f - cos((2.0f * (float)M_PI * n) / NF));
		output[i + 1] = 0.5f * (1.0f - cos((2.0f * (float)M_PI * (n + 1.0f)) / NF));
		output[i + 2] = 0.5f * (1.0f - cos((2.0f * (float)M_PI * (n + 2.0f)) / NF));
		output[i + 3] = 0.5f * (1.0f - cos((2.0f * (float)M_PI * (n + 3.0f)) / NF));
		output[i + 4] = 0.5f * (1.0f - cos((2.0f * (float)M_PI * (n + 4.0f)) / NF));
		output[i + 5] = 0.5f * (1.0f - cos((2.0f * (float)M_PI * (n + 5.0f)) / NF));
		output[i + 6] = 0.5f * (1.0f - cos((2.0f * (float)M_PI * (n + 6.0f)) / NF));
		output[i + 7] = 0.5f * (1.0f - cos((2.0f * (float)M_PI * (n + 7.0f)) / NF));
	}
}

void WindowMultiply(vector<float>& input, vector<float>& win, vector<float>& output)
{
	size_t len = input.size();
	size_t rnds = len >> 3;
	for (size_t i = 0; i < rnds; i++)
	{
		size_t pos = i << 3;
		output[pos] = input[pos] * win[pos];
		output[pos + 1] = input[pos + 1] * win[pos + 1];
		output[pos + 2] = input[pos + 2] * win[pos + 2];
		output[pos + 3] = input[pos + 3] * win[pos + 3];
		output[pos + 4] = input[pos + 4] * win[pos + 4];
		output[pos + 5] = input[pos + 5] * win[pos + 5];
		output[pos + 6] = input[pos + 6] * win[pos + 6];
		output[pos + 7] = input[pos + 7] * win[pos + 7];
	}
}

/* --- FFT --- */
inline size_t br(const size_t i, const size_t power)
{
	int j, k = 0;

	for (j = 0; j < power; j++)
		if ((i >> j) & 1)
			k = k | (1 << (power - j - 1));

	return k;
}

void FilterFFT(vector<float>& fft_re, vector<float>& fft_im, const float sign, const unsigned int fft_size)
{
	size_t blocks, points, i, power;

	power = (int)(log((float)fft_size) / log(2.0f));
	if (1 << power != fft_size) throw new std::runtime_error("fft_size is not a power of 2! in FilterFFT");

	/* bit reverse order */
	for (i = 0; i < fft_size; i++)
	{
		size_t k = br(i, power);
		if (i > k)
		{
			float t;

			t = fft_re[i];
			fft_re[i] = fft_re[k];
			fft_re[k] = t;

			t = fft_im[i];
			fft_im[i] = fft_im[k];
			fft_im[k] = t;
		}
	}

	/* pfft! */
	for (blocks = fft_size / 2, points = 2; blocks >= 1; blocks /= 2, points *= 2)
		for (i = 0; i < blocks; i++)
		{
			size_t j, offset = points * i;

			/* "butterfly" odd elements */
			for (j = 0; j < points / 2; j++)
			{
				float t_re, t_im, top_r, top_i, btm_r, btm_i, theta;
				theta = sign * 2.0f * (float)M_PI * (float)j / (float)points;
				t_re = cos(theta);
				t_im = sin(theta);

				top_r = fft_re[j + offset];
				top_i = fft_im[j + offset];

				btm_r = fft_re[j + points / 2 + offset] * t_re -
					fft_im[j + points / 2 + offset] * t_im;

				btm_i = fft_re[j + points / 2 + offset] * t_im +
					fft_im[j + points / 2 + offset] * t_re;

				fft_re[j + offset] = top_r + btm_r;
				fft_im[j + offset] = top_i + btm_i;

				fft_re[j + points / 2 + offset] = top_r - btm_r;
				fft_im[j + points / 2 + offset] = top_i - btm_i;
			}
		}
}

/*
void FilterDFT(vector<complex<double>>& input, vector<complex<double>>& output)
{
	unsigned int N = input.size();
	unsigned int K = N;

	complex<double> intSum;

	output.clear();
	output.reserve(K);

	for (int k = 0; k < K; k++)
	{
		intSum = complex<double>(0, 0);
		double dN = static_cast<double>(N);
		double dk = static_cast<double>(k);
		double dn = 0.0;
		for (int n = 0; n < N; n++)
		{
			double realPart = cos(((2.0 * M_PI) / dN) * dk * dn);
			double imagPart = sin(((2.0 * M_PI) / dN) * dk * dn);
			dn += 1.0;
			complex<double> w(realPart, -imagPart);
			intSum += input[n] * w;
		}
		output.push_back(intSum);
	}
}
*/

libSfxr::sndParam::sndParam()
{
	memset(&param, 0, sizeof(param));
}

libSfxr::threadSfxr::threadSfxr(unsigned int mode)
{
	pSfxr = new Sfxr();
	pSfxr->setMode(mode);
}

void libSfxr::threadSfxr::push(Sfxr::Parameters* p) { push(*p); }
void libSfxr::threadSfxr::push(Sfxr::Parameters& p)
{
	mutexList.lock();
	buildList.push_back(new sndParam(p));
	mutexList.unlock();
}

void libSfxr::threadSfxr::push(const char* str, unsigned int len)
{
	if (len == 0) len = (unsigned int)strlen(str);
	char* buff = new char[len];
	memcpy(buff, str, len);
	mutexList.lock();
	buildList.push_back(new sndParam(buff,len));
	mutexList.unlock();
}

void libSfxr::threadSfxr::begin()
{
	if (filling) return;
	mutexState.lock();
	filling = true;
	complete = false;
	pThread = new thread(&libSfxr_threadSfxr,this);
	mutexState.unlock();
}

void libSfxr::threadSfxr::end()
{

}

bool libSfxr::threadSfxr::isFilling()
{
	bool ret;
	mutexState.lock();
	ret = filling;
	mutexState.unlock();
	return ret;
}

bool libSfxr::threadSfxr::isComplete()
{
	bool ret;
	mutexState.lock();
	ret = complete;
	mutexState.unlock();
	return ret;
}

void libSfxr::threadSfxr::setComplete(bool s)
{
	mutexState.lock();
	complete = s;
	mutexState.unlock();
}

int libSfxr::threadSfxr::getBuilding()
{
	int ret;
	mutexState.lock();
	ret = building;
	mutexState.unlock();
	return ret;
}

void libSfxr::threadSfxr::setFilling(bool s)
{
	mutexState.lock();
	filling = s;
	mutexState.unlock();
}

libSfxr::libSfxr(unsigned int threadCount, unsigned int mode)
{
	threadTable.reserve(threadCount);
	for (unsigned int i = 0; i < threadCount; i++)
		threadTable.push_back(new threadSfxr(mode));
}

unsigned int libSfxr_threadSfxr(void* p)
{
	libSfxr::threadSfxr* pt = (libSfxr::threadSfxr*)p;

	bool ok = true;
	while (pt->isFilling() && ok)
	{
		int b = pt->getBuilding();

	}
	pt->setComplete(true);

	return 0;
}
