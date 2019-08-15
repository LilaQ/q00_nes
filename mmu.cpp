#include <stdint.h>
#include "mmu.h"

unsigned char memory[0x10000];


void powerUp() {
	//	TODO
}

void reset() {
	
}

//	copy cartridge to memory
void loadROM(unsigned char c[]) {
	for (int i = 0; i < 0x4000; i++) {
		memory[0xc000 + i] = c[i+0x10];
		memory[0x8000 + i] = c[i+0x10];
	}
}

uint8_t readFromMem(uint16_t adr) {
	return memory[adr];
}

//	addressing modes
uint16_t getImmediate(uint16_t adr) {
	return adr;
}
uint16_t getAbsolute(uint16_t adr) {
	return (memory[adr + 1] << 8) | memory[adr];
}
uint16_t getAbsoluteXIndex(uint16_t adr, uint8_t X) {
	return ((memory[adr + 1] << 8) | memory[adr]) + X;
}
uint16_t getAbsoluteYIndex(uint16_t adr, uint8_t Y) {
	return ((memory[adr + 1] << 8) | memory[adr]) + Y;
}
uint16_t getZeropage(uint16_t adr) {
	return memory[adr];
}
uint16_t getZeropageXIndex(uint16_t adr, uint8_t X) {
	return memory[adr + X];
}
uint16_t getZeropageYIndex(uint16_t adr, uint8_t Y) {
	return memory[adr + Y];
}
uint16_t getIndirect(uint16_t adr) {
	return (memory[adr + 1] << 8) | memory[adr];
}
uint16_t getIndirectXIndex(uint16_t adr, uint8_t X) {
	return (memory[adr + X + 1] << 8) | memory[adr + X];
}
uint16_t getIndirectYIndex(uint16_t adr, uint8_t Y) {
	return ((memory[adr + 1] << 8) | memory[adr]) + Y;
}

void writeToMem(uint16_t adr, uint8_t val) {
	memory[adr] = val;
}