#ifndef I8080_EMULATOR_H
#define I8080_EMULATOR_H

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// Struct defining condition codes.
typedef struct ConditionCodes {
  uint8_t  z  : 1; // zero
  uint8_t  s  : 1; // sign
  uint8_t  p  : 1; // parity
  uint8_t  cy : 1; // carry
  uint8_t  ac : 1; // auxillary carry
  uint8_t  pad: 3;
} ConditionCodes;


typedef struct {
  uint8_t  a;
  uint8_t  b;
  uint8_t  c;
  uint8_t  d; 
  
  uint8_t  e;
  uint8_t  h;
  uint8_t  l;
  uint16_t sp;
  uint16_t pc;
  uint8_t  *memory;
  struct   ConditionCodes     cc;
  uint8_t  int_enable;
} CpuState;

typedef uint8_t UWord8;

typedef union {

  uint16_t value;
  
  struct
  {
    UWord8 l;
    UWord8 h;
  };

} UWord16;

void Emulate8080Op(CpuState* state);
int Emulate8080Op_ref(CpuState* state);

CpuState* Init8080(void);

void ReadFileIntoMemoryAt(CpuState* state, char* filename, uint32_t offset);

int compare_states(CpuState* state1, CpuState* state2);
void UnimplementedInstruction(CpuState* state);
#endif /* I8080_EMULATOR_H */
