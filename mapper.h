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
			writeCHRRAM(c, 0x10 + romPRG16ks * 0x4000, 0x2000);
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
			writeCHRRAM(c, 0x10 + romPRG16ks * 0x4000, 0x2000);
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

	uint8_t shift_register = 0b10000;
	uint8_t sr_write_counter = 0;

	int PRGid = 0;
	int CHRid = 0;
	uint16_t PRGsize = 0x4000;	//	32k
	uint16_t CHRsize = 0x1000;	//	4k
	uint16_t PRGswitchAddr = 0x8000;

	unsigned char* m = 0;

	MMC1(int mem_size, int prg16, int chr8) : Mapper(mem_size, prg16, chr8) {
		printf("PRG banks: %d CHR banks: %d\n", romPRG16ks, romCHR8ks);
		PRGid = romPRG16ks;
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
		//	CHR 
		if (romCHR8ks) {
			for (int i = 0; i < 0x2000; i++) {
				writeCHRRAM(c, 0x10 + romPRG16ks * 0x4000, 0x2000);
			}
		}
	}

	virtual void write(uint16_t adr, uint8_t val) {

		if (adr >= 0x8000 && adr <= 0xffff) {
			//	resetting shift register
			if (val & 0b10000000) {
				shift_register = 0b10000;
				sr_write_counter = 0;
				return;
			}
			//	shifting 4 times, writing on the 5th
			else {
				sr_write_counter++;
				shift_register = ((shift_register >> 1) | ((val & 1) << 4)) & 0b11111;
				if (sr_write_counter == 5) {

					//	Control
					if (adr >= 0x8000 && adr <= 0x9fff) {

						//	Mirroring
						//	TODO
						if ((shift_register & 0b11) == 0) {
							//	One screen, lower bank
						}
						else if ((shift_register & 0b11) == 1) {
							//	One screen, upper bank
						}
						else if ((shift_register & 0b11) == 2) {
							//	Vertical
						}
						else if ((shift_register & 0b11) == 3) {
							//	Horizontal
						}

						//	PRG size
						//	32k switch
						if ((shift_register >> 2 & 0b11) == 1 || (shift_register >> 2 & 0b11) == 0) {
							PRGsize = 0x8000;
							PRGswitchAddr = 0x8000;
							PRGid = PRGid / 2;
						}
						//	fix first 16k at 0x8000, switch 16k at 0xc000
						else if ((shift_register >> 2 & 0b11) == 2) {
							PRGsize = 0x4000;
							PRGswitchAddr = 0xc000;
						}
						//	fix last 16 at 0xc000, switch 16 at 0x8000
						else if ((shift_register >> 2 & 0b11) == 3) {
							PRGsize = 0x4000;
							PRGswitchAddr = 0x8000;
						}

						//	CHR size
						CHRsize = ((shift_register >> 4) & 1) ? 0x1000 : 0x2000;

					}
					//	CHR bank 0
					else if (adr >= 0xa000 && adr <= 0xbfff) {
						CHRid = (shift_register & 0b11111);
						if (CHRsize == 0x2000)
							CHRid = (shift_register & 0b11110) >> 1;
						if (romCHR8ks)
							writeCHRRAM(m, 0x10 + romPRG16ks * 0x4000 + CHRid * CHRsize, CHRsize);
					}
					//	CHR bank 1
					else if (adr >= 0xc000 && adr <= 0xdfff && CHRsize == 0x1000) {
						CHRid = (shift_register & 0b11111);
						if (romCHR8ks)
							writeCHRRAM(m, 0x10 + romPRG16ks * 0x4000 + CHRid * CHRsize + 0x1000, CHRsize);
					}
					//	PRG bank
					else if (adr >= 0xe000 && adr <= 0xffff) {
						PRGid = (shift_register & 0b1111);
						if (PRGsize == 0x8000)
							PRGid = shift_register >> 1;
					}

					shift_register = 0b10000;
					sr_write_counter = 0;
				}
			}
		}
		else {
			memory[adr] = val;
		}
	}

	virtual unsigned char read(uint16_t adr) {
		if (PRGsize == 0x4000) {
			//	First PRG area
			if (adr >= 0x8000 && adr <= 0xbfff) {
				//	this area is fixed to the first PRG bank
				if (PRGswitchAddr == 0xc000) {
					return m[(adr % 0x8000) + 0x10];
				}
				//	this area is switchable
				else {
					return m[(adr % 0x8000) + PRGid * PRGsize + 0x10];
				}
			}
			//	Second PRG area
			else if (adr >= 0xc000 & adr <= 0xffff) {
				//	this area is fixed to the last PRG bank
				if (PRGswitchAddr == 0x8000) {
					return m[(adr % 0xc000) + 0x10 + (romPRG16ks - 1) * PRGsize];
				}
				//	this area is switchable
				else {
					return m[(adr % 0x8000) + PRGid * PRGsize + 0x10];
				}
			}
			else {
				return memory[adr];
			}
		}
		//	32k PRG banks
		else if (PRGsize == 0x8000) {
			if (adr >= 0x8000 && adr <= 0xffff) {
				printf(" %d ", PRGid);
				return m[(adr % 0x8000) + 0x10 + PRGid * PRGsize];
			}
			else {
				return memory[adr];
			}
		}
	}
};

struct MMC3 : Mapper {

	int PRGid = 0;
	unsigned char* m = 0;

	MMC3(int mem_size, int prg16, int chr8) : Mapper(mem_size, prg16, chr8) {

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
			writeCHRRAM(c, 0x10 + romPRG16ks * 0x4000, 0x2000);
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