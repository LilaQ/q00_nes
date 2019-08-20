#include <stdint.h>
#include "mmu.h"
#include "ppu.h"

unsigned char memory[0x10000];
bool pbc = false;
bool open_ppuaddr = false;

void powerUp() {
	//	TODO
}

void reset() {
	
}

//	copy cartridge to memory
void loadROM(unsigned char c[]) {
	for (int i = 0; i < 0x10000; i++) {
		//	nestest
		memory[0xc000 + i] = c[i];
		memory[0x8000 + i] = c[i];
	}
}

uint8_t readFromMem(uint16_t adr) {
	switch (adr)
	{
		case 0x2002:		//	PPUSTATUS
			return readPPUSTATUS();
			break;
		default:
			return memory[adr];
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
	return (memory[memory[adr + 1] << 8 | ((memory[adr] + 1) % 0x100)] ) << 8 | memory[memory[adr + 1] << 8 | memory[adr]];
}
uint16_t getIndirectXIndex(uint16_t adr, uint8_t X) {
  	a = (memory[(memory[adr] + X + 1) % 0x100] << 8) | memory[(memory[adr] + X) % 0x100];
	return a % 0x10000;
}
uint16_t getIndirectYIndex(uint16_t adr, uint8_t Y) {
	a = ((memory[(memory[adr] + 1) % 0x100] << 8) | (memory[memory[adr]]));
	pbc = (a & 0xff00) != ((a+Y) & 0xff00);
 	return (a+Y) % 0x10000;
}

void writeToMem(uint16_t adr, uint8_t val) {

	switch (adr) {
		case 0x2000:
			writePPUCTRL(val);
			break;
		case 0x2006:		//	PPUADDR
			if (!open_ppuaddr) {
				writePPUADDR(val, 0);
				open_ppuaddr = true;
			}
			else {
				writePPUADDR(val, 0);
				open_ppuaddr = false;
			}
			break;
		case 0x2007:		//	PPUDATA
			writePPUDATA(val);
			break;
		default:
			memory[adr] = val;
			break;
	}
	
}