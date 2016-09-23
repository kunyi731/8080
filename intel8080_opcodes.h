#ifndef I8080_OPCODE_H
#define I8080_OPCODE_H

#include <stdint.h>
#include "emulator.h"

#define I8080_NUM_OPCODE_ROWS 0x10
#define I8080_NUM_OPCODE_COLS 0x10
#define I8080_NUM_OPCODES (I8080_NUM_OPCODE_ROWS * I8080_NUM_OPCODE_COLS)


/* Data Transfer Group */

uint8_t MOV(CpuState *state);
uint8_t MVI(CpuState *state);
uint8_t LXI(CpuState *state);
uint8_t LDA(CpuState *state);
uint8_t STA(CpuState *state);
uint8_t LHLD(CpuState *state);
uint8_t SHLD(CpuState *state);
uint8_t LDAX(CpuState *state);
uint8_t STAX(CpuState *state);
uint8_t XCHG(CpuState *state);

/* Arithmetic Group */

uint8_t ADD(CpuState *state);
uint8_t ADI(CpuState *state);
uint8_t ADC(CpuState *state);
uint8_t ACI(CpuState *state);
uint8_t SUB(CpuState *state);
uint8_t SUI(CpuState *state);
uint8_t SBB(CpuState *state);
uint8_t SBI(CpuState *state);
uint8_t INR(CpuState *state);
uint8_t DCR(CpuState *state);
uint8_t INX(CpuState *state);
uint8_t DCX(CpuState *state);
uint8_t DAD(CpuState *state);
uint8_t DAA(CpuState *state);

/* Logical Group */

uint8_t ANA(CpuState *state);
uint8_t ANI(CpuState *state);
uint8_t XRA(CpuState *state);
uint8_t XRI(CpuState *state);
uint8_t ORA(CpuState *state);
uint8_t ORI(CpuState *state);
uint8_t CMP(CpuState *state);
uint8_t CPI(CpuState *state);
uint8_t RLC(CpuState *state);
uint8_t RRC(CpuState *state);
uint8_t RAL(CpuState *state);
uint8_t RAR(CpuState *state);
uint8_t CMA(CpuState *state);
uint8_t CMC(CpuState *state);
uint8_t STC(CpuState *state);

/* Branch Group */

uint8_t JMP(CpuState *state);
uint8_t JCOND(CpuState *state);
uint8_t CALL(CpuState *state);
uint8_t CCOND(CpuState *state);
uint8_t RET(CpuState *state);
uint8_t RCOND(CpuState *state);
uint8_t RST(CpuState *state);
uint8_t PCHL(CpuState *state);

/* Stack, I/O and Machine Control Group */

uint8_t PUSH(CpuState *state);
uint8_t POP(CpuState *state);
uint8_t XTHL(CpuState *state);
uint8_t SPHL(CpuState *state);
uint8_t IN(CpuState *state);
uint8_t OUT(CpuState *state);
uint8_t DI(CpuState *state);
uint8_t EI(CpuState *state);
uint8_t HLT(CpuState *state);
uint8_t NOP(CpuState *state);

/* Declare function pointers based on opcodes for Intel 8080 Processor */
uint8_t (* const OpcodeFuncTable[I8080_NUM_OPCODE_ROWS][I8080_NUM_OPCODE_COLS])(CpuState* state);

#endif /* I8080_OPCODE_H */
