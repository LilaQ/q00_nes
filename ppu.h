#pragma once
void initPPU();
void stepPPU();
void writePPUADDR(uint16_t adr, uint8_t cycle_no);
void writePPUDATA(uint8_t data);
void writePPUCTRL(uint8_t val);
uint8_t readPPUSTATUS();
bool NMIinterrupt();

struct PPUCTRL
{
	uint8_t value = 0;
	uint8_t nametable;
	uint8_t ppudata_increment;
	uint8_t ppudata_increment_value;
	uint8_t sprite_pattern_table_adr;
	uint8_t background_pattern_table_adr;
	uint8_t sprite_size;
	uint8_t ppu_master_slave;
	uint8_t generate_nmi;

	void setValue(uint8_t val) {
		value = val;
		nametable = val & 3;
		ppudata_increment = (val >> 2) & 1;
		sprite_pattern_table_adr = (val >> 3) & 1;
		background_pattern_table_adr = (val >> 4) & 1;
		sprite_size = (val >> 5) & 1;
		ppu_master_slave = (val >> 6) & 1;
		generate_nmi = (val >> 7) & 1;

		//	set actual values
		ppudata_increment_value = (ppudata_increment) ? 1 : 32;
	}
};