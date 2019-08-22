#pragma once
void initPPU();
void stepPPU();
void writePPUADDR(uint16_t adr, uint8_t cycle_no);
void writePPUDATA(uint8_t data);
void writePPUCTRL(uint8_t val);
void writeCHRRAM(unsigned char cartridge[], uint16_t offset);
uint8_t readPPUSTATUS();
bool NMIinterrupt();
void drawCHRTable();
void oamDMAtransfer(uint8_t val, unsigned char memory[]);
void writeOAMADDR(uint8_t val);
void writeOAMDATA(uint8_t val);
uint8_t readOAMDATA();
uint16_t getPPUCycles();
uint16_t getPPUScanlines();

struct PPUCTRL
{
	uint8_t value = 0;
	uint8_t base_nametable_address;			//	bit 0 & bit 1
	uint8_t ppudata_increment;				//	bit 2
	uint8_t ppudata_increment_value;		
	uint8_t sprite_pattern_table_adr;		//	bit 3
	uint8_t background_pattern_table_adr;	//	bit 4
	uint16_t background_pattern_table_adr_value;
	uint8_t sprite_size;					//	bit 5
	uint8_t ppu_master_slave;				//	bit 6
	uint8_t generate_nmi;					//	bit 7

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
	}
};

struct PPUSTATUS
{
	uint8_t value = 0;
	
	//	TODO last 5 bits are the last LSB bit stored to VRAM
	uint8_t sprite_overflow;	//	bit 5
	uint8_t sprite0_hit;		//	bit 6
	uint8_t vblank;				//	bit 7;

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

	//	PPUSTATS 5-LSB bits have the values of the last PPU-Register write
	void appendLSB(uint8_t val) {
		value |= (val & 0x1f);
	}
};