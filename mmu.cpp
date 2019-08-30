#define _CRT_SECURE_NO_DEPRECATE
#include <stdint.h>
#include <stdio.h>
#include "cpu.h"
#include "mmu.h"
#include "ppu.h"
#include "main.h"

unsigned char memory[0x10000];
bool pbc = false;
bool open_ppuaddr = false;
ROM_TYPE romType;
int romPRG16ks = 0;
int romCHR8ks = 0;
VRAM_MIRRORING vramMirroring = VRAM_MIRRORING::NONE;

void powerUp() {
	//	TODO
}

void reset() {
	
}

void idROM(unsigned char c[]) {
	//	iNES 1.0 - $4E $45 $53 $1A
	if (c[0] == 0x4e && c[1] == 0x45 && c[2] == 0x53 && c[3] == 0x1a) {
		printf("iNES 1.0 Header detected\n");
		if (c[4] == 1) {
			romType = ROM_TYPE::iNES_16;
			romPRG16ks = 1;
		}
		else if (c[4] == 2) {
			romType = ROM_TYPE::iNES_32;
			romPRG16ks = 2;
		}
		else
			printf("Unhandled ROM-Type with %d kb of PRG ROM\n", c[4] * 16);

		//	mirroring
		if ((c[6] >> 3) & 1) {
			if (c[6] & 1)
				vramMirroring = VRAM_MIRRORING::HORIZONTAL;
			else
				vramMirroring = VRAM_MIRRORING::VERTICAL;
		}
	}
}

VRAM_MIRRORING getMirroring() {
	return vramMirroring;
}

//	copy cartridge to memory
void loadROM(string filename) {

	setPause();

	unsigned char cartridge[0x10000];
	FILE* file = fopen(filename.c_str(), "rb");
	int pos = 0;
	while (fread(&cartridge[pos], 1, 1, file)) {
		pos++;
	}
	fclose(file);

	idROM(cartridge);

	for (int i = 0; i < 0x4000; i++) {
		//	First 16k PRG
		memory[0x8000 + i] = cartridge[i+0x10];
	}
	for (int i = 0; i < 0x4000; i++) {
			memory[0xc000 + i] = cartridge[i + 0x10 + (romPRG16ks - 1) * 0x4000];
	}
	for (int i = 0; i < 0x2000; i++) {
		//	NROM
		writeCHRRAM(cartridge, 0x10 + romPRG16ks * 0x4000);
	}

	resetCPU();

	resetPause();
}

uint8_t readFromMem(uint16_t adr) {
	switch (adr)
	{
		case 0x2002:		//	PPUSTATUS
			return readPPUSTATUS();
			break;
		case 0x2007:		//	PPUDATA
			return readPPUDATA();
			break;
		case 0x4014:		//	OAM DMA
			return readOAMDATA();
			break;
		default:
			return memory[adr];
			break;
	}
}

void writeToMem(uint16_t adr, uint8_t val) {

	switch (adr) {
		case 0x2000:		//	PPUCTRL
			writePPUCTRL(val);
			break;
		case 0x2001:		//	PPUMASK
			printf("Writing PPUMASK: %x\n", val);
			writePPUMASK(val);
			break;
		case 0x2003:		//	OAMADDR
			writeOAMADDR(val);
			break;
		case 0x2004:		//	OAMDATA
			writeOAMDATA(val);
			break;
		case 0x2005:		//	PPUSCROLL
			writePPUSCROLL(val);
			break;
		case 0x2006:		//	PPUADDR
			if (!open_ppuaddr) {
				writePPUADDR(val, 0);
				open_ppuaddr = true;
			}
			else {
				writePPUADDR(val, 1);
				open_ppuaddr = false;
			}
			break;
		case 0x2007:		//	PPUDATA
			writePPUDATA(val);
			break;
		case 0x4014:		//	OAM DMA
			oamDMAtransfer(val, memory);
			break;
		default:
			memory[adr] = val;
			break;
	}
	
}

bool pageBoundaryCrossed() {
	return pbc;
}

//	addressing modes
uint16_t a;
uint16_t getImmediate(uint16_t adr) {
	return adr;
}
uint16_t getAbsolute(uint16_t adr) {
	return (memory[adr + 1] << 8) | memory[adr];
}
uint16_t getAbsoluteXIndex(uint16_t adr, uint8_t X) {
	a = ((memory[adr + 1] << 8) | memory[adr]);
	pbc = (a & 0xff00) != ((a + X) & 0xff00);
	return (a + X) % 0x10000;
}
uint16_t getAbsoluteYIndex(uint16_t adr, uint8_t Y) {
	a = ((memory[adr + 1] << 8) | memory[adr]);
	pbc = (a & 0xff00) != ((a + Y) & 0xff00);
	return (a + Y) % 0x10000;
}
uint16_t getZeropage(uint16_t adr) {
	return memory[adr];
}
uint16_t getZeropageXIndex(uint16_t adr, uint8_t X) {
	return (memory[adr] + X) % 0x100;
}
uint16_t getZeropageYIndex(uint16_t adr, uint8_t Y) {
	return (memory[adr] + Y) % 0x100;
}
uint16_t getIndirect(uint16_t adr) {
	return (memory[memory[adr + 1] << 8 | ((memory[adr] + 1) % 0x100)]) << 8 | memory[memory[adr + 1] << 8 | memory[adr]];
}
uint16_t getIndirectXIndex(uint16_t adr, uint8_t X) {
	a = (memory[(memory[adr] + X + 1) % 0x100] << 8) | memory[(memory[adr] + X) % 0x100];
	return a % 0x10000;
}
uint16_t getIndirectYIndex(uint16_t adr, uint8_t Y) {
	a = ((memory[(memory[adr] + 1) % 0x100] << 8) | (memory[memory[adr]]));
	pbc = (a & 0xff00) != ((a + Y) & 0xff00);
	return (a + Y) % 0x10000;
}