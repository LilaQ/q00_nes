#include <stdint.h>
#include <stdio.h>
#include "SDL2/include/SDL.h"
#include "mmu.h"
#include "ppu.h"

//	init PPU
unsigned char VRAM[0x4000];
const int FB_SIZE = 256 * 224 * 3;
SDL_Renderer* renderer;
SDL_Window* window;
SDL_Texture* texture;
SDL_Event event;
unsigned char framebuffer[FB_SIZE];		//	3 bytes per pixel, RGB24
unsigned char framebuffer_bg[FB_SIZE];	//	3 bytes per pixel, RGB24
unsigned char framebuffer_pt[256 * 128 * 3];	//	3 bytes per pixel, RGB24
uint8_t SCL = 0;
uint8_t ppuStep = 0;
uint16_t ppuAddr = 0;

//	BG
uint16_t BG_VRAMadr;
uint16_t BG_VRAMtempAdr;
uint8_t BG_FineXScroll;
uint8_t BG_FirstWriteToggle;
uint16_t BG_PatternTableShiftRegisterH;
uint16_t BG_PatternTableHLatch;
uint16_t BG_PatternTableShiftRegisterL;
uint16_t BG_PatternTableLLatch;
uint8_t BG_NTShiftRegisterH;
uint8_t BG_NTHLatch;
uint8_t BG_ATShiftRegisterL;
uint8_t BG_ATLLatch;

//	Registers
PPUCTRL PPU_CTRL;
uint16_t PPUADDR = 0x0;
bool NMI_occured = false;
bool NMI_output = false;

//	PPUCTRL write
void writePPUCTRL(uint8_t val) {
	PPU_CTRL.setValue(val);
	NMI_output = PPU_CTRL.generate_nmi > 0;
}

//	PPUADDR write
void writePPUADDR(uint16_t adr, uint8_t cycle_nr) {
	printf("VRAM PPUADDR write at: 0x%04x\n", PPUADDR);
	if (cycle_nr == 0) {
		PPUADDR = adr << 8;
	}
	else {
		PPUADDR |= adr;
	}
}

//	PPUDATA write
void writePPUDATA(uint8_t data) {
	printf("VRAM PPUDATA write at: 0x%04x\n", PPUADDR);
	VRAM[PPUADDR] = data;
	PPUADDR += PPU_CTRL.ppudata_increment_value;
}

//	PPUSTATUS read
uint8_t readPPUSTATUS() {
	return 0x80;
	//	TODO
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
	snprintf(title, sizeof title, "[ CHR tables L/R ]", 0, 0, 0);
	SDL_SetWindowTitle(window_chr, title);

	//	for fast rendering, create a texture
	texture_chr = SDL_CreateTexture(renderer_chr, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 128, 256);

	for (int r = 0; r < 256; r++) {
		for (int col = 0; col < 128; col++) {
			uint16_t adr = (r / 8 * 0x100) + (r % 8) + (col / 8) * 0x10;

			uint8_t pixel = (VRAM[adr] >> (7-(col % 8))) & 1 + ((VRAM[adr + 8] >> (7-(col % 8))) & 1) * 2;
			framebuffer[(r * 128 * 3) + (col * 3)] = COLORS[pixel];
			framebuffer[(r * 128 * 3) + (col * 3) + 1] = COLORS[pixel];
			framebuffer[(r * 128 * 3) + (col * 3) + 2] = COLORS[pixel];
		}
	}

	SDL_UpdateTexture(texture_chr, NULL, framebuffer, 128 * sizeof(unsigned char) * 3);
	SDL_RenderCopy(renderer_chr, texture_chr, NULL, NULL);
	SDL_RenderPresent(renderer_chr);

}

/*uint16_t getNTAddr() {

}

uint16_t getATAddr() {

}

uint16_t getBGAddr() {

}*/

void handleWindowEvents() {
	//	poll events from menu
	SDL_PollEvent(&event);
}

void stepPPU() {

	//drawCHRTable();
	handleWindowEvents();

	//	stepPPU
	/*ppuStep++;
	ppuStep %= 8;
	switch (ppuStep) {
		case 1: ppuAddr = getNTAddr(); break;
		case 2: BG_NTHLatch = readFromMem(ppuAddr);	break;
		case 3: ppuAddr = getATAddr(); break;
		case 4: BG_ATLLatch = readFromMem(ppuAddr);	break;
		case 5: ppuAddr = getBGAddr(); break;
		case 6: BG_PatternTableLLatch = readFromMem(ppuAddr);
		case 7: ppuAddr += 8;
		case 8: BG_PatternTableHLatch = readFromMem(ppuAddr);
	}

	//	V-Blank occurs
	if (SCL >= 241) {
		printf("VBlank\n");
		drawFrame();
	}*/
}