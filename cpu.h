#pragma once
#include <stdint.h>
#include <stdio.h>
void setIRQ(bool v);
void setNMI(bool v);
int NMI();

struct Registers
{
	unsigned char A;
	unsigned char X;
	unsigned char Y;
};

struct Status
{
	uint8_t status = 0x00;	//	TODO usually 0x34 on power up 
	uint8_t carry = 0;
	uint8_t zero = 0;
	uint8_t interruptDisable = 1;
	uint8_t decimal = 0;
	uint8_t overflow = 0;
	uint8_t negative = 0;
	uint8_t brk = 0;

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