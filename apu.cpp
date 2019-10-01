#include <iostream>
#include <cstdint>
#include <vector>
#include <deque>
#include "SDL2/include/SDL.h"
#include "mmu.h"
#define internal static

//	SC1 - Tone and Sweep
//	SC2 - Tone
//	SC3 - Wave Output
//	SC4 - Noise

int SamplesPerSecond = 44100;			//	resolution
bool SoundIsPlaying = false;

int cycle_count = 0;

std::vector<float> SC1buf;
std::vector<float> SC2buf;
std::vector<float> SC3buf;
std::vector<float> SC4buf;
std::vector<float> Mixbuf;
int16_t SC1timer = 0x00;
int16_t SC1timerTarget = 0x00;
uint32_t SC2timer = 0x00;
uint32_t SC3timer = 0x00;
uint32_t SC4timer = 0x00;
int16_t SC1amp = 0;
int16_t SC2amp = 0;
int16_t SC3amp = 0;
int16_t SC4amp = 0;
int16_t SC1freq = 0;
int16_t SC2freq = 0;
int16_t SC3freq = 0;
int16_t SC4freq = 0;
uint8_t SC1dutyIndex = 0;
uint8_t SC2dutyIndex = 0;
uint8_t SC3waveIndex = 0;
uint8_t SC1pcc = 95;
uint8_t SC2pcc = 95;
uint8_t SC3pcc = 95;
uint8_t SC4pcc = 95;
uint16_t SC1pcFS = 0;
uint16_t SC2pcFS = 0;
uint16_t SC3pcFS = 0;
uint16_t SC4pcFS = 0;
uint8_t SC1FrameSeq = 0;
uint8_t SC2FrameSeq = 8;
uint8_t SC3FrameSeq = 8;
uint8_t SC4FrameSeq = 7;
int16_t SC1len = 0;
uint8_t SC2len = 0;
uint8_t SC3len = 0;
uint8_t SC4len = 0;
int16_t SC1envelopeDivider = 0;
int16_t SC1envelopeVol = 0;
int16_t SC2envelopeDivider = 0;
bool SC1enabled = false;
bool SC2enabled = false;
bool SC3enabled = false;
bool SC4enabled = false;
bool SC1envelopeStart = false;
bool SC2envelopeStart = false;
bool SC4envelopeEnabled = false;
bool SC1sweepReload = false;
int8_t SC1sweepPeriod = 0;
uint32_t SC1sweepShadow = 0;
int16_t SC1sweepDivider = 0;
int16_t SC1envelope = 0;
int16_t SC2envelope = 0;
uint8_t SC4envelope = 0;
uint8_t SC4divisor[8] = { 8, 16, 32, 48, 64, 80, 96, 112 };
uint16_t SC4lfsr = 0;
uint16_t apu_cycles = 0;

bool useSC1 = true;
bool useSC2 = true;
bool useSC3 = true;
bool useSC4 = true;
bool bit_remix = false;

float volume = 0.5;

int frames_per_sample = 18;		//	TODO: mit diesem Wert kann die Geschwindigkeit der Emulation direkt beeinflusst werden --> UI einbauen

//	duty table
/*uint8_t duties[4][8] = {
	{0, 1, 0, 0, 0, 0, 0, 0 },
	{0, 1, 1, 0, 0, 0, 0, 0 },
	{0, 1, 1, 1, 1, 0, 0, 0 },
	{1, 0, 0, 1, 1, 1, 1, 1}
};*/

/*0 0 0 0 0 0 0 1	0 1 0 0 0 0 0 0 (12.5%)
1	0 0 0 0 0 0 1 1	0 1 1 0 0 0 0 0 (25%)
2	0 0 0 0 1 1 1 1	0 1 1 1 1 0 0 0 (50%)
3	1 1 1 1 1 1 0 0*/

uint8_t duties[4][8] = {
	{0, 0, 0, 0, 0, 0, 0, 1 },
	{0, 0, 0, 0, 0, 0, 1, 1 },
	{0, 0, 0, 0, 1, 1, 1, 1 },
	{1, 1, 1, 1, 1, 1, 0, 0 }
};

//	length table
uint16_t length_table[0x20] = {
	10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14, 12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

uint16_t sweepCalc();

void toggleAudio() {
	if (!useSC1) {
		useSC1 = true;
		useSC2 = true;
		useSC3 = true;
		useSC4 = true;
	}
	else {
		useSC1 = false;
		useSC2 = false;
		useSC3 = false;
		useSC4 = false;
	}
}

void toggleSC1() {
	useSC1 = !useSC1;
}
void toggleSC2() {
	useSC2 = !useSC2;
}
void toggleSC3() {
	useSC3 = !useSC3;
}
void toggleSC4() {
	useSC4 = !useSC4;
}
void toggleRemix() {
	bit_remix = !bit_remix;
}
void setVolume(float v) {
	volume = v;
}

internal void SDLInitAudio(int32_t SamplesPerSecond, int32_t BufferSize)
{
	SDL_AudioSpec AudioSettings = { 0 };

	AudioSettings.freq = SamplesPerSecond;
	AudioSettings.format = AUDIO_F32SYS;		//	One of the modes that doesn't produce a high frequent pitched tone when having silence
	AudioSettings.channels = 2;
	AudioSettings.samples = BufferSize;

	SDL_OpenAudio(&AudioSettings, 0);

}

//	Square1 channel
void stepSC1(uint8_t c) {

	while (c--) {

		//	Frame Sequencer (every other CPU tick = 1 APU tick)
		/*	mode 0:     mode 1 :	 function
			-------- -  ---------- - ---------------------------- -
			- - - f		- - - - -	 IRQ(if bit 6 is clear)
			- l - l		- l - - l    Length counter and sweep
			e e e e     e e e - e    Envelope and linear counter
		*/
		SC1pcFS++;
		if (SC1pcFS % 2 == 0) {
			apu_cycles++;

			if (((apu_cycles == 3729 || apu_cycles == 7457 || apu_cycles == 11186 || apu_cycles == 14915) && (readFromMem(0x4017) >> 7)) || 
			   ((apu_cycles == 3729 || apu_cycles == 7457 || apu_cycles == 11186 || apu_cycles == 18641) && (readFromMem(0x4017) >> 7) == 0)) {

				if (!SC1envelopeStart) {
					SC1envelopeDivider--;
					if (SC1envelopeDivider < 0) {
						SC1envelopeDivider = readFromMem(0x4000) & 0b1111;
						if (SC1envelope > 0) {
							SC1envelope--;
							SC1envelopeVol = SC1envelope;
						}
						else {
							SC1envelopeVol = SC1envelope;
							if (readFromMem(0x4000) & 0b100000) {
								SC1envelope = 15;
							}
						}
					}
				} 
				else {
					SC1envelopeStart = false;
					SC1envelope = 15;
					SC1envelopeDivider = readFromMem(0x4000) & 0b1111;
				}

			}

			if (readFromMem(0x4000) & 0b10000) {
				SC1amp = readFromMem(0x4000) & 0b1111;
			}
			else {
				SC1amp = SC1envelopeVol;
			}


			//	Length Counter & Sweep
			if (apu_cycles == 7456 || apu_cycles == 14915) {

				//	Length Counter - NOT halted by flag
				if ((readFromMem(0x4000) & 0b00010000) == 0x00) {
					//	length > 0
					if (SC1len) {
						SC1len--;
					}
					else {
						printf("disable \n");
						SC1enabled = false;
					}
				}

				//	Sweep
				if ((readFromMem(0x4001) & 0b10000000)) {
					SC1sweepDivider--;
					if (SC1sweepDivider < 0) {
						printf("Timer pre %d\n", SC1timerTarget);
						int16_t post = SC1timerTarget >> (readFromMem(0x4001) & 0b111);
						int8_t neg = (readFromMem(0x4001) & 0b1000) ? -1 : 1;
						int16_t sum = (uint16_t)(post * neg);
						SC1timerTarget = SC1timerTarget + sum;
						//shadowWriteToMem(0x4003, (readFromMem(0x4003) & 0b11111000) | (SC1timer >> 8));
						//shadowWriteToMem(0x4002, SC1timer & 0xff);
						/*writeToMem(0x4003, (SC1timer >> 8) & 0b111);
						writeToMem(0x4002, SC1timer & 0xff);*/
						printf("Timer Target %d %d %d shift: %d postneg: %d \n", SC1timerTarget, post, neg, readFromMem(0x4001) & 0b111, sum);
					}
					if(SC1sweepDivider < 0 || SC1sweepReload) {
						SC1sweepReload = false;
						SC1sweepDivider = (readFromMem(0x4001) >> 4) & 0b111;
					}
				}

			}

			//	wrap
			apu_cycles %= 14915;

			//	handle timer
			if (SC1timer <= 0x00) {
				SC1timer = SC1timerTarget;

				//	tick duty pointer
				++SC1dutyIndex %= 8;
			}
			else {
				SC1timer--;
			}

			//	handle duty
			int duty = readFromMem(0x4000) >> 6;
			if (duties[duty][SC1dutyIndex] == 1)
				SC1freq = SC1amp;
			else
				SC1freq = 0;

			if (!--SC1pcc) {
				SC1pcc = frames_per_sample;
				//	enabled channel
				if (SC1enabled) {
					SC1buf.push_back((float)SC1freq / 100);
					SC1buf.push_back((float)SC1freq / 100);
				}
				//	disabled channel
				else {
					SC1buf.push_back(0);
					SC1buf.push_back(0);
				}
			}

		}
	}

}

//	Square2 channel
void stepSC2(uint8_t c) {


}

//	WAVE channel
void stepSC3(uint8_t c) {

}

//	Noise channel
void stepSC4(uint8_t c) {


}

void stepAPU(unsigned char cycles) {

	stepSC1(cycles);
	stepSC2(cycles);
	stepSC3(cycles);
	stepSC4(cycles);

	if (SC1buf.size() >= 100) { //&& SC2buf.size() >= 100 && SC3buf.size() >= 100 && SC4buf.size() >= 100) {

		for (int i = 0; i < 100; i++) {
			float res = 0;
			if (useSC1)
				res += SC1buf.at(i) * volume;
			if (useSC1)
				res += SC1buf.at(i) * volume;
			if (useSC1)
				res += SC1buf.at(i) * volume;
			if (useSC1)
				res += SC1buf.at(i) * volume;
			/*if (useSC2)
				res += SC2buf.at(i) * volume;
			if (useSC3)
				res += SC3buf.at(i) * volume;
			if (useSC4)
				res += SC4buf.at(i) * volume;*/
			Mixbuf.push_back(res);
		}
		//	send audio data to device; buffer is times 4, because we use floats now, which have 4 bytes per float, and buffer needs to have information of amount of bytes to be used
		SDL_QueueAudio(1, Mixbuf.data(), Mixbuf.size() * 4);

		SC1buf.clear();
		SC2buf.clear();
		SC3buf.clear();
		SC4buf.clear();
		Mixbuf.clear();

		//TODO: we could, instead of just idling everything until music buffer is drained, at least call stepPPU(0), to have a constant draw cycle, and maybe have a smoother drawing?
		while (SDL_GetQueuedAudioSize(1) > 4096 * 4) {
		}
	}

}

void initAPU() {

	SDL_setenv("SDL_AUDIODRIVER", "directsound", 1);
	//SDL_setenv("SDL_AUDIODRIVER", "disk", 1);
	SDL_Init(SDL_INIT_AUDIO);

	// Open our audio device; Sample Rate will dictate the pace of our synthesizer
	SDLInitAudio(44100, 1024);

	if (!SoundIsPlaying)
	{
		SDL_PauseAudio(0);
		SoundIsPlaying = true;
	}
}

void stopSPU() {
	SDL_Quit();
	SC1buf.clear();
	SC2buf.clear();
	SC3buf.clear();
	SC4buf.clear();
	Mixbuf.clear();
	SoundIsPlaying = false;
	cycle_count = 0;
	SC1timer = 0x00;
	SC2timer = 0x00;
	SC3timer = 0x00;
	SC4timer = 0x00;
	SC1amp = 0;
	SC2amp = 0;
	SC3amp = 0;
	SC4amp = 0;
	SC1freq = 0;
	SC2freq = 0;
	SC3freq = 0;
	SC4freq = 0;
	SC1dutyIndex = 0;
	SC2dutyIndex = 0;
	SC3waveIndex = 0;
	SC1pcc = frames_per_sample;
	SC2pcc = frames_per_sample;
	SC3pcc = frames_per_sample;
	SC4pcc = frames_per_sample;
	SC1pcFS = 0;
	SC2pcFS = 0;
	SC3pcFS = 0;
	SC4pcFS = 0;
	SC1FrameSeq = 0;
	SC2FrameSeq = 0;
	SC3FrameSeq = 0;
	SC4FrameSeq = 0;
	SC1len = 0;
	SC2len = 0;
	SC3len = 0;
	SC4len = 0;
	SC1enabled = false;
	SC2enabled = false;
	SC3enabled = false;
	SC4enabled = false;
	SC1envelopeStart = false;
	SC2envelopeStart = false;
	SC4envelopeEnabled = false;
	SC1sweepReload = false;
	SC1sweepPeriod = 0;
	SC1sweepShadow = 0;
	SC1envelope = 0;
	SC2envelope = 0;
	SC4envelope = 0;
	SC4lfsr = 0;
}

//	reloads the length counter for SC1, with all the other according settings
/*
Channel is enabled (see length counter).
If length counter is zero, it is set to 64 (256 for wave channel).
Channel volume is reloaded from NRx2.
Volume envelope timer is reloaded with period.
Enable Volume envelope <----- LILAQ
Wave channel's position is set to 0 but sample buffer is NOT refilled.

Frequency timer is reloaded with period.
Noise channel's LFSR bits are all set to 1.
Square 1's sweep does several things (see frequency sweep).
*/
void resetSC1length(uint8_t val) {

	SC1len = length_table[val >> 3];
	SC1enabled = true;
	SC1timerTarget = ((readFromMem(0x4003) & 0b111) << 8) | readFromMem(0x4002);
	SC1dutyIndex = 0;

}

void resetSC1Envelope() {
	SC1envelopeStart = true;
	printf("ENV RELOAD FUNC\n");
}

void resetSC1Sweep() {
	SC1sweepReload = true;
	//	setting up sweep
	/*SC1sweepPeriod = ((readFromMem(0x4001) >> 4) & 7) + 1;
	int SC1sweepShift = readFromMem(0x4001) & 7;
	int SC1sweepNegate = ((readFromMem(0x4001) >> 3) & 1) ? -1 : 1;
	if (SC1sweepPeriod && SC1sweepShift)	//	this was an OR before. Change if needed
		SC1sweepReload = true;
	else
		SC1sweepReload = false;
	SC1sweepShadow = (((readFromMem(0x4003) & 7) << 8) | readFromMem(0x4002));
	if (SC1sweepShift) {
		if ((SC1sweepShadow + ((SC1sweepShadow >> SC1sweepShift) * SC1sweepNegate)) > 2047) {
			SC1sweepReload = false;
			SC1enabled = false;
		}
	}*/
}

//	reloads the length counter for SC2, with all the other according settings
void resetSC2length(uint8_t val) {

	SC2len = length_table[val >> 3];
	//printf("Reload SC2len with: %d (from val: %x) ", SC2len, val);

	SC2enabled = true;

	SC2amp = readFromMem(0x4004) & 0b1111;

	SC2envelope = readFromMem(0x4004) & 0xb1111; // ???
	SC2envelopeStart = true;

	SC2timer = ((readFromMem(0x4007) & 0b111) << 8) | readFromMem(0x4006);
	//printf("Timer (SC2): %d -> from 0x4003: 0x%02x and 0x4002: 0x%02x\n", SC2timer, readFromMem(0x4007), readFromMem(0x4006));

	SC2dutyIndex = 0;

}

void resetSC2Envelope() {
	SC2envelopeStart = true;
}

//	reloads the length counter for SC3, with all the other according settings
void resetSC3length(uint8_t val) {
	uint16_t r = (((readFromMem(0xff1e) & 7) << 8) | readFromMem(0xff1d));
	SC3timer = (2048 - r) * 2;
	if (!SC3len)
		SC3len = 256 - val;
	SC3enabled = true;
	SC3waveIndex = 0;

	//	disable channel if dac is off
	if ((readFromMem(0xff1a) >> 6) == 0x0)
		SC3enabled = false;
}

//	reloads the length counter for SC4, with all the other according settings
void resetSC4length(uint8_t val) {

	if (!SC4len)
		SC4len = 64 - val;
	SC4enabled = true;
	SC4timer = SC4divisor[readFromMem(0xff22) & 0x7] << (readFromMem(0xff22) >> 4);
	SC4lfsr = 0x7fff;
	SC4amp = readFromMem(0xff21) >> 4;
	SC4envelope = readFromMem(0xff21) & 7;
	SC4envelopeEnabled = true;

	//	disable channel if dac is off
	if ((readFromMem(0xff21) >> 3) == 0x0)
		SC4enabled = false;
}
