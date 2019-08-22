#define _CRT_SECURE_NO_DEPRECATE
#include <string>
#include "mmu.h"
#include "cpu.h"
#include "ppu.h"
#include <iostream>
#include <cstdint>
using namespace::std;
//	[q00.nes]
//
//	CPU speed: 1.662607 MHz (~601 ns per cycle)	/ 1.773447Mhz for PAL (???)
//	
//	$0000 - $07FF	$0800	2KB internal RAM
//	$0800 - $0FFF	$0800	Mirrors of $0000 - $07FF
//	$1000 - $17FF	$0800	Mirrors of $0000 - $07FF
//	$1800 - $1FFF	$0800	Mirrors of $0000 - $07FF
//	$2000 - $2007	$0008	NES PPU registers
//	$2008 - $3FFF	$1FF8	Mirrors of $2000 - 2007 (repeats every 8 bytes)
//	$4000 - $4017	$0018	NES APU and I / O registers
//	$4018 - $401F	$0008	APU and I / O functionality that is normally disabled.See CPU Test Mode.
//	$4020 - $FFFF	$BFE0	Cartridge space : PRG ROM, PRG RAM, and mapper registers(See Note)

unsigned char cartridge[0x10000];
string filename = "dk.nes";

int lastcyc = 0;
int ppus = 0;

int main()
{

	//	load cartridge
	FILE* file = fopen(filename.c_str(), "rb");
	int pos = 0;
	while (fread(&cartridge[pos], 1, 1, file)) {
		pos++;
	}
	fclose(file);
	loadROM(cartridge);

	initPPU();

	resetCPU();

	while (1) {
		lastcyc = stepCPU();
		ppus = lastcyc * 3;
		while (ppus--) {
			stepPPU();
		}

	}

	return 1;
}

int getLastCyc() {
	return lastcyc;
}