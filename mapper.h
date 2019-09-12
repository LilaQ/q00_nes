#pragma once
#include <stdint.h>
struct Mapper {
	unsigned char *memory;
	int romPRG16ks;
	int romCHR8ks;

	Mapper(int mem_size, int prg16, int chr8) {
		memory = new unsigned char[mem_size];
		romPRG16ks = prg16;
		romCHR8ks = chr8;
	}

	~Mapper() {
		delete memory;
		delete &romPRG16ks;
		delete &romCHR8ks;
	}

	virtual uint8_t read(uint16_t adr) {
		return memory[adr];
	}

	virtual void write(uint16_t adr, uint8_t val) {
		memory[adr] = val;
	}
	virtual void loadMem(unsigned char* c) = 0;
};

struct NROM : Mapper {

	NROM(int mem_size, int prg16, int chr8) : Mapper(mem_size, prg16, chr8) {

	}

	virtual void loadMem(unsigned char *c) {
		for (int i = 0; i < 0x4000; i++) {
			//	First 16k PRG
			memory[0x8000 + i] = c[i+0x10];
		}
		for (int i = 0; i < 0x4000; i++) {
			memory[0xc000 + i] = c[i + 0x10 + (romPRG16ks - 1) * 0x4000];
		}
		for (int i = 0; i < 0x2000; i++) {
			writeCHRRAM(c, 0x10 + romPRG16ks * 0x4000);
		}
	}

};

struct UNROM : Mapper {

	int PRGid = 0;
	unsigned char* m = 0;

	UNROM(int mem_size, int prg16, int chr8) : Mapper(mem_size, prg16, chr8) {

	}

	virtual void loadMem(unsigned char* c) {
		m = c;
		//	Switchable first 16k PRG
		for (int i = 0; i < 0x4000; i++) {
			memory[0x8000 + i] = c[i + 0x10];
		}
		//	Fixed last 16k PRG
		for (int i = 0; i < 0x4000; i++) {
			memory[0xc000 + i] = c[i + 0x10 + (romPRG16ks - 1) * 0x4000];
		}
		for (int i = 0; i < 0x2000; i++) {
			//	NROM
			writeCHRRAM(c, 0x10 + romPRG16ks * 0x4000);
		}
	}

	virtual void write(uint16_t adr, uint8_t val) {
		if (adr >= 0x8000 && adr <= 0xffff) {
			PRGid = val & 0b111;
		}
		else {
			memory[adr] = val;
		}
	}

	virtual unsigned char read(uint16_t adr) {
		if (adr >= 0x8000 && adr <= 0xbfff) {
			return m[(adr % 0x8000) + PRGid * 0x4000 + 0x10];
		}
		else {
			return memory[adr];
		}
	}

};

struct MMC1 : Mapper {

	int PRGid = 0;
	unsigned char* m = 0;

	MMC1(int mem_size, int prg16, int chr8) : Mapper(mem_size, prg16, chr8) {

	}

	virtual void loadMem(unsigned char* c) {
		m = c;
		//	Switchable first 16k PRG
		for (int i = 0; i < 0x4000; i++) {
			memory[0x8000 + i] = c[i + 0x10];
		}
		//	Fixed last 16k PRG
		for (int i = 0; i < 0x4000; i++) {
			memory[0xc000 + i] = c[i + 0x10 + (romPRG16ks - 1) * 0x4000];
		}
		for (int i = 0; i < 0x2000; i++) {
			//	NROM
			writeCHRRAM(c, 0x10 + romPRG16ks * 0x4000);
		}
	}

	virtual void write(uint16_t adr, uint8_t val) {
		if (adr >= 0x8000 && adr <= 0xffff) {
			PRGid = val & 0b111;
		}
		else {
			memory[adr] = val;
		}
	}

	virtual unsigned char read(uint16_t adr) {
		if (adr >= 0x8000 && adr <= 0xbfff) {
			return m[(adr % 0x8000) + PRGid * 0x4000 + 0x10];
		}
		else {
			return memory[adr];
		}
	}

};