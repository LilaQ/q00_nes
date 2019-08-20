#pragma once
#include <stdint.h>

struct Registers
{
	unsigned char A;
	unsigned char X;
	unsigned char Y;
};

struct Status
{
	uint8_t status = 0x24;	//	TODO usually 0x34 on power up 
	uint8_t carry;
	uint8_t zero;
	uint8_t interruptDisable;
	uint8_t decimal;
	uint8_t overflow;
	uint8_t negative;
	uint8_t brk;

	void setCarry(int v) {
		carry = v;
		status = (status & ~1) | carry;
	}

	void setZero(int v) {
		zero = v;
		status = (status & ~2) | (zero << 1);
	}

	void setInterruptDisable(int v) {
		interruptDisable = v;
		status = (status & ~4) | (interruptDisable << 2);
	}

	void setDecimal(int v) {
		decimal = v;
		status = (status & ~8) | (decimal << 3);
	}

	void setOverflow(int v) {
		overflow = v;
		status = (status & ~64) | (overflow << 6);
	}

	void setNegative(int v) {
		negative = v;
		status = (status & ~128) | (negative << 7);
	}

	void setBrk(int v) {
		brk = v;
		status = (status & ~16) | (brk << 4);
	}

	void setStatus(uint8_t status) {
		this->status = status | 0x20;
		this->carry = status & 1;
		this->zero = (status >> 1) & 1;
		this->interruptDisable = (status >> 2) & 1;
		this->decimal = (status >> 3) & 1;
		this->brk = (status >> 4) & 1;
		this->overflow = (status >> 6) & 1;
		this->negative = (status >> 7) & 1;
	}

};

int stepCPU();
void resetCPU();