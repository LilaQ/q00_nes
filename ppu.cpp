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

void initPPU() {
	//	init and create window and renderer
	SDL_Init(SDL_INIT_VIDEO);
	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
	SDL_CreateWindowAndRenderer(256, 128, 0, &window, &renderer);
	SDL_SetWindowSize(window, 1024, 512);
	SDL_RenderSetLogicalSize(renderer, 1024, 512);
	SDL_SetWindowResizable(window, SDL_TRUE);

	//	for fast rendering, create a texture
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 256, 128);
}

int COLORS[] {
	0x00,
	0x55,
	0xaa,
	0xff
};

void drawPatternTable() {

	for (int r = 0; r < 128; r++) {
		for (int col = 0; col < 256; col++) {
			uint16_t adr = (r * 256 * 3) + (col * 3);

			//printf("offset: 0x%04x - offset+8: 0x%04x\n", (r * 256) + col, (r * 256) + col + 8);
			uint8_t pixel = (VRAM[0x2000 + (r * 256) + col] >> (col % 8)) & 1 + ((VRAM[0x2000 + (r * 256) + col + 8] >> (col % 8)) & 1) * 2;

			framebuffer[(r * 256 * 3) + (col * 3)] = COLORS[pixel];
			framebuffer[(r * 256 * 3) + (col * 3) + 1] = COLORS[pixel];
			framebuffer[(r * 256 * 3) + (col * 3) + 2] = COLORS[pixel];
		}
	}

	SDL_UpdateTexture(texture, NULL, framebuffer, 256 * sizeof(unsigned char) * 3);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);

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

	drawPatternTable();
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