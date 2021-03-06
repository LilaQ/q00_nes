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
int16_t SC2timer = 0x00;
int16_t SC2timerTarget = 0x00;
int16_t SC3timer = 0x00;
int16_t SC3timerTarget = 0x00;
int16_t SC4timer = 0x00;
int16_t SC4timerTarget = 0x00;
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
uint8_t SC3ampIndex = 0;
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
int16_t SC2len = 0;
int16_t SC3len = 0;
int16_t SC4len = 0;
int16_t SC1envelopeDivider = 0;
int16_t SC1envelopeVol = 0;
int16_t SC2envelopeDivider = 0;
int16_t SC2envelopeVol = 0;
int16_t SC4envelopeDivider = 0;
int16_t SC4envelopeVol = 0;
bool SC1enabled = false;
bool SC2enabled = false;
bool SC3enabled = false;
bool SC4enabled = false;
bool SC1envelopeStart = false;
bool SC2envelopeStart = false;
bool SC4envelopeStart = false;
bool SC1sweepReload = false;
bool SC2sweepReload = false;
bool SC1sweepEnabled = false;
bool SC2sweepEnabled = false;
bool SC1constantVolFlag = false;
bool SC4constantVolFlag = false;
int8_t SC1sweepPeriod = 0;
int8_t SC2sweepPeriod = 0;
int8_t SC1constantVol = 0;
int8_t SC4constantVol = 0;
uint32_t SC1sweepShadow = 0;
uint32_t SC2sweepShadow = 0;
int16_t SC1sweepDivider = 0;
int16_t SC2sweepDivider = 0;
int16_t SC1envelope = 0;
int16_t SC2envelope = 0;
uint8_t SC4envelope = 0;
uint8_t SC4divisor[8] = { 8, 16, 32, 48, 64, 80, 96, 112 };
uint16_t SC4lfsr = 1;
uint16_t SC3linearCounter = 0;
uint16_t SC3linearReloadValue = 0;
bool SC3linearReload = false;
bool SC3controlFlag = false;
bool SC4modeFlag = false;
uint16_t apu_cycles_sc1 = 0;
uint16_t apu_cycles_sc2 = 0;
uint16_t apu_cycles_sc3 = 0;
uint16_t apu_cycles_sc4 = 0;

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

uint8_t duties[4][8] = {
	{0, 0, 0, 0, 0, 0, 0, 1 },
	{0, 0, 0, 0, 0, 0, 1, 1 },
	{0, 0, 0, 0, 1, 1, 1, 1 },
	{1, 1, 1, 1, 1, 1, 0, 0 }
};

uint8_t SC3triangleAmps[32] = { 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

//	length table
uint16_t length_table[0x20] = {
	10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14, 12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

//	noise table(s)
uint8_t noise_ntsc[0x10] = {
	 4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};
uint8_t noise_pal[0x10] = {
	 4, 8, 14, 30, 60, 88, 118, 148, 188, 236, 354, 472, 708,  944, 1890, 3778
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
			apu_cycles_sc1++;

			if (((apu_cycles_sc1 == 3729 || apu_cycles_sc1 == 7457 || apu_cycles_sc1 == 11186 || apu_cycles_sc1 == 14915) && (readFromMem(0x4017) >> 7)) ||
			   ((apu_cycles_sc1 == 3729 || apu_cycles_sc1 == 7457 || apu_cycles_sc1 == 11186 || apu_cycles_sc1 == 18641) && (readFromMem(0x4017) >> 7) == 0)) {

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

			if (SC1constantVolFlag) {
				SC1amp = SC1constantVol;
			}
			else {
				SC1amp = SC1envelopeVol;
			}

			//	Length Counter & Sweep
			if (apu_cycles_sc1 == 7456 || apu_cycles_sc1 == 14915) {

				//	Length Counter - NOT halted by flag
				if ((readFromMem(0x4000) & 0x20) == 0x00) {
					//	length > 0
					if (SC1len) {
						SC1len--;
					}
				}

				//	Sweep
				if (SC1sweepEnabled) {
					SC1sweepDivider--;
					if (SC1sweepDivider < 0) {
						int16_t post = SC1timerTarget >> (readFromMem(0x4001) & 0b111);
						int8_t neg = (readFromMem(0x4001) & 0b1000) ? -1 : 1;
						int16_t sum = (uint16_t)(post * neg);
						SC1timerTarget = SC1timerTarget + sum;
						if (SC1timerTarget >= 0x7ff || SC1timerTarget <= 8) {
							SC1amp = 0;
							writeToMem(0x4001, readFromMem(0x4001) & 0x7f);		//	< --  TODO SC2
							SC1enabled = false;									//	< --  TODO SC2
						}
					}
					if(SC1sweepDivider < 0 || SC1sweepReload) {
						SC1sweepReload = false;
						SC1sweepDivider = (readFromMem(0x4001) >> 4) & 0b111;
					}
				}
			}
			if (SC1len <= 0) {
				SC1enabled = false;
			}

			//	wrap
			apu_cycles_sc1 %= 14915;

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
				if (SC1enabled && (readFromMem(0x4015) & 0b1) && SC1len) {
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

	while (c--) {

		//	Frame Sequencer (every other CPU tick = 1 APU tick)
		/*	mode 0:     mode 1 :	 function
			-------- -  ---------- - ---------------------------- -
			- - - f		- - - - -	 IRQ(if bit 6 is clear)
			- l - l		- l - - l    Length counter and sweep
			e e e e     e e e - e    Envelope and linear counter
		*/
		SC2pcFS++;
		if (SC2pcFS % 2 == 0) {
			apu_cycles_sc2++;

			if (((apu_cycles_sc2 == 3729 || apu_cycles_sc2 == 7457 || apu_cycles_sc2 == 11186 || apu_cycles_sc2 == 14915) && (readFromMem(0x4017) >> 7)) ||
				((apu_cycles_sc2 == 3729 || apu_cycles_sc2 == 7457 || apu_cycles_sc2 == 11186 || apu_cycles_sc2 == 18641) && (readFromMem(0x4017) >> 7) == 0)) {

				if (!SC2envelopeStart) {
					SC2envelopeDivider--;
					if (SC2envelopeDivider < 0) {
						SC2envelopeDivider = readFromMem(0x4004) & 0b1111;
						if (SC2envelope > 0) {
							SC2envelope--;
							SC2envelopeVol = SC2envelope;
						}
						else {
							SC2envelopeVol = SC2envelope;
							if (readFromMem(0x4004) & 0b100000) {
								SC2envelope = 15;
							}
						}
					}
				}
				else {
					SC2envelopeStart = false;
					SC2envelope = 15;
					SC2envelopeDivider = readFromMem(0x4004) & 0b1111;
				}
			}

			if (readFromMem(0x4004) & 0b10000) {
				SC2amp = readFromMem(0x4004) & 0b1111;
			}
			else {
				SC2amp = SC2envelopeVol;
			}

			//	Length Counter & Sweep
			if (apu_cycles_sc2 == 7456 || apu_cycles_sc2 == 14915) {

				//	Length Counter - NOT halted by flag
				if ((readFromMem(0x4004) & 0b00010000) == 0x00) {
					//	length > 0
					if (SC2len) {
						SC2len--;
					}
					else {
						SC2enabled = false;
					}
				}

				//	Sweep
				if (SC2sweepEnabled) {
					SC2sweepDivider--;
					if (SC2sweepDivider < 0) {
						int16_t post = SC2timerTarget >> (readFromMem(0x4005) & 0b111);
						int8_t neg = (readFromMem(0x4005) & 0b1000) ? -1 : 1;
						int16_t sum = (uint16_t)(post * neg);
						SC2timerTarget = SC2timerTarget + sum;
						if (SC2timerTarget >= 0x7ff || SC2timerTarget <= 8) {
							SC2amp = 0;
						}
					}
					if (SC2sweepDivider < 0 || SC2sweepReload) {
						SC2sweepReload = false;
						SC2sweepDivider = (readFromMem(0x4005) >> 4) & 0b111;
					}
				}
			}

			//	wrap
			apu_cycles_sc2 %= 14915;

			//	handle timer
			if (SC2timer <= 0x00) {
				SC2timer = SC2timerTarget;

				//	tick duty pointer
				++SC2dutyIndex %= 8;
			}
			else {
				SC2timer--;
			}

			//	handle duty
			int duty = readFromMem(0x4004) >> 6;
			if (duties[duty][SC2dutyIndex] == 1)
				SC2freq = SC2amp;
			else
				SC2freq = 0;

			if (!--SC2pcc) {
				SC2pcc = frames_per_sample;
				//	enabled channel
				if (SC2enabled && SC2len) {
					SC2buf.push_back((float)SC2freq / 100);
					SC2buf.push_back((float)SC2freq / 100);
				}
				//	disabled channel
				else {
					SC2buf.push_back(0);
					SC2buf.push_back(0);
				}
			}

		}
	}

}

//	TRIANGLE channel
void stepSC3(uint8_t c) {
	while (c--) {
		SC3pcFS++;
		if (SC3pcFS % 2 == 0) {
			apu_cycles_sc3++;
			//	Length Counter
			if (apu_cycles_sc3 == 7456 || apu_cycles_sc3 == 14915) {

				//	Length Counter - NOT halted by flag
				if ((readFromMem(0x4000) & 0b00010000) == 0x00) {
					//	length > 0
					if (SC3len) {
						SC3len--;
					}
					else {
						SC3enabled = false;
					}
				}
			}

			//	Linear Counter
			if (((apu_cycles_sc3 == 3729 || apu_cycles_sc3 == 7457 || apu_cycles_sc3 == 11186 || apu_cycles_sc3 == 14915) && (readFromMem(0x4017) >> 7)) ||
				((apu_cycles_sc3 == 3729 || apu_cycles_sc3 == 7457 || apu_cycles_sc3 == 11186 || apu_cycles_sc3 == 18641) && (readFromMem(0x4017) >> 7) == 0)) {

				if (SC3linearReload) {
					SC3linearCounter = SC3linearReloadValue;
				}
				else if (SC3linearCounter > 0) {
					SC3linearCounter--;
				}
				if (!SC3controlFlag) {
					SC3linearReload = false;
				}

			}
			//	wrap
			apu_cycles_sc3 %= 14915;
		}

		//	handle timer
		if (SC3timer == 0x00) {
			SC3timer = SC3timerTarget;

			if (SC3linearCounter && SC3len) {

				//++SC3ampIndex %= 32;
				SC3ampIndex = (SC3ampIndex + 1) & 0x1F;

				//	handle amp from table
				if (SC3timerTarget >= 2 && SC3timerTarget <= 0x7ff) {
					SC3freq = SC3triangleAmps[SC3ampIndex];
				}
			}

		}
		else {
			SC3timer--;
		}

		if(SC3pcFS %2 == 0){
			if (!--SC3pcc) {
				SC3pcc = frames_per_sample;
				//	enabled channel
				if (SC3enabled && (readFromMem(0x4015) & 0x4) && SC3len) {
					SC3buf.push_back((float)SC3freq / 100);
					SC3buf.push_back((float)SC3freq / 100);
				}
				//	disabled channel
				else {
					SC3buf.push_back(0);
					SC3buf.push_back(0);
				}
			}
		}
	}
}


//	Noise channel
void stepSC4(uint8_t c) {

	while (c--) {

		//	Frame Sequencer (every other CPU tick = 1 APU tick)
		/*	mode 0:     mode 1 :	 function
			-------- -  ---------- - ---------------------------- -
			- - - f		- - - - -	 IRQ(if bit 6 is clear)
			- l - l		- l - - l    Length counter and sweep
			e e e e     e e e - e    Envelope and linear counter
		*/
		SC4pcFS++;
		if (SC4pcFS % 2 == 0) {
			apu_cycles_sc4++;

			if (((apu_cycles_sc4 == 3729 || apu_cycles_sc4 == 7457 || apu_cycles_sc4 == 11186 || apu_cycles_sc4 == 14915) && (readFromMem(0x4017) >> 7)) ||
				((apu_cycles_sc4 == 3729 || apu_cycles_sc4 == 7457 || apu_cycles_sc4 == 11186 || apu_cycles_sc4 == 18641) && (readFromMem(0x4017) >> 7) == 0)) {

				if (!SC4envelopeStart) {
					SC4envelopeDivider--;
					if (SC4envelopeDivider < 0) {
						SC4envelopeDivider = readFromMem(0x400c) & 0b1111;
						if (SC4envelope > 0) {
							SC4envelope--;
							SC4envelopeVol = SC4envelope;
						}
						else {
							SC4envelopeVol = SC4envelope;
							if (readFromMem(0x400c) & 0b100000) {
								SC4envelope = 15;
							}
						}
					}
				}
				else {
					SC4envelopeStart = false;
					SC4envelope = 15;
					SC4envelopeDivider = readFromMem(0x400c) & 0b1111;
				}
			}

			if (SC4constantVolFlag) {
				SC4amp = SC4constantVol;
			}
			else {
				SC4amp = SC4envelopeVol;
			}

			//	Length Counter & Sweep
			if (apu_cycles_sc4 == 7456 || apu_cycles_sc4 == 14915) {

				//	Length Counter - NOT halted by flag
				if ((readFromMem(0x400c) & 0x20) == 0x00) {
					//	length > 0
					if (SC4len) {
						SC4len--;
					}
				}

			}
			if (SC4len <= 0) {
				SC4enabled = false;
			}

			//	wrap
			apu_cycles_sc4 %= 14915;

			//	handle timer
			if (SC4timer <= 0x00) {
				SC4timer = SC4timerTarget;

				//	calc lsfr
				/*When the timer clocks the shift register, the following actions occur in order:

				Feedback is calculated as the exclusive-OR of bit 0 and one other bit: bit 6 if Mode flag is set, otherwise bit 1.
				The shift register is shifted right by one bit.
				Bit 14, the leftmost bit, is set to the feedback calculated earlier.*/
				uint16_t feedback = (SC4modeFlag) ? (SC4lfsr & 1) ^ ((SC4lfsr >> 6) & 1) : (SC4lfsr & 1) ^ ((SC4lfsr >> 1) & 1);
				SC4lfsr >>= 1;
				SC4lfsr |= feedback << 14;
			}
			else {
				SC4timer--;
			}


			if (!--SC4pcc) {
				SC4pcc = frames_per_sample;
				//	enabled channel
				if (SC4enabled && (readFromMem(0x4015) & 0b1000) && SC4len) {
					SC4buf.push_back((SC4lfsr & 1) ? 0 : ((float)SC4amp / 100));
					SC4buf.push_back((SC4lfsr & 1) ? 0 : ((float)SC4amp / 100));
				}
				//	disabled channel
				else {
					SC4buf.push_back(0);
					SC4buf.push_back(0);
				}
			}

		}
	}

}

void stepAPU(unsigned char cycles) {

	stepSC1(cycles);
	stepSC2(cycles);
	stepSC3(cycles);
	stepSC4(cycles);

	if (SC1buf.size() >= 100 && SC2buf.size() >= 100 && SC3buf.size() >= 100 && SC4buf.size() >= 100) {

		for (int i = 0; i < 100; i++) {
			float res = 0;
			if (useSC1)
				res += SC1buf.at(i) * volume;
			if (useSC2)
				res += SC2buf.at(i) * volume;
			if (useSC3)
				res += SC3buf.at(i) * volume;
			if (useSC4)
				res += SC4buf.at(i) * volume;
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
	SC3ampIndex = 0;
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
	SC4envelopeStart = false;
	SC1sweepReload = false;
	SC1sweepPeriod = 0;
	SC1sweepShadow = 0;
	SC1envelope = 0;
	SC2envelope = 0;
	SC4envelope = 0;
	SC4lfsr = 1;
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
	SC1len = length_table[(val >> 3) & 0b11111];
	SC1enabled = SC1len > 0;
	SC1timerTarget = ((readFromMem(0x4003) & 0b111) << 8) | readFromMem(0x4002);
	SC1dutyIndex = 0;
}

void resetSC1Envelope() {
	SC1envelopeStart = true;
}

void resetSC1Sweep() {
	SC1sweepReload = true;
	SC1sweepEnabled = (readFromMem(0x4001) & 0x80) == 0x80;
}

void resetSC1hi() {
	SC1timerTarget = ((readFromMem(0x4003) & 0b111) << 8) | readFromMem(0x4002);
}

void resetSC1Ctrl() {
	SC1constantVol = readFromMem(0x4000) & 0b1111;
	SC1constantVolFlag = (readFromMem(0x4000) & 0x10) == 0x10;
}

//	reloads the length counter for SC2, with all the other according settings
void resetSC2length(uint8_t val) {
	SC2len = length_table[val >> 3];
	SC2enabled = SC2len > 0;
	SC2timerTarget = ((readFromMem(0x4007) & 0b111) << 8) | readFromMem(0x4006);
	SC2timer = 0;
	SC2dutyIndex = 0;
}

void resetSC2Envelope() {
	SC2envelopeStart = true;
}

void resetSC2Sweep() {
	SC2sweepReload = true;
	SC2sweepEnabled = (readFromMem(0x4005) & 0x80) == 0x80;
}

void resetSC2hi() {
	SC2timerTarget = ((readFromMem(0x4007) & 0b111) << 8) | readFromMem(0x4006);
}

//	reloads the length counter for SC3, with all the other according settings
void resetSC3length(uint8_t val) {
	SC3len = length_table[val >> 3];
	SC3timerTarget = ((readFromMem(0x400b) & 0b111) << 8) | readFromMem(0x400a);
	SC3linearReload = true;
	SC3enabled = SC3len > 0;
}

void resetSC3hi() {
	SC3timerTarget = ((readFromMem(0x400b) & 0b111) << 8) | readFromMem(0x400a);
}

void resetSC3linearReload() {
	SC3controlFlag = (readFromMem(0x4008) & 0x80) == 0x80;
	SC3linearReloadValue = readFromMem(0x4008) & 0x7f;
}

//	reloads the length counter for SC4, with all the other according settings
void resetSC4length(uint8_t val) {
	SC4len = length_table[val >> 3];
	SC4enabled = true;
}

void resetSC4hi() {
	SC4timerTarget = noise_ntsc[(readFromMem(0x400e) & 0b1111)];
	SC4modeFlag = (readFromMem(0x400e) & 0x80) == 0x80;
}

void resetSC4Ctrl() {
	SC4constantVol = readFromMem(0x400c) & 0b1111;
	SC4constantVolFlag = (readFromMem(0x400c) & 0x10) == 0x10;
}

void resetChannelEnables() {
	if ((readFromMem(0x4015) & 0b1) == 0) {
		SC1len = 0;
	}
	if ((readFromMem(0x4015) & 0b10) == 0) {
		SC2len = 0;
	}
	if ((readFromMem(0x4015) & 0b100) == 0) {
		SC3len = 0;
	}
	if ((readFromMem(0x4015) & 0b1000) == 0) {
		SC3len = 0;
	}
}
