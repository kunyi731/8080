#include <stdio.h>
#include <stdlib.h>

#include "emulator.h"

int main (int argc, char** argv)
{
  FILE *f = fopen(argv[1], "rb");
  if (f == NULL)
  {
    printf("error: Couldn't open %s\n", argv[1]);
    exit(1);
  }
  
  // Get the file size and read it into a memory buffer
  fseek(f, 0L, SEEK_END);
  int fsize = ftell(f);
  fseek(f, 0L, SEEK_SET);

  unsigned char *buffer = malloc(fsize);
  
  fread(buffer, fsize, 1, f);
  fclose(f);
  
  // Go through file and execute commands
  int done = 0;
  int vblankcycles = 0;
  CpuState* state = Init8080();
#ifdef DBG_TEST 
  ReadFileIntoMemoryAt(state, "cpudiag.bin", 0x100);

  //Fix the first instruction to be JMP 0x100    
  state->memory[0]=0xc3;    
  state->memory[1]=0;    
  state->memory[2]=0x01;    

  //Fix the stack pointer from 0x6ad to 0x7ad    
  // this 0x06 byte 112 in the code, which is    
  // byte 112 + 0x100 = 368 in memory    
  state->memory[368] = 0x7;    

  //Skip DAA test    
  state->memory[0x59c] = 0xc3; //JMP    
  state->memory[0x59d] = 0xc2;    
  state->memory[0x59e] = 0x05;
#else
  ReadFileIntoMemoryAt(state, "invaders.h", 0);
  ReadFileIntoMemoryAt(state, "invaders.g", 0x800);
  ReadFileIntoMemoryAt(state, "invaders.f", 0x1000);
  ReadFileIntoMemoryAt(state, "invaders.e", 0x1800);
#endif

#ifdef DBG_REF
  CpuState* state_ref = Init8080();
  ReadFileIntoMemoryAt(state_ref, "invaders.h", 0);
  ReadFileIntoMemoryAt(state_ref, "invaders.g", 0x800);
  ReadFileIntoMemoryAt(state_ref, "invaders.f", 0x1000);
  ReadFileIntoMemoryAt(state_ref, "invaders.e", 0x1800);
#endif

  while (1)
  {
    Emulate8080Op(state);
#ifdef DBG_REF
    done = Emulate8080Op_ref(state_ref);
    if (!compare_states(state, state_ref))
    {
      return 1;
    }
#endif
  }
  return 0;
}
