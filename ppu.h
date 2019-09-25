#pragma once
#include <string>
#include "mmu.h"
using namespace::std;
void initPPU(string filename);
void stepPPU();
void writePPUADDR(uint8_t adr);
void writePPUDATA(uint8_t data);
uint8_t readPPUDATA();
void writePPUCTRL(uint8_t val);
void writeCHRRAM(unsigned char cartridge[], uint32_t offset, uint16_t size, uint16_t target_address = 0x0000);
uint8_t readPPUSTATUS();
void writePPUSCROLL(uint8_t val);
void writePPUMASK(uint8_t val);
void drawCHRTable();
void drawOAM();
void drawNameTables();
void oamDMAtransfer(uint8_t val, unsigned char memory[]);
void writeOAMADDR(uint8_t val);
void writeOAMDATA(uint8_t val);
void initVRAM(VRAM_MIRRORING m);
void wrV(uint16_t adr, uint8_t val);
void isMMC3(bool v);
uint8_t readOAMDATA();
uint16_t getPPUCycles();
uint16_t getPPUScanlines();

struct PPUCTRL
{
	uint8_t value = 0;
	uint8_t base_nametable_address = 0;			//	bit 0 & bit 1
	uint16_t base_nametable_address_value = 0;
	uint8_t ppudata_increment = 0;				//	bit 2
	uint8_t ppudata_increment_value = 0;		
	uint8_t sprite_pattern_table_adr = 0;		//	bit 3
	uint16_t sprite_pattern_table_adr_value = 0;	
	uint8_t background_pattern_table_adr = 0;	//	bit 4
	uint16_t background_pattern_table_adr_value = 0;
	uint8_t sprite_size = 0;					//	bit 5
	uint8_t ppu_master_slave = 0;				//	bit 6
	uint8_t generate_nmi = 0;					//	bit 7

	void setValue(uint8_t val) {
		value = val;
		base_nametable_address = val & 3;
		ppudata_increment = (val >> 2) & 1;
		sprite_pattern_table_adr = (val >> 3) & 1;
		background_pattern_table_adr = (val >> 4) & 1;
		sprite_size = (val >> 5) & 1;
		ppu_master_slave = (val >> 6) & 1;
		generate_nmi = (val >> 7) & 1;

		//	set actual values
		ppudata_increment_value = (ppudata_increment == 0) ? 1 : 32;
		background_pattern_table_adr_value = (background_pattern_table_adr == 0) ? 0x0000 : 0x1000;
		sprite_pattern_table_adr_value = (sprite_pattern_table_adr == 0) ? 0x0000 : 0x1000;
		base_nametable_address_value = 0x2000 + (base_nametable_address * 0x400);

	}

	void reset() {
		setValue(value &= 0xfc);
	}

};

struct PPUSTATUS
{
	uint8_t value = 0x20;
	
	//	TODO last 5 bits are the last LSB bit stored to VRAM
	uint8_t sprite_overflow = 1;	//	bit 5
	uint8_t sprite0_hit = 0;		//	bit 6
	uint8_t vblank = 0;				//	bit 7;

	void setValue(uint8_t val) {
		value = val;
		sprite_overflow = (val >> 5) & 1;
		sprite0_hit = (val >> 6) & 1;
		vblank = (val >> 7) & 1;
	}

	void clearVBlank() {
		value &= 0x7f;
	}

	void setVBlank() {
		value |= 0x80;
	}

	void clearSpriteZero() {
		value &= 0xbf;
	}

	void setSpriteZero() {
		value |= 0x40;
	}

	bool isSpriteZero() {
		return (value & 0x40) > 0;
	}

	//	PPUSTATS 5-LSB bits have the values of the last PPU-Register write
	void appendLSB(uint8_t val) {
		value |= (val & 0x1f);
	}
};

struct PPUMASK {

	uint8_t value = 0;

	uint8_t greyscale = 0;
	uint8_t show_bg_left_8px = 0;
	uint8_t show_sprites_left_8px = 0;
	uint8_t show_bg = 0;
	uint8_t show_sprites = 0;

	void setValue(uint8_t val) {
		value = val;
		greyscale = value & 1;
		show_bg_left_8px = (value >> 1) & 1;
		show_sprites_left_8px = (value >> 2) & 1;
		show_bg = (value >> 3) & 1;
		show_sprites = (value >> 4) & 1;
	}

};