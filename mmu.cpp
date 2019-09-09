#define _CRT_SECURE_NO_DEPRECATE
#include <stdint.h>
#include <stdio.h>
#include "cpu.h"
#include "mmu.h"
#include "ppu.h"
#include "input.h"
#include "mapper.h"
#include "main.h"

bool pbc = false;
bool open_ppuaddr = false;
ROM_TYPE romType;
int poll_input = -1;
VRAM_MIRRORING vramMirroring = VRAM_MIRRORING::NONE;

Mapper *mapper;

void powerUp() {
	//	TODO
}

void reset() {
	
}

void idROM(unsigned char c[]) {
	
	//	iNES 1.0 - $4E $45 $53 $1A
	if (c[0] == 0x4e && c[1] == 0x45 && c[2] == 0x53 && c[3] == 0x1a) {
		
		printf("iNES 1.0 Header detected\n");
		
		//	mapper ID
		switch ((c[7] & 0xf0) | (c[6] >> 4))
		{
			//	NROM
			case 0:
				printf("NROM cartridge!\n");
				mapper = new NROM(0x10000, c[4], 0);
				break;
			//	UNROM
			case 2:
				printf("UnROM cartridge!\n");
				mapper = new UNROM(0x400000, c[4], 0);
				break;
			default:
				printf("Unhandled Mapper %d\n", (c[7] & 0xf0) | (c[6] >> 4));
				break;
		}

		//	mirroring
		if (c[6] & 1)
			vramMirroring = VRAM_MIRRORING::VERTICAL;
		else
			vramMirroring = VRAM_MIRRORING::HORIZONTAL;

		initVRAM(vramMirroring);
	}
}

VRAM_MIRRORING getMirroring() {
	return vramMirroring;
}

//	copy cartridge to memory
void loadROM(string filename) {

	setPause();

	unsigned char cartridge[0xf0000];
	FILE* file = fopen(filename.c_str(), "rb");
	int pos = 0;
	while (fread(&cartridge[pos], 1, 1, file)) {
		pos++;
	}
	fclose(file);

	idROM(cartridge);

	mapper->loadMem(cartridge);

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
		case 0x4016:		//	CONTROLLER #1
			if (poll_input >= 0) {
				uint8_t ret = readController1(poll_input++);
				if (poll_input > 7)
					poll_input = -1;	//	disable polling
				return ret | 0x40;
			}
			return 0x40;
			break;
		default:
			return mapper->read(adr);
			break;
	}
}

void writeToMem(uint16_t adr, uint8_t val) {

	switch (adr) {
		case 0x2000:		//	PPUCTRL
			writePPUCTRL(val);
			break;
		case 0x2001:		//	PPUMASK
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
			writePPUADDR(val);
			break;
		case 0x2007:		//	PPUDATA
			writePPUDATA(val);
			break;
		case 0x4014:		//	OAM DMA
			oamDMAtransfer(val, mapper->memory);
			break;
		case 0x4016:		//	enable / disable input polling
			poll_input = val;
			break;
		default:
			mapper->write(adr, val);
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
	return (mapper->read(adr + 1) << 8) | mapper->read(adr);
}
uint16_t getAbsoluteXIndex(uint16_t adr, uint8_t X) {
	a = ((mapper->read(adr + 1) << 8) | mapper->read(adr));
	pbc = (a & 0xff00) != ((a + X) & 0xff00);
	return (a + X) % 0x10000;
}
uint16_t getAbsoluteYIndex(uint16_t adr, uint8_t Y) {
	a = ((mapper->read(adr + 1) << 8) | mapper->read(adr));
	pbc = (a & 0xff00) != ((a + Y) & 0xff00);
	return (a + Y) % 0x10000;
}
uint16_t getZeropage(uint16_t adr) {
	return mapper->read(adr);
}
uint16_t getZeropageXIndex(uint16_t adr, uint8_t X) {
	return (mapper->read(adr) + X) % 0x100;
}
uint16_t getZeropageYIndex(uint16_t adr, uint8_t Y) {
	return (mapper->read(adr) + Y) % 0x100;
}
uint16_t getIndirect(uint16_t adr) {
	return (mapper->read(mapper->read(adr + 1) << 8 | ((mapper->read(adr) + 1) % 0x100))) << 8 | mapper->read(mapper->read(adr + 1) << 8 | mapper->read(adr));
}
uint16_t getIndirectXIndex(uint16_t adr, uint8_t X) {
	a = (mapper->read((mapper->read(adr) + X + 1) % 0x100) << 8) | mapper->read((mapper->read(adr) + X) % 0x100);
	return a % 0x10000;
}
uint16_t getIndirectYIndex(uint16_t adr, uint8_t Y) {
	a = ((mapper->read((mapper->read(adr) + 1) % 0x100) << 8) | (mapper->read(mapper->read(adr))));
	pbc = (a & 0xff00) != ((a + Y) & 0xff00);
	return (a + Y) % 0x10000;
}