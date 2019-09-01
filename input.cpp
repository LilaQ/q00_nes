#include <stdint.h>
#include "SDL2/include/SDL.h"
#include <iostream>

uint8_t a1, b1, select1, start1, up1, down1, left1, right1;

void resetController1() {
	a1 = 0;
	b1 = 0;
	select1 = 0;
	start1 = 0;
	up1 = 0;
	down1 = 0;
	left1 = 0;
	right1 = 0;
}

void setController1(uint8_t *SDL_keys) {
	resetController1();
	if (SDL_keys[SDL_SCANCODE_J])
		a1 = 1;
	if (SDL_keys[SDL_SCANCODE_K])
		b1 = 1;
	if (SDL_keys[SDL_SCANCODE_S])
		down1 = 1;
	if (SDL_keys[SDL_SCANCODE_A])
		left1 = 1;
	if (SDL_keys[SDL_SCANCODE_D])
		right1 = 1;
	if (SDL_keys[SDL_SCANCODE_W])
		up1 = 1;
	if (SDL_keys[SDL_SCANCODE_Q])
		select1 = 1;
	if (SDL_keys[SDL_SCANCODE_E])
		start1 = 1;
}

uint8_t readController1(uint8_t bit) {
	switch (bit) {
	case 0:
		return a1;
		break;
	case 1:
		return b1;
		break;
	case 2:
		return select1;
		break;
	case 3:
		return start1;
		break;
	case 4:
		return up1;
		break;
	case 5:
		return down1;
		break;
	case 6:
		return left1;
		break;
	case 7:
		return right1;
		break;
	default:
		return 0;
		break;
	}
}