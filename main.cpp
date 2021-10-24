#include <iostream>
#include <fstream>
#include "cppSfxr.h"
#include <chrono>

using namespace std::chrono;
using namespace std;

class Benchmark
{
public:
	high_resolution_clock::time_point _start, _stop;
	duration<double> time_span;

	void start() { _start = high_resolution_clock::now(); }
	void stop() { _stop = high_resolution_clock::now(); }
	double duration() { time_span = (_stop - _start); return time_span.count();  }
};

int main()
{
	std::cout << "cppSxfr Test Run, GO!\n-\n\n";
	Sfxr* pSfxr = new Sfxr();

	// **********************************************************************************************************
	// create some sounds!
	std::cout << "\t *create a randomized sound! save as 16-bit randomize.wav!\n";
	pSfxr->create(-1);
	pSfxr->exportWaveFile("snd_randomize.wav");
	std::cout << "\t *save same sound as 8-bit pickup_coin_lowres.wav!\n";
	pSfxr->setPCM(0, 8);
	pSfxr->exportWaveFile("snd_randomize_lowres.wav");


	std::cout << "\t *testing string output PCM8 on randomize sample!\n";
	char* buffer;
	unsigned int blen = pSfxr->size(Sfxr::ExportFormat::PCM8);
	buffer = new char[blen];
	pSfxr->exportBuffer(Sfxr::ExportFormat::PCM8, buffer);
	ofstream ofs("snd_randomize_44100_8.raw", ios::binary);
	ofs.write(buffer, blen);

	pSfxr->setPCM();

	std::cout << "\t *create a pickup/coin sound! save as 16-bit pickup_coin.wav!\n";
	pSfxr->create(SFXR_PICKUP_COIN);
	pSfxr->exportWaveFile("snd_pickup_coin.wav");
	std::cout << "\t *save same sound as 8-bit pickup_coin_lowres.wav!\n";
	pSfxr->setPCM(0, 8);
	pSfxr->exportWaveFile("snd_pickup_coin_lowres.wav");

	pSfxr->setPCM();

	std::cout << "\t *create a explosion sound! save as a float explosion.wav!\n";
	pSfxr->create(SFXR_EXPLOSION);
	pSfxr->exportWaveFloatFile("snd_explosion.wav");

	std::cout << "\t *create a powerup sound! save as a float powerup.wav!\n";
	pSfxr->create(SFXR_POWERUP);
	pSfxr->exportWaveFloatFile("snd_powerup.wav");

	std::cout << "\t *create a hit/hurt sound! save as a float hit.wav!\n";
	pSfxr->create(SFXR_HIT_HURT);
	pSfxr->exportWaveFloatFile("snd_hit.wav");

	std::cout << "\t *create a laser/shoot sound! save as a float shoot.wav!\n";
	pSfxr->create(SFXR_LASER_SHOOT);
	pSfxr->exportWaveFloatFile("snd_shoot.wav");

	std::cout << "\t *create a jump sound! save as a float jump.wav!\n";
	pSfxr->create(SFXR_JUMP);
	pSfxr->exportWaveFloatFile("snd_hit.wav");

	std::cout << "\t *create a blip/select sound! save as a float blip.wav!\n";
	pSfxr->create(SFXR_BLIP_SELECT);
	pSfxr->exportWaveFloatFile("snd_blip.wav");

	std::cout << "\t *now going to benchmark the sample generation speed!\n";
	// **********************************************************************************************************
	// benchmark!
	double totalSamples = 0.0;
	double totalTime = 0.0;
	Sfxr::SoundQuickInfo Info;
	Benchmark bench;

	std::cout << "\t *creating 1000 pickups...\n";
	bench.start();
	for (int i = 0; i < 1000; i++)
	{
		pSfxr->create(SFXR_PICKUP_COIN);
		pSfxr->getInfo(Info);
		totalSamples += (double)Info.totalSamples;
	}
	bench.stop();
	totalTime += bench.duration();
	
	std::cout << "\t *creating 1000 powerups...\n";
	bench.start();
	for (int i = 0; i < 1000; i++)
	{
		pSfxr->create(SFXR_POWERUP);
		pSfxr->getInfo(Info);
		totalSamples += (double)Info.totalSamples;
	}
	bench.stop();
	totalTime += bench.duration();

	std::cout << "\t *creating 1000 explosions...\n";
	bench.start();
	for (int i = 0; i < 1000; i++)
	{
		pSfxr->create(SFXR_EXPLOSION);
		pSfxr->getInfo(Info);
		totalSamples += (double)Info.totalSamples;
	}
	bench.stop();
	totalTime += bench.duration();

	std::cout << "\t *creating 1000 hits...\n";
	bench.start();
	for (int i = 0; i < 1000; i++)
	{
		pSfxr->create(SFXR_HIT_HURT);
		pSfxr->getInfo(Info);
		totalSamples += (double)Info.totalSamples;
	}
	bench.stop();
	totalTime += bench.duration();

	std::cout << "\t *creating 1000 lasers...\n";
	bench.start();
	for (int i = 0; i < 1000; i++)
	{
		pSfxr->create(SFXR_LASER_SHOOT);
		pSfxr->getInfo(Info);
		totalSamples += (double)Info.totalSamples;
	}
	bench.stop();
	totalTime += bench.duration();

	std::cout << "\t *creating 1000 jumps...\n";
	bench.start();
	for (int i = 0; i < 1000; i++)
	{
		pSfxr->create(SFXR_JUMP);
		pSfxr->getInfo(Info);
		totalSamples += (double)Info.totalSamples;
	}
	bench.stop();
	totalTime += bench.duration();

	std::cout << "\t *creating 1000 blips...\n";
	bench.start();
	for (int i = 0; i < 1000; i++)
	{
		pSfxr->create(SFXR_BLIP_SELECT);
		pSfxr->getInfo(Info);
		totalSamples += (double)Info.totalSamples;
	}
	bench.stop();
	totalTime += bench.duration();

	std::cout << "\n-\n\t*benchmarks complete!\n";
	std::cout << "\t *time taken: " << totalTime << " seconds !\n";
	std::cout << "\t *sounds per second: " << 7000.0 / totalTime << " sound/sec !\n";
	std::cout << "\t *resultant seconds of samples per second: " << totalSamples / totalTime / 44100.0 << "x !\n";

	std::cout << "\n-\nTests complete!\n";
	delete pSfxr;
}

