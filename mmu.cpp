#define _CRT_SECURE_NO_DEPRECATE
#include <stdint.h>
#include <stdio.h>
#include "cpu.h"
#include "mmu.h"
#include "ppu.h"
#include "apu.h"
#include "input.h"
#include "mapper.h"
#include "main.h"

bool pbc = false;
bool open_ppuaddr = false;
ROM_TYPE romType;
int poll_input_1 = -1;
int poll_input_2 = -1;
VRAM_MIRRORING vramMirroring = VRAM_MIRRORING::NONE;

Mapper *mapper;

void powerUp() {
	//	TODO
}

void reset() {
	
}

bool irq() {
	return mapper->IRQ();
}

void nextScanline() {
	mapper->nextScanline();
}

void idROM(unsigned char c[]) {
	
	//	iNES 1.0 - $4E $45 $53 $1A
	if (c[0] == 0x4e && c[1] == 0x45 && c[2] == 0x53 && c[3] == 0x1a) {
		
		printf("iNES 1.0 Header detected\n");
		
		//	mapper ID
		//	TODO, most mappers here pass a zero, instead of byte 5 for CHR size
		switch ((c[7] & 0xf0) | (c[6] >> 4))
		{
			//	NROM
			case 0:
				printf("NROM cartridge!\n");
				mapper = new NROM(0x10000, c[4], 0);
				break;
			//	MMC1
			case 1:
				printf("MMC1 cartridge!\n");
				mapper = new MMC1(0x400000, c[4], 0);
				break;
			//	UNROM
			case 2:
				printf("UnROM cartridge!\n");
				mapper = new UNROM(0x400000, c[4], 0);
				break;
			//	MMC3
			case 4:
				printf("MMC3 cartridge!\n");
				mapper = new MMC3(0x400000, c[4], c[5], false);
				break;
			//	MMC3 / Mapper 37 (SMB + Tetris + World Cup)
			case 37:
				printf("MMC3 cartridge!\n");
				mapper = new MMC3(0x400000, c[4], c[5], true);
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
			if (poll_input_1 >= 0) {
				uint8_t ret = readController1(poll_input_1++);
				if (poll_input_1 > 7)
					poll_input_1 = -1;	//	disable polling
				return ret | 0x40;
			}
			return 0x40;
			break;
		case 0x4017:		//	CONTROLLER #2
			if (poll_input_2 >= 0) {
				uint8_t ret = readController2(poll_input_2++);
				if (poll_input_2 > 7)
					poll_input_2 = -1;	//	disable polling
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
		case 0x4000:
			mapper->write(adr, val);
			resetSC1Ctrl();
			break;
		case 0x4001:		//	SC1 SWEEP
			mapper->write(adr, val);
			resetSC1Sweep();
			break;
		case 0x4002:
			mapper->write(adr, val);
			resetSC1hi();
			break;
		case 0x4003:		//	SC1 LENGTH, SC1 ENVELOPE
			mapper->write(adr, val);
			resetSC1length(val);
			resetSC1Envelope();
			break;
		case 0x4005:		//	SC2 SWEEP
			mapper->write(adr, val);
			resetSC2Sweep();
			break;
		case 0x4006:
			mapper->write(adr, val);
			resetSC2hi();
			break;
		case 0x4007:		//	SC2 LENGTH, SC2 ENVELOPE
			mapper->write(adr, val);
			resetSC2length(val);
			resetSC2Envelope();
			break;
		case 0x4008:		//	SC3 LINEAR RELOAD
			mapper->write(adr, val);
			resetSC3linearReload();
			break;
		case 0x400a:		//	SC3 LENGTH HI-BITS
			mapper->write(adr, val);
			resetSC3hi();
			break;
		case 0x400b:		//	SC3 LENGTH
			mapper->write(adr, val);
			resetSC3length(val);
			break;
		case 0x400c:		//	SC4 CTRL
			mapper->write(adr, val);
			resetSC4Ctrl();
			break;
		case 0x400e:		//	SC4 HI
			mapper->write(adr, val);
			resetSC4hi();
			break;
		case 0x400f:		//	SC4 LENGTH
			mapper->write(adr, val);
			resetSC4length(val);
			break;
		case 0x4014:		//	OAM DMA
			oamDMAtransfer(val, mapper->memory);
			break;
		case 0x4015:		//	SC1/2/3/4 Enable / Disable
			mapper->write(adr, val);
			resetChannelEnables();
			break;
		case 0x4016:		//	enable / disable input polling
			poll_input_1 = val;
			poll_input_2 = val;
			break;
		default:
			mapper->write(adr, val);
			break;
	}
	
}

void shadowWriteToMem(uint16_t adr, uint8_t val) {
	mapper->write(adr, val);
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