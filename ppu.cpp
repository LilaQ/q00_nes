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
unsigned char *_VRAM[0x4000];	//	16 kbytes
unsigned char pVRAM[0x4000];		
unsigned char OAM[0x100];		//	256 bytes
const int FB_SIZE = 256 * 240 * 3;
SDL_Renderer *renderer, *renderer_nt, *renderer_oam;
SDL_Window *window, *window_nt, *window_oam;
SDL_Texture *texture, *texture_nt, *texture_oam;
unsigned char framebuffer[FB_SIZE];		//	3 bytes per pixel, RGB24
unsigned char framebuffer_bg[FB_SIZE];	//	3 bytes per pixel, RGB24
unsigned char framebuffer_chr[256 * 128 * 3];	//	3 bytes per pixel, RGB24 - CHR / Pattern Table
unsigned char framebuffer_nt[256 * 960 * 3];	//	3 bytes per pixel, RGB24 - Nametables
unsigned char framebuffer_oam[256 * 240 * 3];	//	3 bytes per pixel, RGB24 - OAM / Sprites
uint16_t ppuScanline = 0;
uint16_t ppuCycles = 30;
uint8_t PPUDATA_buffer = 0x00;
uint8_t PPUSCROLL_x, tmp_PPUSCROLL_x, tmp_PPUSCROLL_y, tmp_PPUSCROLL_fine_y, PPUSCROLL_fine_y, PPUSCROLL_pos = 0;
int8_t PPUSCROLL_y;
uint16_t scr_v, scr_t;
uint8_t scr_x;
bool scr_w;
uint8_t nts = 0;

//	Registers
PPUCTRL PPU_CTRL;
PPUSTATUS PPU_STATUS;
PPUMASK PPU_MASK;
uint16_t PPUADDR = 0x0;
uint8_t OAMADDR = 0x0;
bool NMI_occured = false;
bool NMI_output = false;

void initVRAM(VRAM_MIRRORING m) {
	for (int i = 0; i < 0x4000; i++) {
		_VRAM[i] = &pVRAM[i];
	}
	switch (m)
	{
	case VERTICAL:
		for (int i = 0; i < 0x400; i++) {
			_VRAM[0x2000 + i] = &pVRAM[0x2000 + i];
			_VRAM[0x2800 + i] = &pVRAM[0x2000 + i];
			_VRAM[0x2400 + i] = &pVRAM[0x2400 + i];
			_VRAM[0x2c00 + i] = &pVRAM[0x2400 + i];
		}
		break;
		break;
	case HORIZONTAL:
		for (int i = 0; i < 0x400; i++) {
			_VRAM[0x2000 + i] = &pVRAM[0x2000 + i];
			_VRAM[0x2400 + i] = &pVRAM[0x2000 + i];
			_VRAM[0x2800 + i] = &pVRAM[0x2800 + i];
			_VRAM[0x2c00 + i] = &pVRAM[0x2800 + i];
		}
		break;
	case NONE:
	default:
		break;
	}
}

void wrV(uint16_t adr, uint8_t val) {
	*_VRAM[adr] = val;

}

uint8_t rdV(uint16_t adr) {
	return *_VRAM[adr];
}

//	DEBUG functions
uint16_t getPPUCycles() {
	return ppuCycles;
}
uint16_t getPPUScanlines() {
	return ppuScanline;
}

//	PPUCTRL write (2000)
void writePPUCTRL(uint8_t val) {
	PPU_CTRL.setValue(val);
	NMI_output = PPU_CTRL.generate_nmi > 0;
	PPU_STATUS.appendLSB(val);
	nts = val & 3;
}

//	PPUADDR write (2006)
void writePPUADDR(uint8_t adr) {

	if (scr_w == 0) {
		PPUADDR = adr << 8;
		nts = ((adr >> 2) & 3);

		//	set temp scroll values
		tmp_PPUSCROLL_y &= 0b111;
		tmp_PPUSCROLL_y |= (adr & 0b11) << 3;
		tmp_PPUSCROLL_fine_y = (adr >> 12) & 0b111;

		scr_w = 1;
	}
	else {
		PPUADDR |= adr;
		PPU_CTRL.setValue((PPU_CTRL.value & 0xfc) | nts);

		//	set temp scroll values (also pass to real values)
		tmp_PPUSCROLL_y &= 0b11000;
		tmp_PPUSCROLL_y |= (adr >> 5) & 0b111;
		
		//if (ppuScanline >= 1 && ppuScanline <= 239 && tmp_PPUSCROLL_y) {
			PPUSCROLL_y = tmp_PPUSCROLL_y - (ppuScanline / 8);
			PPUSCROLL_fine_y = tmp_PPUSCROLL_fine_y;
		//}

		scr_w = 0;
	}

	PPU_STATUS.appendLSB(adr & 0xff);
}

//	PPUDATA write
void writePPUDATA(uint8_t data) {
	if (PPUADDR == 0x3f10)
		wrV(0x3f00, data);
	if (PPUADDR == 0x3f14)
		wrV(0x3f04, data);
	if (PPUADDR == 0x3f18)
		wrV(0x3f08, data);
	if (PPUADDR == 0x3f1c)
		wrV(0x3f0c, data);
	if (PPUADDR == 0x3f00)
		wrV(0x3f10, data);
	if (PPUADDR == 0x3f04)
		wrV(0x3f14, data);
	if (PPUADDR == 0x3f08)
		wrV(0x3f18, data);
	if (PPUADDR == 0x3f0c)
		wrV(0x3f1c, data);
	wrV(PPUADDR, data);
	PPUADDR += PPU_CTRL.ppudata_increment_value;

	PPU_STATUS.appendLSB(data);
}

//	PPUDATA read
uint8_t readPPUDATA() {
	uint8_t ret = PPUDATA_buffer;
	PPUDATA_buffer = rdV(PPUADDR);
	PPUADDR += PPU_CTRL.ppudata_increment_value;
	return ret;
}

//	PPUSTATUS read
uint8_t readPPUSTATUS() {
	uint8_t ret = PPU_STATUS.value;
	PPU_STATUS.clearVBlank();
	NMI_occured = false;
	PPUADDR += PPU_CTRL.ppudata_increment_value;
	scr_w = 0;
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
		wrV(i, cartridge[offset + i]);
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

//	Read OAM Data
uint8_t readOAMDATA() {
	//	TODO, only in VBlank / FBlank?
	return OAM[OAMADDR];
}

//	Write PPUSCROLL (2005)
void writePPUSCROLL(uint8_t val) {

	if (scr_w == 0) {

		//	set temp scroll values
		tmp_PPUSCROLL_x = (val & 0x1f);
		PPUSCROLL_x = val;
		scr_w = 1;
	}
	else {
		//	set temp scroll values
		tmp_PPUSCROLL_y = (val >> 3) & 0b11111;
		tmp_PPUSCROLL_fine_y = val & 0b111;

		scr_w = 0;
	}
}

//	Write PPUMASK
void writePPUMASK(uint8_t val) {
	PPU_MASK.setValue(val);
}

/*
	FIX PALETTE ADDRESSES
	0x3f04, 0x3f08 and 0x3f0c fall back to default color at 0x3f00
*/
uint16_t fixPalette(uint16_t c) {
	if ((c & 3) == 0) {
		return c & 0xffe0;
	}
	return c;
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
	SDL_CreateWindowAndRenderer(256, 240, 0, &window, &renderer);
	SDL_SetWindowSize(window, 512, 480);
	//SDL_RenderSetLogicalSize(renderer, 512, 480);
	SDL_SetWindowResizable(window, SDL_TRUE);

	//	for fast rendering, create a texture
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 256, 240);

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
	SDL_UpdateTexture(texture, NULL, framebuffer, 256 * sizeof(unsigned char) * 3);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}


void drawCHRTable() {

	SDL_Renderer* renderer_chr;
	SDL_Window* window_chr;
	SDL_Texture* texture_chr;

	//	init and create window and renderer
	SDL_CreateWindowAndRenderer(128, 256, 0, &window_chr, &renderer_chr);
	SDL_SetWindowSize(window_chr, 256, 512);
	SDL_RenderSetLogicalSize(renderer_chr, 256, 512);
	SDL_SetWindowResizable(window_chr, SDL_TRUE);

	//	window decorations
	char title[50];
	snprintf(title, sizeof title, "[ CHR tables L/R ]");
	SDL_SetWindowTitle(window_chr, title);

	//	for fast rendering, create a texture
	texture_chr = SDL_CreateTexture(renderer_chr, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 128, 256);

	for (int r = 0; r < 256; r++) {
		for (int col = 0; col < 128; col++) {
			uint16_t adr = (r / 8 * 0x100) + (r % 8) + (col / 8) * 0x10;

			uint8_t pixel = ((rdV(adr) >> (7-(col % 8))) & 1) + ((rdV(adr + 8) >> (7-(col % 8))) & 1) * 2;
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

	SDL_CreateWindowAndRenderer(256, 240, 0, &window_oam, &renderer_oam);
	SDL_SetWindowSize(window_oam, 256, 240);
	SDL_RenderSetLogicalSize(renderer_oam, 256, 240);
	SDL_SetWindowResizable(window_oam, SDL_TRUE);

	//	window decorations
	char title[50];
	snprintf(title, sizeof title, "[ OAM / Sprites ]");
	SDL_SetWindowTitle(window_oam, title);

	//	for fast rendering, create a texture
	texture_oam = SDL_CreateTexture(renderer_oam, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 256, 240);

	//	clear array
	memset(framebuffer_oam, 0xcc, sizeof(framebuffer_oam));

	for (int i = 0; i < 64; i++) {
		uint8_t Y_Pos = OAM[i * 4];
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
						V = ((rdV(PPU_CTRL.sprite_pattern_table_adr_value + Tile_Index_Nr * 0x10 + j) >> (7 - (t % 8))) & 1) + ((rdV(PPU_CTRL.sprite_pattern_table_adr_value + Tile_Index_Nr * 0x10 + j + 8) >> (7 - (t % 8))) & 1) * 2;
						break;
					case 0x01:	//	horizontal flip
						V = ((rdV(PPU_CTRL.sprite_pattern_table_adr_value + Tile_Index_Nr * 0x10 + j) >> (t % 8)) & 1) + ((rdV(PPU_CTRL.sprite_pattern_table_adr_value + Tile_Index_Nr * 0x10 + j + 8) >> (t % 8)) & 1) * 2;
						break;
					case 0x02:	//	vertical flip
						break;
					case 0x03:	//	horizontal & vertical flip
						break;
				}
				uint8_t R = (PALETTE[rdV(Palette_Offset + V)] >> 16) & 0xff;
				uint8_t G = (PALETTE[rdV(Palette_Offset + V)] >> 8) & 0xff;
				uint8_t B = PALETTE[rdV(Palette_Offset + V)] & 0xff;

				if (V) {
					//	when drawing "+1" is needed for the Y-Position, it's a quirk of the N64 because of the prep-scanline
					framebuffer_oam[((Y_Pos + 1 + j) * 256 * 3) + ((X_Pos + t) * 3)] = R;
					framebuffer_oam[((Y_Pos + 1 + j) * 256 * 3) + ((X_Pos + t) * 3) + 1] = G;
					framebuffer_oam[((Y_Pos + 1 + j) * 256 * 3) + ((X_Pos + t) * 3) + 2] = B;

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
	snprintf(title, sizeof title, "[ nametables ]");
	SDL_SetWindowTitle(window_nt, title);

	for (int r = 0; r < 960; r++) {
		for (int col = 0; col < 256; col++) {
			uint16_t tile_id = ((r / 8) * 32) + (col / 8);												//	sequential tile number
			uint16_t natural_address = 0x2000 + tile_id;
			natural_address += 0x40 * ((natural_address % 0x2000) / 0x3c0);
			uint16_t tile_nr = rdV(natural_address);													//	tile ID at the current address (skip attribute tables)
			uint16_t adr = PPU_CTRL.background_pattern_table_adr_value + (tile_nr * 0x10) + (r % 8);	//	adress of the tile in CHR RAM

			//	select the correct byte of the attribute table
			uint16_t tile_attr_nr = rdV((natural_address & 0xfc00) + 0x03c0 + (((((r + (r / 240) * 16) / 32) * 8) + (col / 32)) % 0x40));

			//	select the part of the byte that we need (2-bits)
			uint16_t attr_shift = (((tile_id % 32) / 2 % 2) + ( (tile_id + (r/240) * 64) / 64 % 2) * 2) * 2;
			uint16_t palette_offset = ((tile_attr_nr >> attr_shift) & 0x3) * 4;
			uint8_t pixel = ((rdV(adr) >> (7 - (col % 8))) & 1) + (((rdV(adr + 8) >> (7 - (col % 8))) & 1) * 2);

			framebuffer_nt[(r * 256 * 3) + (col * 3)] = (PALETTE[rdV(fixPalette(0x3f00 + palette_offset + pixel))] >> 16 ) & 0xff;
			framebuffer_nt[(r * 256 * 3) + (col * 3) + 1] = (PALETTE[rdV(fixPalette(0x3f00 + palette_offset + pixel))] >> 8) & 0xff;
			framebuffer_nt[(r * 256 * 3) + (col * 3) + 2] = (PALETTE[rdV(fixPalette(0x3f00 + palette_offset + pixel))] ) & 0xff;

		}
	}

	SDL_UpdateTexture(texture_nt, NULL, framebuffer_nt, 256 * sizeof(unsigned char) * 3);
	SDL_RenderCopy(renderer_nt, texture_nt, NULL, NULL);
	SDL_RenderPresent(renderer_nt);

}


uint16_t getAttribute(uint16_t adr) {
	uint16_t base = (adr & 0xfc00) + 0x03c0;
	uint16_t diff = adr - (adr & 0xfc00);
	uint16_t cell = (diff / 128) * 8 + ((diff % 128) % 32 / 4);
	return rdV(base + cell);
}

uint8_t getAttributeTilePart(uint16_t adr) {
	uint16_t diff = adr - (adr & 0xfc00);
	uint8_t tilePar = (diff / 2) % 2 + ((diff / 64) % 2) * 2;
	return tilePar * 2;
}

uint16_t translateScrolledAddress(uint16_t adr, uint8_t scroll_x, int8_t scroll_y, uint8_t x, uint8_t y, uint8_t tile_id) {


	uint16_t scrolled_address = adr + (scroll_y) * 0x20;
	bool y_mod = false;

	//	appears to be in AT area
	if (scrolled_address % 0x400 >= 0x3c0) {
		scrolled_address = (scrolled_address & 0x2800 ^ 0x800 ^ 0x400) + ((scrolled_address % 0x400) - 0x3c0);
		y_mod = true;
	}
	//	crossed NT
	else if ((adr & 0x400) != (scrolled_address & 0x400)) {
		scrolled_address ^= 0x400;
		//	scroll values above what's vertically visible in the viewport, will NOT make the NT change!
		if (scroll_y <= 29) {
			scrolled_address ^= 0x800;	//	XOR the bit, that changes NTs vertically
			scrolled_address += 0x40;	//	skip 64 bytes of attribute table
		}
	}

	uint16_t temp_adr = scrolled_address;
	scrolled_address += (scroll_x / 8);

	bool x_correct = false;

	if (((scrolled_address & 0xffe0) != (temp_adr & 0xffe0)) || ((x > 128) && ((tile_id % 32) == 0)))		//	check if X-boundary crossed ( AND account for transfer-tile glitch)
	{
		if (y_mod) {
			scrolled_address = adr + scroll_y * 0x20 + (scroll_x / 8);
		}
		scrolled_address ^= 0x800;	//	XOR the bit, that changes NTs vertically
		scrolled_address ^= 0x400;	//	XOR the bit, that changes NTs horizontally
		x_correct = true;
	}

	if (x_correct) {
		scrolled_address -= 0x20;	//	remove the 'line break'
	}

	return scrolled_address;
}

/*
	CALC SCANLINE ARRAY FOR ROW X
*/
uint16_t tile_id, tile_nr, adr, tile_attr_nr, attr_shift, palette_offset, Palette_Offset;
uint8_t pixel, Y_Pos, Tile_Index_Nr, Attributes, X_Pos;
void renderScanline(uint16_t row) {

	int r = row;
	if (PPU_MASK.show_bg) {
		for (int col = 0; col < 256; col++) {
			tile_id = (((r+PPUSCROLL_fine_y) / 8) * 32) + ((col + PPUSCROLL_x % 8) / 8);					//	sequential tile number
			uint16_t natural_address = PPU_CTRL.base_nametable_address_value + tile_id;
			uint16_t scrolled_address = translateScrolledAddress(natural_address, PPUSCROLL_x, PPUSCROLL_y, col, r, tile_id);
			tile_nr = rdV(scrolled_address);												//	tile ID at the current address
			adr = PPU_CTRL.background_pattern_table_adr_value + (tile_nr * 0x10) + ((r+PPUSCROLL_fine_y) % 8);	//	adress of the tile in CHR RAM

			if(r==150 && !col)
			printf("Using Y-Scroll: %d at NT %d\n", PPUSCROLL_y, nts);

			//	select the correct byte of the attribute table
			tile_attr_nr = getAttribute(scrolled_address);

			//	select the part of the byte that we need (2-bits)
			attr_shift = getAttributeTilePart(scrolled_address);
			palette_offset = ((tile_attr_nr >> attr_shift) & 0x3) * 4;

			pixel = ((rdV(adr) >> (7 - ((col + PPUSCROLL_x % 8) % 8))) & 1) + (((rdV(adr + 8) >> (7 - ((col + PPUSCROLL_x % 8) % 8))) & 1) * 2);

			framebuffer[(r * 256 * 3) + (col * 3)] = (PALETTE[rdV(fixPalette((0x3f00 + palette_offset + pixel) & 0xff0f))] >> 16) & 0xff;
			framebuffer[(r * 256 * 3) + (col * 3) + 1] = (PALETTE[rdV(fixPalette((0x3f00 + palette_offset + pixel) & 0xff0f))] >> 8) & 0xff;
			framebuffer[(r * 256 * 3) + (col * 3) + 2] = (PALETTE[rdV(fixPalette((0x3f00 + palette_offset + pixel) & 0xff0f))]) & 0xff;

		}
	}

	if (PPU_MASK.show_sprites) {
		for (int i = 63; i >= 0; i--) {
			Y_Pos = OAM[i * 4] + 1;
			Tile_Index_Nr = OAM[i * 4 + 1];
			Attributes = OAM[i * 4 + 2];
			X_Pos = OAM[i * 4 + 3];
			Palette_Offset = 0x3f10 + ((Attributes & 3) * 4);
			if (r >= Y_Pos && (Y_Pos + 8) > r && Y_Pos) {
				//	iterate through 8x8 sprite in Pattern Table, with offset of Y_Pos and X_Pos
				int j = r - Y_Pos;
				for (int t = 0; t < 8; t++) {
					uint8_t V = 0x00;
					switch ((Attributes >> 6) & 3) {
					case 0x00:	//	no flip
						V = ((rdV(PPU_CTRL.sprite_pattern_table_adr_value + Tile_Index_Nr * 0x10 + j) >> (7 - (t % 8))) & 1) + ((rdV(PPU_CTRL.sprite_pattern_table_adr_value + Tile_Index_Nr * 0x10 + j + 8) >> (7 - (t % 8))) & 1) * 2;
						break;
					case 0x01:	//	horizontal flip
						V = ((rdV(PPU_CTRL.sprite_pattern_table_adr_value + Tile_Index_Nr * 0x10 + j) >> (t % 8)) & 1) + ((rdV(PPU_CTRL.sprite_pattern_table_adr_value + Tile_Index_Nr * 0x10 + j + 8) >> (t % 8)) & 1) * 2;
						break;
					case 0x02:	//	vertical flip
						V = ((rdV(PPU_CTRL.sprite_pattern_table_adr_value + Tile_Index_Nr * 0x10 + (7 - j)) >> (7 - (t % 8))) & 1) + ((rdV(PPU_CTRL.sprite_pattern_table_adr_value + Tile_Index_Nr * 0x10 + (7 - j) + 8) >> (7 - (t % 8))) & 1) * 2;
						break;
					case 0x03:	//	horizontal & vertical flip
						V = ((rdV(PPU_CTRL.sprite_pattern_table_adr_value + Tile_Index_Nr * 0x10 + (7 - j)) >> (t % 8)) & 1) + ((rdV(PPU_CTRL.sprite_pattern_table_adr_value + Tile_Index_Nr * 0x10 + (7 - j) + 8) >> (t % 8)) & 1) * 2;
						break;
					}

					uint8_t R = (PALETTE[rdV(Palette_Offset + V)] >> 16) & 0xff;
					uint8_t G = (PALETTE[rdV(Palette_Offset + V)] >> 8) & 0xff;
					uint8_t B = PALETTE[rdV(Palette_Offset + V)] & 0xff;

					if (V) {
						//if (!((Attributes >> 5) & 1)) {
							//	when drawing "+1" is needed for the Y-Position, it's a quirk of the N64 because of the prep-scanline
							framebuffer[((Y_Pos + j) * 256 * 3) + ((X_Pos + t) * 3)] = R;
							framebuffer[((Y_Pos + j) * 256 * 3) + ((X_Pos + t) * 3) + 1] = G;
							framebuffer[((Y_Pos + j) * 256 * 3) + ((X_Pos + t) * 3) + 2] = B;
						//}

						//	sprite zero hit
							if (!PPU_STATUS.isSpriteZero() && i == 0) {
								PPU_STATUS.setSpriteZero();
							}
					}
						
				}
			}
		}
	}
}

void stepPPU() {

	//	stepPPU
	ppuCycles++;
	if (ppuCycles > 340) {
		ppuCycles -= 341;
		ppuScanline++;
	}
	if (0 <= ppuScanline && ppuScanline <= 239) {		//	drawing
		if (ppuCycles == 250) {
			renderScanline(ppuScanline);
		}
	}
	else if (ppuScanline == 241 && ppuCycles == 1) {	//	VBlank 
		//printf("VBlanking!\n");
		drawFrame();
		PPU_STATUS.setVBlank();
		if (PPU_CTRL.generate_nmi) {
			NMI_occured = true;
			NMI_output = true;
		}
	}
	else if (ppuScanline == 261) {
		//	VBlank off / pre-render line
		if (ppuCycles == 1) {
			PPU_STATUS.clearVBlank();
			PPU_STATUS.clearSpriteZero();
			NMI_occured = false;
		}
		else if (ppuCycles >= 280 && ppuCycles <= 304) {
			if (PPU_MASK.show_bg || PPU_MASK.show_sprites) {
				PPUSCROLL_y = tmp_PPUSCROLL_y;
				PPUSCROLL_fine_y = tmp_PPUSCROLL_fine_y;
			}
		}
		else if(ppuCycles == 340)
			ppuScanline = 0;
	}
}

void stopNMI() {
	NMI_occured = false;
	NMI_output = false;
}