#pragma once
#include <stdint.h>
void initAPU();
void stepAPU(unsigned char cycles);
void resetSC1length(uint8_t val);
void resetSC2length(uint8_t val);

