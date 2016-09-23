#ifndef I8080_DISASSEMBLER_H
#define I8080_DISASSEMBLER_H

#include <stdint.h>

// Reads memory and disassemble one 8080 op code.
//   codebuffer: pointer to 8080 binary code
//   pc: current offset into the code
//   @return: number of bytes
int Disassemble8080Op(uint8_t *codebuffer, uint16_t pc);

#endif
