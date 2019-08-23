#include <stdint.h>
#include <stdio.h>
#include "SDL2/include/SDL.h"
#include "mmu.h"
#include "ppu.h"

//	init PPU
unsigned char VRAM[0x4000];		//	16 kbytes
unsigned char OAM[0x100];		//	256 bytes
const int FB_SIZE = 256 * 224 * 3;
SDL_Renderer* renderer;
SDL_Window* window;
SDL_Texture* texture;
SDL_Event event;
SDL_Renderer* renderer_nt;
SDL_Window* window_nt;
SDL_Texture* texture_nt;
SDL_Event event_nt;
unsigned char framebuffer[FB_SIZE];		//	3 bytes per pixel, RGB24
unsigned char framebuffer_bg[FB_SIZE];	//	3 bytes per pixel, RGB24
unsigned char framebuffer_chr[256 * 128 * 3];	//	3 bytes per pixel, RGB24 - CHR / Pattern Table
unsigned char framebuffer_nt[256 * 1024 * 3];	//	3 bytes per pixel, RGB24 - Nametables
uint16_t ppuScanline = 0;
uint16_t ppuCycles = 30;

//	Registers
PPUCTRL PPU_CTRL;
PPUSTATUS PPU_STATUS;
uint16_t PPUADDR = 0x0;
uint8_t OAMADDR = 0x0;
bool NMI_occured = false;
bool NMI_output = false;

//	DEBUG functions
uint16_t getPPUCycles() {
	return ppuCycles;
}
uint16_t getPPUScanlines() {
	return ppuScanline;
}

//	PPUCTRL write
void writePPUCTRL(uint8_t val) {
	PPU_CTRL.setValue(val);
	NMI_output = PPU_CTRL.generate_nmi > 0;

	PPU_STATUS.appendLSB(val);
}

//	PPUADDR write
void writePPUADDR(uint16_t adr, uint8_t cycle_nr) {
	//printf("VRAM PPUADDR write at: 0x%04x\n", PPUADDR);
	if (cycle_nr == 0) {
		PPUADDR = adr << 8;
	}
	else {
		PPUADDR |= adr;
	}

	PPU_STATUS.appendLSB(adr);
}

//	PPUDATA write
void writePPUDATA(uint8_t data) {
	if(PPUADDR == 0x220b)
		printf("VRAM PPUDATA write at: 0x%04x with val: 0x%02x\n", PPUADDR, data);
	VRAM[PPUADDR] = data;
	PPUADDR += PPU_CTRL.ppudata_increment_value;
	//	TODO : VRAM mirroring

	PPU_STATUS.appendLSB(data);
}

//	PPUSTATUS read
uint8_t readPPUSTATUS() {
	//printf("VRAM PPUSTATUS read\n");
	uint8_t ret = PPU_STATUS.value;
	PPU_STATUS.clearVBlank();
	NMI_occured = false;
	return ret;
}

//	NMI
bool NMIinterrupt() {
	if (NMI_occured && NMI_output) {
		NMI_occured = false;
		NMI_output = false;
		return true;
	}
	return false;
}

//	Load cartridge, map CHR-ROM to CHR-RAM (NROM)
void writeCHRRAM(unsigned char cartridge[], uint16_t offset) {
	for (int i = 0; i < 0x2000; i++) {
		VRAM[i] = cartridge[offset + i];
	}
}

//	OAM DMA Transfer - copy sprites to VRAM
void oamDMAtransfer(uint8_t val, unsigned char memory[]) {
	for (int i = 0; i < 0x100; i++)
		OAM[OAMADDR + i] = memory[(val << 8) + i];
}

//	Write OAM Address
void writeOAMADDR(uint8_t val) {
	OAMADDR = val;

	PPU_STATUS.appendLSB(val);
}

//	Write OAM Data
void writeOAMDATA(uint8_t val) {
	OAM[OAMADDR] = val;
	OAMADDR++;

	PPU_STATUS.appendLSB(val);
}

uint8_t readOAMDATA() {
	//	TODO, only in VBlank / FBlank?
	return OAM[OAMADDR];
}

uint32_t PALETTE[64] = {
 0x7C7C7C,
 0x0000FC,
 0x0000BC,
 0x4428BC,
 0x940084,
 0xA80020,
 0xA81000,
 0x881400,
 0x503000,
 0x007800,
 0x006800,
 0x005800,
 0x004058,
 0x000000,
 0x000000,
 0x000000,
 0xBCBCBC,
 0x0078F8,
 0x0058F8,
 0x6844FC,
 0xD800CC,
 0xE40058,
 0xF83800,
 0xE45C10,
 0xAC7C00,
 0x00B800,
 0x00A800,
 0x00A844,
 0x008888,
 0x000000,
 0x000000,
 0x000000,
 0xF8F8F8,
 0x3CBCFC,
 0x6888FC,
 0x9878F8,
 0xF878F8,
 0xF85898,
 0xF87858,
 0xFCA044,
 0xF8B800,
 0xB8F818,
 0x58D854,
 0x58F898,
 0x00E8D8,
 0x787878,
 0x000000,
 0x000000,
 0xFCFCFC,
 0xA4E4FC,
 0xB8B8F8,
 0xD8B8F8,
 0xF8B8F8,
 0xF8A4C0,
 0xF0D0B0,
 0xFCE0A8,
 0xF8D878,
 0xD8F878,
 0xB8F8B8,
 0xB8F8D8,
 0x00FCFC,
 0xF8D8F8,
 0x000000,
 0x000000
};

void initPPU() {
	//	init and create window and renderer
	SDL_Init(SDL_INIT_VIDEO);
	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
	SDL_CreateWindowAndRenderer(128, 256, 0, &window, &renderer);
	SDL_SetWindowSize(window, 256, 512);
	SDL_RenderSetLogicalSize(renderer, 256, 512);
	SDL_SetWindowResizable(window, SDL_TRUE);

	//	for fast rendering, create a texture
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 128, 256);

	//	init and create window and renderer
	SDL_Init(SDL_INIT_VIDEO);
	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
	SDL_CreateWindowAndRenderer(256, 960, 0, &window_nt, &renderer_nt);
	SDL_SetWindowSize(window_nt, 256, 960);
	SDL_RenderSetLogicalSize(renderer_nt, 256, 960);
	SDL_SetWindowResizable(window_nt, SDL_TRUE);

	//	for fast rendering, create a texture
	texture_nt = SDL_CreateTexture(renderer_nt, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 256, 960);


	//	DEBUG CALLS
	drawCHRTable();
}

int COLORS[] {
	0x00,
	0x55,
	0xaa,
	0xff
};

void drawCHRTable() {

	SDL_Renderer* renderer_chr;
	SDL_Window* window_chr;
	SDL_Texture* texture_chr;
	SDL_Event event_chr;

	//	init and create window and renderer
	SDL_Init(SDL_INIT_VIDEO);
	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
	SDL_CreateWindowAndRenderer(128, 256, 0, &window_chr, &renderer_chr);
	SDL_SetWindowSize(window_chr, 256, 512);
	SDL_RenderSetLogicalSize(renderer_chr, 256, 512);
	SDL_SetWindowResizable(window_chr, SDL_TRUE);

	//	window decorations
	char title[50];
	snprintf(title, sizeof title, "[ CHR tables L/R ]", 0, 0);
	SDL_SetWindowTitle(window_chr, title);

	//	for fast rendering, create a texture
	texture_chr = SDL_CreateTexture(renderer_chr, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 128, 256);

	for (int r = 0; r < 256; r++) {
		for (int col = 0; col < 128; col++) {
			uint16_t adr = (r / 8 * 0x100) + (r % 8) + (col / 8) * 0x10;

			uint8_t pixel = ((VRAM[adr] >> (7-(col % 8))) & 1) + ((VRAM[adr + 8] >> (7-(col % 8))) & 1) * 2;
			framebuffer_chr[(r * 128 * 3) + (col * 3)] = COLORS[pixel];
			framebuffer_chr[(r * 128 * 3) + (col * 3) + 1] = COLORS[pixel];
			framebuffer_chr[(r * 128 * 3) + (col * 3) + 2] = COLORS[pixel];
		}
	}

	SDL_UpdateTexture(texture_chr, NULL, framebuffer_chr, 128 * sizeof(unsigned char) * 3);
	SDL_RenderCopy(renderer_chr, texture_chr, NULL, NULL);
	SDL_RenderPresent(renderer_chr);

}

void drawNameTables() {

	//	window decorations
	char title[50];
	snprintf(title, sizeof title, "[ nametables ]", 0, 0);
	SDL_SetWindowTitle(window_nt, title);

	for (int r = 0; r < 960; r++) {
		for (int col = 0; col < 256; col++) {
			uint16_t tile_id = ((r / 8) * 32) + (col / 8);												//	sequential tile number
			uint16_t tile_nr = VRAM[0x2000 + (r / 8 * 32) + (col / 8)];									//	tile ID at the current address
			uint16_t adr = PPU_CTRL.background_pattern_table_adr_value + (tile_nr * 0x10) + (r % 8);	//	adress of the tile in CHR RAM

			//	select the correct byte of the attribute table
			uint16_t tile_attr_nr = VRAM[((0x2000 + (r / 8 * 32) + (col / 8)) & 0xfc00) + 0x03c0 + ((r / 32) * 8) + (col / 32)];
			//	select the part of the byte that we need (2-bits)
			uint16_t tile_attr_subitem = (((tile_id % 32) / 2 % 2) + (tile_id / 64 % 2) * 2) * 2;
			//printf("%d => tile_atr_sub %d \n", tile_id, tile_attr_subitem);
			
			uint16_t palette_offset = ((tile_attr_nr >> tile_attr_subitem) & 0x3) * 4;
			//if (((0x2000 + (r / 8 * 32) + (col / 8)) & 0xfc00) + 0x03c0 + ((r / 32) * 8) + (col / 32) == 0x23ca && (tile_id == 0xa9 || tile_id == 0xa8) )
				//printf("%d\n", tile_id);
				//printf("%x\n", VRAM[0x23ca]);
				//printf("%x\n", ((0x2000 + (r / 8 * 32) + (col / 8)) & 0xfc00) + 0x03c0 + ((r / 32) * 8) + (col / 32));
			//printf("tilenr: %x attnr: 0x%04x VRAM[attnr]: %d subitem: %d paloff: %02x\n", tile_id, tile_attr_nr, VRAM[tile_attr_nr], tile_attr_subitem, palette_offset);

			uint8_t pixel = ((VRAM[adr] >> (7 - (col % 8))) & 1) + (((VRAM[adr + 8] >> (7 - (col % 8))) & 1) * 2);
			framebuffer[(r * 256 * 3) + (col * 3)] = (PALETTE[VRAM[0x3f00 + palette_offset + pixel]] >> 16 ) & 0xff;
			framebuffer[(r * 256 * 3) + (col * 3) + 1] = (PALETTE[VRAM[0x3f00 + palette_offset + pixel]] >> 8) & 0xff;
			framebuffer[(r * 256 * 3) + (col * 3) + 2] = (PALETTE[VRAM[0x3f00 + palette_offset + pixel]] ) & 0xff;


			/*framebuffer[(r * 256 * 3) + (col * 3)] = COLORS[pixel];
			framebuffer[(r * 256 * 3) + (col * 3) + 1] = COLORS[pixel];
			framebuffer[(r * 256 * 3) + (col * 3) + 2] = COLORS[pixel];*/
		}
	}

	SDL_UpdateTexture(texture_nt, NULL, framebuffer, 256 * sizeof(unsigned char) * 3);
	SDL_RenderCopy(renderer_nt, texture_nt, NULL, NULL);
	SDL_RenderPresent(renderer_nt);

}

void handleWindowEvents() {
	//	poll events from menu
	SDL_PollEvent(&event);
}

void stepPPU() {

	//drawCHRTable();
	//handleWindowEvents();

	//	stepPPU
	ppuCycles++;
	if (ppuCycles > 340) {
		ppuCycles -= 341;
		ppuScanline++;
	}
	//printf("PPU Cycles: %d Scanlines: %d\n", ppuCycles, ppuScanline);
	if (0 <= ppuScanline && ppuScanline <= 239) {	//	drawing
		//printf("Drawing NTs!\n");
		/*
		Nametable byte
		Attribute table byte
		Pattern table tile low
		Pattern table tile high(+8 bytes from pattern table tile low)
		*/
	} 
	else if (ppuScanline == 241 && ppuCycles == 1) {	//	VBlank 
		//printf("VBlanking!\n");
		PPU_STATUS.setVBlank();
		NMI_occured = true;
		drawNameTables();
		handleWindowEvents();
	}
	else if (ppuScanline == 261 && ppuCycles == 1) {	//	VBlank off / pre-render line
		//printf("VBlanking off!\n");
		PPU_STATUS.clearVBlank();
		NMI_occured = false;
		ppuScanline = 0;
	}

}