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
		printf("PRG banks: %d \tCHR banks: %d\n", romPRG16ks, romCHR8ks);
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

	uint8_t bank_register = 0;
	uint8_t rom_bank_mode = 0;
	uint8_t chr_a12_inversion = 0;

	bool is_mapper_37 = false;
	uint32_t m37_mask = 0x00000;

	bool irq_enabled = true;
	uint16_t irq_reload_value = 0x0;
	uint16_t irq_scanline_counter = 0x0;

	uint8_t prg_chr_bank[8] = {
		0, 0, 0, 0, 0, 0, 0, 1
		// 0, 1, -2, -1
	};

	MMC3(int mem_size, int prg16, int chr8, bool m37) : Mapper(mem_size, prg16, chr8) {
		romPRG16ks = prg16 * 2;
		if (m37) {
			romPRG16ks /= 4;
			is_mapper_37 = m37;

		}
		initVRAM(VRAM_MIRRORING::HORIZONTAL);
	}

	virtual void loadMem(unsigned char* c) {
		printf("Loading Mem... PRG (8k): %d, CHR (8k): %d\n", romPRG16ks, romCHR8ks);
		m = c;
		update();
	}

	virtual void write(uint16_t adr, uint8_t val) {

		//	Registers
		if (adr >= 0x8000 && adr <= 0x9fff) {

			//	Bank select | even
			if ((adr % 2) == 0) {
				bank_register = val & 0b111;
				rom_bank_mode = (val >> 6) & 1;
				chr_a12_inversion = (val >> 7) & 1;
				update();
			}

			//	Bank data | odd
			else {
				prg_chr_bank[bank_register] = val;
				update();
  				//printf("Setting register %d to %d\n", bank_register, val);
			}

		}
		else if (adr >= 0x6000 && adr <= 0x7fff) {
			if (is_mapper_37) {
				/*
				0,1,2	$00000-$0FFFF (64kB)	$00000-$1FFFF
					3	$10000-$1FFFF (64kB)	$00000-$1FFFF
				4,5,6	$20000-$3FFFF (128kB)	$20000-$3FFFF
					7	$30000-$3FFFF (64kB)	$20000-$3FFFF*/
				if ((val & 0b111) <= 2 && (val & 0b111) >= 0) {
					m37_mask = 0x00000;
				}
				else if ((val & 0b111) == 3) {
					m37_mask = 0x10000;
				}
				else if ((val & 0b111) <= 6 && (val & 0b111) >= 4) {
					m37_mask = 0x20000;
				}
				else if ((val & 0b111) == 7) {
					m37_mask = 0x30000;
				}
				//printf("Setting outer bank to #%x\n", (val & 0b111));
				update();
			}
			else {
				memory[adr] = val;
			}
		}
		else if (adr >= 0xa000 && adr <= 0xbfff) {
			//	mirroring | even
			if ((adr % 2) == 0) {
				if ((val & 1) == 0) {
					initVRAM(VRAM_MIRRORING::VERTICAL);
				}
				else {
					initVRAM(VRAM_MIRRORING::HORIZONTAL);
				}
			}
		}
		else if (adr >= 0xc000 && adr <= 0xdfff) {
			//	set reload value | even
			if ((adr % 2) == 0) {
				irq_reload_value = val;
			}
			//	actually set reload value to scanline counter | odd
			else {
				irq_scanline_counter = irq_reload_value;
			}
		}
		else if (adr >= 0xe000 && adr <= 0xffff) {
			//	IRQ disable | even
			if ((adr % 2) == 0) {
				irq_enabled = false;
			}
			//	IRQ enable | odd
			else {
				irq_enabled = true;
			}
		}
		else {
			memory[adr] = val;
		}
	}

	virtual unsigned char read(uint16_t adr) {
		
		/*if (adr >= 0xe000 && adr <= 0xffff) {
			return m[(adr % 0x8000) + PRGid * 0x4000 + 0x10];
		}
		else {
			return memory[adr];
		}*/
		return memory[adr];
	}

	void update() {

		/*
			PRG
		*/

		uint32_t p0, p1, p2, p3;
		if (rom_bank_mode == 0) {
			p0 = prg_chr_bank[6] * 0x2000;
			p1 = prg_chr_bank[7] * 0x2000;
			p2 = (romPRG16ks - 2) * 0x2000;
			p3 = (romPRG16ks - 1) * 0x2000;
		}
		else {
			p0 = (romPRG16ks - 2) * 0x2000;
			p1 = prg_chr_bank[7] * 0x2000;
			p2 = prg_chr_bank[6] * 0x2000;
			p3 = (romPRG16ks - 1) * 0x2000;
		}

		//	CPU $8000 - $9FFF(or $C000 - $DFFF) : 8 KB switchable PRG ROM bank
		for (int i = 0; i < 0x2000; i++) {
			memory[0x8000 + i] = m[i + m37_mask + 0x10 + p0];
		}
		//printf("Setting page 0x8000-0x9fff to ROM address %x with bank id #%d\n", p0, prg_chr_bank[6]);
		//	CPU $A000-$BFFF: 8 KB switchable PRG ROM bank
		for (int i = 0; i < 0x2000; i++) {
			memory[0xa000 + i] = m[i + m37_mask + 0x10 + p1];
		}
		//	CPU $C000-$DFFF (or $8000-$9FFF): 8 KB PRG ROM bank, fixed to the second-last bank
		for (int i = 0; i < 0x2000; i++) {
			memory[0xc000 + i] = m[i + m37_mask + 0x10 + p2];
		}
		//	Fixed last 8k PRG
		for (int i = 0; i < 0x2000; i++) {
			memory[0xe000 + i] = m[i + m37_mask + 0x10 + p3];
		}

		/*
			CHR
		*/
		if (chr_a12_inversion == 0) {
			writeCHRRAM(m, (0x10 + (prg_chr_bank[0] & 0xffffe) * 0x400 + 0x40000) | m37_mask, 0x0800, 0x0000);	//	R0 to 0x0000 - 0x07ff (2 banks)
			writeCHRRAM(m, (0x10 + (prg_chr_bank[1] & 0xffffe) * 0x400 + 0x40000) | m37_mask, 0x0800, 0x0800);	//	R1 to 0x0800 - 0x0fff (2 banks)
			writeCHRRAM(m, (0x10 + prg_chr_bank[2] * 0x400 + 0x40000) | m37_mask, 0x0400, 0x1000);	//	R2 to 0x1000 - 0x13ff (1 bank)
			writeCHRRAM(m, (0x10 + prg_chr_bank[3] * 0x400 + 0x40000) | m37_mask, 0x0400, 0x1400);	//	R3 to 0x1400 - 0x17ff (1 bank)
			writeCHRRAM(m, (0x10 + prg_chr_bank[4] * 0x400 + 0x40000) | m37_mask, 0x0400, 0x1800);	//	R4 to 0x1800 - 0x1bff (1 bank)
			writeCHRRAM(m, (0x10 + prg_chr_bank[5] * 0x400 + 0x40000) | m37_mask, 0x0400, 0x1c00);	//	R5 to 0x1c00 - 0x1fff (1 bank)
		}
		else {
			writeCHRRAM(m, (0x10 + prg_chr_bank[2] * 0x400 + 0x40000) | m37_mask, 0x0400, 0x0000);	//	R2 to 0x0000 - 0x03ff (1 bank)
			writeCHRRAM(m, (0x10 + prg_chr_bank[3] * 0x400 + 0x40000) | m37_mask, 0x0400, 0x0400);	//	R3 to 0x0400 - 0x07ff (1 bank)
			writeCHRRAM(m, (0x10 + prg_chr_bank[4] * 0x400 + 0x40000) | m37_mask, 0x0400, 0x0800);	//	R4 to 0x0800 - 0x0bff (1 bank)
			writeCHRRAM(m, (0x10 + prg_chr_bank[5] * 0x400 + 0x40000) | m37_mask, 0x0400, 0x0c00);	//	R5 to 0x0c00 - 0x0fff (1 bank)
			writeCHRRAM(m, (0x10 + (prg_chr_bank[0] & 0xffffe) * 0x400 + 0x40000) | m37_mask, 0x0800, 0x1000);	//	R0 to 0x1000 - 0x17ff (2 banks)
			writeCHRRAM(m, (0x10 + (prg_chr_bank[1] & 0xffffe) * 0x400 + 0x40000) | m37_mask, 0x0800, 0x1800);	//	R1 to 0x1800 - 0x1fff (2 banks)
		}
	}

};