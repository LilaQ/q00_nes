#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include "SDL2/include/SDL.h"
#include "mmu.h"
#include "ppu.h"
#include "wmu.h"
#ifdef _WIN32
	#include <Windows.h>
	#include <WinUser.h>
	#include "SDL2/include/SDL_syswm.h"
#endif // _WIN32

using namespace::std;

//	init PPU
unsigned char VRAM[0x4000];		//	16 kbytes
unsigned char OAM[0x100];		//	256 bytes
const int FB_SIZE = 256 * 224 * 3;
SDL_Renderer *renderer, *renderer_nt, *renderer_oam;
SDL_Window *window, *window_nt, *window_oam;
SDL_Texture *texture, *texture_nt, *texture_oam;
unsigned char framebuffer[FB_SIZE];		//	3 bytes per pixel, RGB24
unsigned char framebuffer_bg[FB_SIZE];	//	3 bytes per pixel, RGB24
unsigned char framebuffer_chr[256 * 128 * 3];	//	3 bytes per pixel, RGB24 - CHR / Pattern Table
unsigned char framebuffer_nt[256 * 1024 * 3];	//	3 bytes per pixel, RGB24 - Nametables
unsigned char framebuffer_oam[256 * 224 * 3];	//	3 bytes per pixel, RGB24 - OAM / Sprites
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

//	NES color palette
uint32_t PALETTE[64] = {
		0x7C7C7C, 0x0000FC, 0x0000BC, 0x4428BC, 0x940084, 0xA80020, 0xA81000, 0x881400, 0x503000, 0x007800, 0x006800, 0x005800, 0x004058, 0x000000, 0x000000, 0x000000,
		0xBCBCBC, 0x0078F8, 0x0058F8, 0x6844FC, 0xD800CC, 0xE40058, 0xF83800, 0xE45C10, 0xAC7C00, 0x00B800, 0x00A800, 0x00A844, 0x008888, 0x000000, 0x000000, 0x000000,
		0xF8F8F8, 0x3CBCFC, 0x6888FC, 0x9878F8, 0xF878F8, 0xF85898, 0xF87858, 0xFCA044, 0xF8B800, 0xB8F818, 0x58D854, 0x58F898, 0x00E8D8, 0x787878, 0x000000, 0x000000,
		0xFCFCFC, 0xA4E4FC, 0xB8B8F8, 0xD8B8F8, 0xF8B8F8, 0xF8A4C0, 0xF0D0B0, 0xFCE0A8, 0xF8D878, 0xD8F878, 0xB8F8B8, 0xB8F8D8, 0x00FCFC, 0xF8D8F8, 0x000000, 0x000000
};

void initPPU(string filename) {

	/*
		MAIN WINDOW
	*/

	//	init and create window and renderer
	SDL_Init(SDL_INIT_VIDEO);
	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
	SDL_CreateWindowAndRenderer(256, 224, 0, &window, &renderer);
	SDL_SetWindowSize(window, 512, 448);
	SDL_RenderSetLogicalSize(renderer, 512, 448);
	SDL_SetWindowResizable(window, SDL_TRUE);

	//	for fast rendering, create a texture
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 256, 224);

	SDL_version compiled;
	SDL_version linked;

	SDL_VERSION(&compiled);
	SDL_GetVersion(&linked);
	printf("We compiled against SDL version %d.%d.%d ...\n",
		compiled.major, compiled.minor, compiled.patch);
	printf("But we are linking against SDL version %d.%d.%d.\n",
		linked.major, linked.minor, linked.patch);


	/*
		INIT WINDOW
	*/
	initWindow(window, filename);

}

int COLORS[] {
	0x00,
	0x55,
	0xaa,
	0xff
};


/*
	DRAW FRAME
*/

void drawFrame() {

	for (int r = 0; r < 960; r++) {
		for (int col = 0; col < 256; col++) {
			uint16_t tile_id = ((r / 8) * 32) + (col / 8);												//	sequential tile number
			uint16_t tile_nr = VRAM[0x2000 + (r / 8 * 32) + (col / 8)];									//	tile ID at the current address
			uint16_t adr = PPU_CTRL.background_pattern_table_adr_value + (tile_nr * 0x10) + (r % 8);	//	adress of the tile in CHR RAM

			//	select the correct byte of the attribute table
			uint16_t tile_attr_nr = VRAM[((0x2000 + (r / 8 * 32) + (col / 8)) & 0xfc00) + 0x03c0 + ((r / 32) * 8) + (col / 32)];
			//	select the part of the byte that we need (2-bits)
			uint16_t attr_shift = (((tile_id % 32) / 2 % 2) + (tile_id / 64 % 2) * 2) * 2;
			uint16_t palette_offset = ((tile_attr_nr >> attr_shift) & 0x3) * 4;
			uint8_t pixel = ((VRAM[adr] >> (7 - (col % 8))) & 1) + (((VRAM[adr + 8] >> (7 - (col % 8))) & 1) * 2);
			framebuffer[(r * 256 * 3) + (col * 3)] = (PALETTE[VRAM[0x3f00 + palette_offset + pixel]] >> 16) & 0xff;
			framebuffer[(r * 256 * 3) + (col * 3) + 1] = (PALETTE[VRAM[0x3f00 + palette_offset + pixel]] >> 8) & 0xff;
			framebuffer[(r * 256 * 3) + (col * 3) + 2] = (PALETTE[VRAM[0x3f00 + palette_offset + pixel]]) & 0xff;

		}
	}

	for (int i = 63; i >= 0; i--) {
		uint8_t Y_Pos = OAM[i * 4];		//	when drawing "+1" is needed for the Y-Position, it's a quirk of the N64 because of the prep-scanline
		uint8_t Tile_Index_Nr = OAM[i * 4 + 1];
		uint8_t Attributes = OAM[i * 4 + 2];
		uint8_t X_Pos = OAM[i * 4 + 3];
		uint16_t Palette_Offset = 0x3f10 + ((Attributes & 3) * 4);

		//	iterate through 8x8 sprite in Pattern Table, with offset of Y_Pos and X_Pos
		for (int j = 0; j < 8; j++) {
			for (int t = 0; t < 8; t++) {
				uint8_t V = 0x00;
				switch ((Attributes >> 6) & 3) {
					case 0x00:	//	no flip
						V = ((VRAM[PPU_CTRL.sprite_pattern_table_adr_value + Tile_Index_Nr * 0x10 + j] >> (7 - (t % 8))) & 1) + ((VRAM[PPU_CTRL.sprite_pattern_table_adr_value + Tile_Index_Nr * 0x10 + j + 8] >> (7 - (t % 8))) & 1) * 2;
						break;
					case 0x01:	//	horizontal flip
						V = ((VRAM[PPU_CTRL.sprite_pattern_table_adr_value + Tile_Index_Nr * 0x10 + j] >> (t % 8)) & 1) + ((VRAM[PPU_CTRL.sprite_pattern_table_adr_value + Tile_Index_Nr * 0x10 + j + 8] >> (t % 8)) & 1) * 2;
						break;
					case 0x02:	//	vertical flip
						break;
					case 0x03:	//	horizontal & vertical flip
						break;
				}
				uint8_t R = (PALETTE[VRAM[Palette_Offset + V]] >> 16) & 0xff;
				uint8_t G = (PALETTE[VRAM[Palette_Offset + V]] >> 8) & 0xff;
				uint8_t B = PALETTE[VRAM[Palette_Offset + V]] & 0xff;

				if (V) {
					//	when drawing "+1" is needed for the Y-Position, it's a quirk of the N64 because of the prep-scanline
					framebuffer[((Y_Pos + 1 + j) * 256 * 3) + ((X_Pos + t) * 3)] = R;
					framebuffer[((Y_Pos + 1 + j) * 256 * 3) + ((X_Pos + t) * 3) + 1] = G;
					framebuffer[((Y_Pos + 1 + j) * 256 * 3) + ((X_Pos + t) * 3) + 2] = B;

					//	sprite zero hit
					if (!PPU_STATUS.isSpriteZero() && i == 0)
						PPU_STATUS.setSpriteZero();
				}
			}
		}
	}

	SDL_UpdateTexture(texture, NULL, framebuffer, 256 * sizeof(unsigned char) * 3);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);

}

void drawCHRTable() {

	SDL_Renderer* renderer_chr;
	SDL_Window* window_chr;
	SDL_Texture* texture_chr;
	SDL_Event event_chr;

	//	init and create window and renderer
	SDL_CreateWindowAndRenderer(128, 256, 0, &window_chr, &renderer_chr);
	SDL_SetWindowSize(window_chr, 256, 512);
	SDL_RenderSetLogicalSize(renderer_chr, 256, 512);
	SDL_SetWindowResizable(window_chr, SDL_TRUE);

	//	window decorations
	char title[50];
	snprintf(title, sizeof title, "[ CHR tables L/R ]", 0);
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


/*
	OAM / SPRITES
*/
void drawOAM() {

	SDL_CreateWindowAndRenderer(256, 224, 0, &window_oam, &renderer_oam);
	SDL_SetWindowSize(window_oam, 256, 224);
	SDL_RenderSetLogicalSize(renderer_oam, 256, 224);
	SDL_SetWindowResizable(window_oam, SDL_TRUE);

	//	window decorations
	char title[50];
	snprintf(title, sizeof title, "[ OAM / Sprites ]", 0);
	SDL_SetWindowTitle(window_oam, title);

	//	for fast rendering, create a texture
	texture_oam = SDL_CreateTexture(renderer_oam, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 256, 224);

	//	clear array
	memset(framebuffer_oam, 0x00, sizeof(framebuffer_oam));

	for (int i = 0; i < 64; i++) {
		uint8_t Y_Pos = OAM[i * 4];
		uint8_t Tile_Index_Nr = OAM[i * 4 + 1];
		uint8_t Attributes = OAM[i * 4 + 2];
		uint8_t X_Pos = OAM[i * 4 + 3];
		uint16_t Palette_Offset = 0x3f10 + ((Attributes & 3) * 4);

		//	iterate through 8x8 sprite in Pattern Table, with offset of Y_Pos and X_Pos
		for (int j = 0; j < 8; j++) {
			for (int t = 0; t < 8; t++) {
				uint8_t V = ((VRAM[PPU_CTRL.sprite_pattern_table_adr_value + Tile_Index_Nr * 0x10 + j] >> (7 - (t % 8))) & 1) + ((VRAM[PPU_CTRL.sprite_pattern_table_adr_value + Tile_Index_Nr * 0x10 + j + 8] >> (7 - (t % 8))) & 1) * 2;
				uint8_t R = (PALETTE[VRAM[Palette_Offset + V]] >> 16) & 0xff;
				uint8_t G = (PALETTE[VRAM[Palette_Offset + V]] >> 8) & 0xff;
				uint8_t B = PALETTE[VRAM[Palette_Offset + V]] & 0xff;

				if (V) {
					framebuffer_oam[((Y_Pos + j) * 256 * 3) + ((X_Pos + t) * 3)] = R;
					framebuffer_oam[((Y_Pos + j) * 256 * 3) + ((X_Pos + t) * 3) + 1] = G;
					framebuffer_oam[((Y_Pos + j) * 256 * 3) + ((X_Pos + t) * 3) + 2] = B;
				}
			}
		}
	}

	SDL_UpdateTexture(texture_oam, NULL, framebuffer_oam, 256 * sizeof(unsigned char) * 3);
	SDL_RenderCopy(renderer_oam, texture_oam, NULL, NULL);
	SDL_RenderPresent(renderer_oam);

}


/*
	NAMETABLES
*/
void drawNameTables() {

	SDL_CreateWindowAndRenderer(256, 960, 0, &window_nt, &renderer_nt);
	SDL_SetWindowSize(window_nt, 256, 960);
	SDL_RenderSetLogicalSize(renderer_nt, 256, 960);
	SDL_SetWindowResizable(window_nt, SDL_TRUE);

	//	for fast rendering, create a texture
	texture_nt = SDL_CreateTexture(renderer_nt, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 256, 960);

	//	window decorations
	char title[50];
	snprintf(title, sizeof title, "[ nametables ]", 0);
	SDL_SetWindowTitle(window_nt, title);

	for (int r = 0; r < 960; r++) {
		for (int col = 0; col < 256; col++) {
			uint16_t tile_id = ((r / 8) * 32) + (col / 8);												//	sequential tile number
			uint16_t tile_nr = VRAM[0x2000 + (r / 8 * 32) + (col / 8)];									//	tile ID at the current address
			uint16_t adr = PPU_CTRL.background_pattern_table_adr_value + (tile_nr * 0x10) + (r % 8);	//	adress of the tile in CHR RAM

			//	select the correct byte of the attribute table
			uint16_t tile_attr_nr = VRAM[((0x2000 + (r / 8 * 32) + (col / 8)) & 0xfc00) + 0x03c0 + ((r / 32) * 8) + (col / 32)];
			//	select the part of the byte that we need (2-bits)
			uint16_t attr_shift = (((tile_id % 32) / 2 % 2) + (tile_id / 64 % 2) * 2) * 2;
			uint16_t palette_offset = ((tile_attr_nr >> attr_shift) & 0x3) * 4;
			uint8_t pixel = ((VRAM[adr] >> (7 - (col % 8))) & 1) + (((VRAM[adr + 8] >> (7 - (col % 8))) & 1) * 2);
			framebuffer_nt[(r * 256 * 3) + (col * 3)] = (PALETTE[VRAM[0x3f00 + palette_offset + pixel]] >> 16 ) & 0xff;
			framebuffer_nt[(r * 256 * 3) + (col * 3) + 1] = (PALETTE[VRAM[0x3f00 + palette_offset + pixel]] >> 8) & 0xff;
			framebuffer_nt[(r * 256 * 3) + (col * 3) + 2] = (PALETTE[VRAM[0x3f00 + palette_offset + pixel]] ) & 0xff;

		}
	}

	SDL_UpdateTexture(texture_nt, NULL, framebuffer_nt, 256 * sizeof(unsigned char) * 3);
	SDL_RenderCopy(renderer_nt, texture_nt, NULL, NULL);
	SDL_RenderPresent(renderer_nt);

}

void stepPPU() {

	//	stepPPU
	ppuCycles++;
	if (ppuCycles > 340) {
		ppuCycles -= 341;
		ppuScanline++;
	}
	//printf("PPU Cycles: %d Scanlines: %d\n", ppuCycles, ppuScanline);
	if (0 <= ppuScanline && ppuScanline <= 239) {	//	drawing
		//printf("Drawing NTs!\n");
	}
	else if (ppuScanline == 241 && ppuCycles == 1) {	//	VBlank 
		//printf("VBlanking!\n");
		PPU_STATUS.setVBlank();
		if (PPU_CTRL.generate_nmi) {
			NMI_occured = true;
			NMI_output = true;
		}
		drawFrame();
	}
	else if (ppuScanline == 261) {
		//	VBlank off / pre-render line
		if (ppuCycles == 1) {
			PPU_STATUS.clearVBlank();
			PPU_STATUS.clearSpriteZero();
			NMI_occured = false;
		}
		else if (ppuCycles == 340) {
			ppuScanline = 0;
		}

	}

}