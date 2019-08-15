#include "cpu.h"
#include "mmu.h"
#include "main.h"
#include <map>

//	set up vars
uint16_t PC = 0xc000;
uint8_t SP_ = 0xfd;
Registers registers;
Status status;

//	set up jump map
//typedef uint8_t(*FuncPtr)(ADDRESSING_MODE mode);
//std::map<int, FuncPtr> opcodeTable;
//FuncPtr opcodeTable[256];


uint8_t LDX(unsigned char &tar, uint16_t adr, uint8_t cycles) { 
	tar = readFromMem(adr);
	status.setZero(tar == 0);
	status.setNegative(tar >> 7);
	return cycles; 
}
uint8_t STX(uint16_t adr, uint8_t val, uint8_t cycles) {
	writeToMem(adr, val);
	return cycles;
}
uint8_t AND(uint16_t adr, uint8_t cycles) {
	registers.A &= readFromMem(adr);
	status.setZero(registers.A == 0);
	status.setNegative(registers.A >> 7);
	return cycles;
}
uint8_t ORA(uint16_t adr, uint8_t cycles) {
	registers.A |= readFromMem(adr);
	status.setZero(registers.A == 0);
	status.setNegative(registers.A >> 7);
	return cycles;
}
uint8_t EOR(uint16_t adr, uint8_t cycles) {
	registers.A ^= readFromMem(adr);
	status.setZero(registers.A == 0);
	status.setNegative(registers.A >> 7);
	return cycles;
}
uint8_t ADD(uint8_t val, uint8_t cycles) {
	//	TODO overflow flag und bcd bei decimal flag
	uint16_t sum = registers.A + val + status.carry;
	status.setOverflow((~(registers.A ^ val) & (registers.A ^ sum) & 0x80) > 0);
	status.setCarry(sum > 0xff);
	registers.A = sum;
	status.setZero(registers.A == 0);
	status.setNegative(registers.A >> 7);
	return cycles;
}
uint8_t ADC(uint16_t adr, uint8_t cycles) {
	return ADD(readFromMem(adr), cycles);
}
uint8_t SBC(uint16_t adr, uint8_t cycles) {
	return ADD(~readFromMem(adr), cycles);
}
uint8_t INC(uint16_t adr, uint8_t cycles) {
	writeToMem(adr, readFromMem(adr) + 1);
	status.setZero(readFromMem(adr) == 0);
	status.setNegative(readFromMem(adr) >> 7);
	return cycles;
}
uint8_t DEC(uint16_t adr, uint8_t cycles) {
	writeToMem(adr, readFromMem(adr) - 1);
	status.setZero(readFromMem(adr) == 0);
	status.setNegative(readFromMem(adr) >> 7);
	return cycles;
}
uint8_t ASLA(uint8_t cycles) {
	status.setCarry(registers.A >> 7);
	registers.A = (registers.A << 1);
	status.setZero(registers.A == 0);
	status.setNegative(registers.A >> 7);
	return cycles;
}
uint8_t ASL(uint16_t adr, uint8_t cycles) {
	status.setCarry(readFromMem(adr) >> 7);
	writeToMem(adr, (readFromMem(adr) << 1));
	status.setZero(readFromMem(adr) == 0);
	status.setNegative(readFromMem(adr) >> 7);
	return cycles;
}
uint8_t LSRA(uint8_t cycles) {
	status.setCarry(registers.A & 1);
	registers.A = (registers.A >> 1);
	status.setZero(registers.A == 0);
	status.setNegative(registers.A >> 7);
	return cycles;
}
uint8_t LSR(uint16_t adr, uint8_t cycles) {
	status.setCarry(readFromMem(adr) & 1);
	writeToMem(adr, (readFromMem(adr) >> 1));
	status.setZero(readFromMem(adr) == 0);
	status.setNegative(readFromMem(adr) >> 7);
	return cycles;
}
uint8_t ROLA(uint8_t cycles) {
	int tmp = status.carry;
	status.setCarry(registers.A >> 7);
	registers.A = (registers.A << 1) | tmp;
	status.setZero(registers.A == 0);
	status.setNegative(registers.A >> 7);
	return cycles;
}
uint8_t ROL(uint16_t adr, uint8_t cycles) {
	int tmp = status.carry;
	status.setCarry(readFromMem(adr) >> 7);
	writeToMem(adr, (readFromMem(adr) << 1) | tmp);
	status.setZero(readFromMem(adr) == 0);
	status.setNegative(readFromMem(adr) >> 7);
	return cycles;
}
uint8_t RORA(uint8_t cycles) {
	int tmp = registers.A & 1;
	registers.A = (registers.A >> 1) | (status.carry << 7);
	status.setCarry(tmp);
	status.setZero(registers.A == 0);
	status.setNegative(registers.A >> 7);
	return cycles;
}
uint8_t ROR(uint16_t adr, uint8_t cycles) {
	int tmp = readFromMem(adr) & 1;
	writeToMem(adr, (readFromMem(adr) << 1) | (status.carry << 7));
	status.setCarry(tmp);
	status.setZero(readFromMem(adr) == 0);
	status.setNegative(readFromMem(adr) >> 7);
	return cycles;
}
uint8_t CMP(uint8_t tar, uint16_t adr, uint8_t cycles) {
	status.setCarry(tar >= readFromMem(adr));
	status.setZero(readFromMem(adr) == tar);
	status.setNegative((tar - readFromMem(adr)) >> 7);
	return cycles;
}
uint8_t BIT(uint16_t adr, uint8_t cycles) {
	status.setNegative(readFromMem(adr) >> 7);           
	status.setOverflow((readFromMem(adr) >> 6) & 1);
	status.setZero((registers.A & readFromMem(adr)) == 0);
	return cycles;
}

int c = 7;
int j = 0;
int f = 0;
int r = 0; //	don't delete, return val holder
int stepCPU() {
	c += getLastCyc();
	j += getLastCyc() * 3;
	if (j >= 341) {
		j -= 341;
		f++;
	}
	printf("%04x A:%02x X:%02x Y:%02x P:%02x SP:%02x PPU:%3d,%3d CYC:%d\n", PC, registers.A, registers.X, registers.Y, status.status, SP_, j, f, c);
	switch (readFromMem(PC)) {
		case 0x00: { PC++; status.setBrk(1); return 7; break; }
		case 0x01: { PC++; return ORA(getIndirectXIndex(PC++, registers.X), 6); break; }
		case 0x05: { PC++; return ORA(getZeropage(PC++), 3); break; }
		case 0x06: { PC++; return ASL(getZeropage(PC++), 6); break; }
		case 0x08: { PC++; writeToMem(SP_ + 0x100, status.status | 0x30); SP_--; return 3; break; }
		case 0x09: { PC++; return ORA(getImmediate(PC++), 2); break; }
		case 0x0a: { PC++; return ASLA(2); PC++; break; }
		case 0x0d: { PC++; r = ORA(getAbsolute(PC), 4); PC += 2; return r; break; }
		case 0x0e: { PC++; r = ASL(getAbsolute(PC), 6); PC += 2; return r; break; }
		case 0x10: { if (!status.negative) { PC += 2 + (int8_t)readFromMem(PC + 1); return 3; } else { PC += 2; return 2; } break; }		//	TODO +1 cyc if page boundary is crossed
		case 0x11: { PC++; return ORA(getIndirectYIndex(PC++, registers.Y), 6); break; }		//	TODO +1 cyc if page boundary is crossed
		case 0x15: { PC++; return ORA(getZeropageXIndex(PC++, registers.X), 4); break; }
		case 0x16: { PC++; return ASL(getZeropageXIndex(PC++, registers.X), 6); break; }
		case 0x18: { PC++; status.setCarry(0); return 2; break; }
		case 0x19: { PC++; r = ORA(getAbsoluteYIndex(PC, registers.Y), 4); PC += 2; return r; break; }		//	TODO +1 cyc if page boundary is crossed
		case 0x1d: { PC++; r = ORA(getAbsoluteXIndex(PC, registers.X), 4); PC += 2; return r; break; }		//	TODO +1 cyc if page boundary is crossed
		case 0x1e: { PC++; r = ASL(getAbsoluteXIndex(PC, registers.X), 7); PC += 2; return r; break; }
		case 0x20: { writeToMem(SP_ + 0x100, (PC + 2) >> 8); SP_--; writeToMem(SP_ + 0x100, (PC + 2) & 0xff); SP_--; PC = getAbsolute(PC+1); return 6; break; }
		case 0x21: { PC++; return AND(getIndirectXIndex(PC, registers.X), 6); PC++; break; }
		case 0x24: { PC++; return BIT(getZeropage(PC++), 3); break; }
		case 0x25: { PC++; return AND(getZeropage(PC++), 3); break; }
		case 0x26: { PC++; return ROL(getZeropage(PC++), 5); break; }
		case 0x28: { PC++; SP_++; status.setStatus(readFromMem(SP_ + 0x100) & 0xef); return 4; break; }
		case 0x29: { PC++; return AND(getImmediate(PC++), 2); break; }
		case 0x2a: { PC++; return ROLA(2); PC++; break; }
		case 0x2c: { PC++; r = BIT(getAbsolute(PC), 4); PC += 2; return r; break; }
		case 0x2d: { PC++; r = AND(getAbsolute(PC), 4); PC += 2; return r; break; }
		case 0x2e: { PC++; r = ROL(getAbsolute(PC), 6); PC += 2; return r; break; }
		case 0x30: { if (status.negative) { PC += 2 + (int8_t)readFromMem(PC + 1); return 3; } else { PC += 2; return 2; } break; }		//	TODO +1 cyc if page boundary is crossed
		case 0x31: { PC++; return AND(getIndirectYIndex(PC++, registers.Y), 5); break; }		//	TODO +1 cyc if page boundary is crossed
		case 0x35: { PC++; return AND(getZeropageXIndex(PC++, registers.X), 4); break; }
		case 0x36: { PC++; return ROL(getZeropageXIndex(PC++, registers.X), 6); break; }
		case 0x38: { PC++; status.setCarry(1); return 2; break; }
		case 0x39: { PC++; r = AND(getAbsoluteYIndex(PC, registers.Y), 4); PC += 2; return r; break; }		//	TODO +1 cyc if page boundary is crossed
		case 0x3d: { PC++; r = AND(getAbsoluteXIndex(PC, registers.X), 4); PC += 2; return r; break; }		//	TODO +1 cyc if page boundary is crossed
		case 0x3e: { PC++; r = ROL(getAbsoluteXIndex(PC, registers.X), 7); PC += 2; return r; break; }
		case 0x40: { SP_++; status.setStatus(readFromMem(SP_ + 0x100)); SP_++; PC = readFromMem(SP_ + 0x100); SP_++; PC |= (readFromMem(SP_ + 0x100) << 8); break; }
		case 0x41: { PC++; return EOR(getIndirectXIndex(PC, registers.X), 6); PC++; break; }
		case 0x45: { PC++; return EOR(getZeropage(PC++), 3); break; }
		case 0x46: { PC++; return LSR(getZeropage(PC++), 5); break; }
		case 0x48: { PC++; writeToMem(SP_ + 0x100, registers.A); SP_--; return 3; break; }
		case 0x49: { PC++; return EOR(getImmediate(PC++), 2); break; }
		case 0x4a: { PC+=2; return LSRA(2); break; }
		case 0x4c: { PC++; PC = getAbsolute(PC); return 3; break; }
		case 0x4d: { PC++; r = EOR(getAbsolute(PC), 4); PC += 2; return r; break; }
		case 0x4e: { PC++; r = LSR(getAbsolute(PC), 2); PC += 2; return r; break; }
		case 0x50: { if (!status.overflow) { PC += 2 + (int8_t)readFromMem(PC + 1); return 3; } else { PC += 2; return 2; } break; }		//	TODO +1 cyc if page boundary is crossed
		case 0x51: { PC++; return EOR(getIndirectYIndex(PC++, registers.Y), 5); break; }		//	TODO +1 cyc if page boundary is crossed
		case 0x55: { PC++; return EOR(getZeropageXIndex(PC++, registers.X), 4); break; }
		case 0x56: { PC++; return LSR(getZeropageXIndex(PC++, registers.X), 6); break; }
		case 0x58: { PC++; status.setInterruptDisable(0); return 2; break; }
		case 0x59: { PC++; r = EOR(getAbsoluteYIndex(PC, registers.Y), 4); PC += 2; return r; break; }		//	TODO +1 cyc if page boundary is crossed
		case 0x5d: { PC++; r = EOR(getAbsoluteXIndex(PC, registers.X), 4); PC += 2; return r; break; }		//	TODO +1 cyc if page boundary is crossed
		case 0x5e: { PC++; r = LSR(getAbsoluteXIndex(PC, registers.X), 7); PC += 2; return r; break; }
		case 0x60: { SP_++; PC = readFromMem(SP_ + 0x100); SP_++; PC |= readFromMem(SP_ + 0x100) << 8; PC++; return 6;  break; }
		case 0x61: { PC++; return ADC(getIndirectXIndex(PC++, registers.X), 6); break; }
		case 0x65: { PC++; return ADC(getZeropage(PC++), 3); break; }
		case 0x66: { PC++; return ROR(getZeropage(PC++), 5); break; }
		case 0x68: { PC++; SP_++; registers.A = readFromMem(SP_ + 0x100); status.setNegative(registers.A >> 7); status.setZero(registers.A == 0); return 4; break; }
		case 0x69: { PC++; return ADC(getImmediate(PC++), 2); break; }
		case 0x6a: { PC+=2; return RORA(2); break; }
		case 0x6c: { PC = getIndirect(PC); break; }
		case 0x6d: { PC++; return ADC(getAbsolute(PC++), 4); break; }
		case 0x6e: { PC++; return ROR(getAbsolute(PC++), 2); break; }
		case 0x70: { if (status.overflow) { PC += 2 + (int8_t)readFromMem(PC + 1); return 3; } else { PC += 2; return 2; } break; }		//	TODO +1 cyc if page boundary is crossed
		case 0x71: { PC++; return ADC(getIndirectYIndex(PC++, registers.Y), 5); break; }		//	TODO +1 cyc if page boundary is crossed
		case 0x75: { PC++; return ADC(getZeropageXIndex(PC++, registers.X), 4); break; }
		case 0x76: { PC++; return ROR(getZeropageXIndex(PC++, registers.X), 6); break; }
		case 0x78: { PC++; status.setInterruptDisable(1); return 2; break; }
		case 0x79: { PC++; r = ADC(getAbsoluteYIndex(PC, registers.Y), 4); PC += 2; return r; break; }		//	TODO +1 cyc if page boundary is crossed
		case 0x7d: { PC++; r = ADC(getAbsoluteXIndex(PC, registers.X), 4); PC += 2; return r; break; }		//	TODO +1 cyc if page boundary is crossed
		case 0x7e: { PC++; r = ROR(getAbsoluteXIndex(PC, registers.X), 7); PC += 2; return r; break; }
		case 0x81: { PC++; return STX(getIndirectXIndex(PC++, registers.X), registers.A, 6); break; }
		case 0x84: { PC++; return STX(getZeropage(PC++), registers.Y, 3); break; }
		case 0x85: { PC++; return STX(getZeropage(PC++), registers.A, 3); break; }
		case 0x86: { PC++; return STX(getZeropage(PC++), registers.X, 3); break; }
		case 0x88: { PC++; registers.Y--; status.setZero(registers.Y == 0); status.setNegative(registers.Y >> 7); return 2; break; }				//	DEY
		case 0x8a: { PC++; registers.A = registers.X; status.setZero(registers.A == 0);	status.setNegative(registers.A >> 7); return 2; break; }		//	TXA
		case 0x8c: { PC++; r = STX(getAbsolute(PC), registers.Y, 4); PC += 2; return r; break; }
		case 0x8d: { PC++; r = STX(getAbsolute(PC), registers.A, 4); PC += 2; return r; break; }
		case 0x8e: { PC++; r = STX(getAbsolute(PC), registers.X, 4); PC += 2; return r; break; }
		case 0x90: { if (!status.carry) { PC += 2 + (int8_t)readFromMem(PC + 1); return 3; } else { PC += 2; return 2; } break; }		//	TODO +1 cyc if page boundary is crossed
		case 0x91: { PC++; return STX(getIndirectYIndex(PC++, registers.Y), registers.A, 4); break; }
		case 0x94: { PC++; return STX(getZeropageXIndex(PC++, registers.X), registers.Y, 4); break; }
		case 0x95: { PC++; return STX(getZeropageXIndex(PC++, registers.X), registers.A, 4); break; }
		case 0x96: { PC++; return STX(getZeropageYIndex(PC++, registers.Y), registers.X, 4); break; }
		case 0x98: { PC++; registers.A = registers.Y; status.setZero(registers.A == 0);	status.setNegative(registers.A >> 7); return 2; break; }		//	TYA
		case 0x99: { PC++; return STX(getAbsoluteYIndex(PC, registers.Y), registers.A, 4); PC += 2; break; }
		case 0x9a: { PC++; SP_ = registers.X; return 2; break; }					//	TSX
		case 0x9d: { PC++; return STX(getAbsoluteXIndex(PC, registers.X), registers.A, 5); PC += 2; break; }
		case 0xa0: { PC++; return LDX(registers.Y, getImmediate(PC++), 2); break; }
		case 0xa1: { PC++; return LDX(registers.A, getIndirectXIndex(PC++, registers.X), 6); break; }
		case 0xa2: { PC++; return LDX(registers.X, getImmediate(PC++), 2); break; }
		case 0xa4: { PC++; return LDX(registers.Y, getZeropage(PC++), 3); break; }
		case 0xa5: { PC++; return LDX(registers.A, getZeropage(PC++), 3); break; }
		case 0xa6: { PC++; return LDX(registers.X, getZeropage(PC++), 3); break; }
		case 0xa8: { PC++; registers.Y = registers.A; status.setZero(registers.A == 0);	status.setNegative(registers.A >> 7); return 2; break; }		//	TAY
		case 0xa9: { PC++; return LDX(registers.A, getImmediate(PC++), 2); break; }
		case 0xaa: { PC++; registers.X = registers.A; status.setZero(registers.A == 0);	status.setNegative(registers.A >> 7); return 2; break; }		//	TAX
		case 0xac: { PC++; r = LDX(registers.Y, getAbsolute(PC), 4); PC += 2; return r; break; }
		case 0xad: { PC++; r = LDX(registers.A, getAbsolute(PC), 4); PC += 2; return r; break; }
		case 0xae: { PC++; r = LDX(registers.X, getAbsolute(PC), 4); PC += 2; return r; break; }
		case 0xb0: { if (status.carry) { PC += 2 + (int8_t)readFromMem(PC + 1); return 3; } else { PC += 2; return 2; } break; }		//	TODO +1 cyc if page boundary is crossed
		case 0xb1: { PC++; return LDX(registers.A, getIndirectYIndex(PC++, registers.Y), 5); break; }		//	TODO +1 cyc if page boundary is crossed
		case 0xb4: { PC++; return LDX(registers.Y, getZeropageXIndex(PC++, registers.X), 4); break; }
		case 0xb5: { PC++; return LDX(registers.A, getZeropageXIndex(PC++, registers.X), 4); break; }
		case 0xb6: { PC++; return LDX(registers.X, getZeropageYIndex(PC++, registers.Y), 4); break; }
		case 0xb8: { PC++; status.setOverflow(0); return 2; break; }
		case 0xb9: { PC++; r = LDX(registers.A, getAbsoluteYIndex(PC, registers.Y), 4); PC += 2; return r; break; }	//	TODO +1 cyc if page boundary is crossed		   
		case 0xba: { PC++; registers.X = SP_; status.setZero(registers.X == 0);	status.setNegative(registers.X >> 7); return 2; break; }				//	TSX
		case 0xbc: { PC++; r = LDX(registers.Y, getAbsoluteXIndex(PC, registers.X), 4); PC += 2; return r; break; }	//	TODO +1 cyc if page boundary is crossed		   
		case 0xbd: { PC++; r = LDX(registers.A, getAbsoluteXIndex(PC, registers.X), 4); PC += 2; return r; break; }	//	TODO +1 cyc if page boundary is crossed
		case 0xbe: { PC++; r = LDX(registers.X, getAbsoluteYIndex(PC, registers.Y), 4); PC += 2; return r; break; }
		case 0xc0: { PC++; return CMP(registers.Y, getImmediate(PC++), 2); break; }
		case 0xc1: { PC++; return CMP(registers.A, getIndirectXIndex(PC++, registers.X), 6); break; }
		case 0xc4: { PC++; return CMP(registers.Y, getZeropage(PC++), 3); break; }
		case 0xc5: { PC++; return CMP(registers.A, getZeropage(PC++), 3); break; }
		case 0xc6: { PC++; return DEC(getZeropage(PC++), 5); break; }
		case 0xc8: { PC++; registers.Y++; status.setZero(registers.Y == 0); status.setNegative(registers.Y >> 7); return 2; break; }				//	INY
		case 0xc9: { PC++; return CMP(registers.A, getImmediate(PC++), 2); break; }
		case 0xca: { PC++; registers.X--; status.setZero(registers.X == 0); status.setNegative(registers.X >> 7); return 2; break; }				//	DEX
		case 0xcc: { PC++; r = CMP(registers.Y, getAbsolute(PC), 2); PC += 2; return r; break; }
		case 0xcd: { PC++; r = CMP(registers.A, getAbsolute(PC), 4); PC += 2; return r; break; }
		case 0xce: { PC++; return DEC(getAbsolute(PC), 6); PC += 2; break; }
		case 0xd0: { if (!status.zero) { PC += 2 + (int8_t)readFromMem(PC + 1); return 3; } else { PC += 2; return 2; } break; }		//	TODO +1 cyc if page boundary is crossed
		case 0xd1: { PC++; return CMP(registers.A, getIndirectYIndex(PC++, registers.Y), 5); break; }	//	TODO +1 cyc if page boundary is crossed
		case 0xd5: { PC++; return CMP(registers.A, getZeropageXIndex(PC++, registers.X), 4); break; }
		case 0xd6: { PC++; r = DEC(getZeropageXIndex(PC, registers.X), 6); PC += 2; return r; break; }
		case 0xd8: { PC++; status.setDecimal(0); return 2; break; }
		case 0xd9: { PC++; r = CMP(registers.A, getAbsoluteYIndex(PC, registers.Y), 4); PC += 2; return r; break; }	//	TODO +1 cyc if page boundary is crossed
		case 0xdd: { PC++; r = CMP(registers.A, getAbsoluteXIndex(PC, registers.X), 4); PC += 2; return r; break; }	//	TODO +1 cyc if page boundary is crossed
		case 0xde: { PC++; r = DEC(getAbsoluteXIndex(PC, registers.X), 7); PC += 2; break; }
		case 0xe0: { PC++; return CMP(registers.X, getImmediate(PC++), 2); break; }
		case 0xe1: { PC++; return SBC(getIndirectXIndex(PC++, registers.X), 6); break; }
		case 0xe4: { PC++; return CMP(registers.X, getZeropage(PC++), 3); break; }
		case 0xe5: { PC++; return SBC(getZeropage(PC++), 3); break; }
		case 0xe6: { PC++; return INC(getZeropage(PC++), 5); break; }
		case 0xe8: { PC++; registers.X++; status.setZero(registers.X == 0); status.setNegative(registers.X >> 7); return 2; break; }				//	INX
		case 0xe9: { PC++; return SBC(getImmediate(PC++), 2); break; }
		case 0xea: { PC++; return 2; break; }
		case 0xec: { PC++; r = CMP(registers.X, getAbsolute(PC), 4); PC += 2; return r; break; }
		case 0xed: { PC++; r = SBC(getAbsolute(PC), 4); PC += 2; return r; break; }
		case 0xee: { PC++; r = INC(getAbsolute(PC), 6); PC += 2; return r; break; }
		case 0xf0: { if (status.zero) { PC += 2 + (int8_t)readFromMem(PC + 1); return 3; } else { PC += 2; return 2; } break; }		//	TODO +1 cyc if page boundary is crossed
		case 0xf1: { PC++; return SBC(getIndirectYIndex(PC++, registers.Y), 5); break; }	//	TODO +1 cyc if page boundary is crossed
		case 0xf5: { PC++; return SBC(getZeropageXIndex(PC++, registers.X), 4); break; }
		case 0xf6: { PC++; return INC(getZeropageXIndex(PC++, registers.X), 6); break; }
		case 0xf8: { PC++; status.setDecimal(1); return 2; break; }
		case 0xf9: { PC++; r = SBC(getAbsoluteYIndex(PC, registers.Y), 4); PC += 2; return r; break; }	//	TODO +1 cyc if page boundary is crossed
		case 0xfd: { PC++; r = SBC(getAbsoluteXIndex(PC, registers.X), 4); PC += 2; return r; break; }	//	TODO +1 cyc if page boundary is crossed
		case 0xfe: { PC++; r = INC(getAbsoluteXIndex(PC, registers.X), 7); PC += 2; return r; break; }
		
		
		default:
			printf("ERROR! Unimplemented opcode 0x%02x at address 0x%04x !\n", readFromMem(PC), PC);
			std::exit(1);
			break;
	}
	
	return 0;
}