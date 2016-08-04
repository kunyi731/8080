#include <stdio.h>
#include "emulator.h"
#include <stdlib.h>

int main (int argc, char**argv)
{
  FILE *f= fopen(argv[1], "rb");
  if (f==NULL)
  {
      printf("error: Couldn't open %s\n", argv[1]);
      exit(1);
  }
  
  //Get the file size and read it into a memory buffer

  fseek(f, 0L, SEEK_END);
  int fsize = ftell(f);
  fseek(f, 0L, SEEK_SET);


  unsigned char *buffer=malloc(fsize);
  
  fread(buffer, fsize, 1, f);
  fclose(f);

  
  //Go through file and execute commands
  int done = 0;
  int vblankcycles = 0;
  State8080* state = Init8080();
  State8080* state_ref = Init8080();

  ReadFileIntoMemoryAt(state, "invaders.h", 0);
  ReadFileIntoMemoryAt(state, "invaders.g", 0x800);
  ReadFileIntoMemoryAt(state, "invaders.f", 0x1000);
  ReadFileIntoMemoryAt(state, "invaders.e", 0x1800);
  
  ReadFileIntoMemoryAt(state_ref, "invaders.h", 0);
  ReadFileIntoMemoryAt(state_ref, "invaders.g", 0x800);
  ReadFileIntoMemoryAt(state_ref, "invaders.f", 0x1000);
  ReadFileIntoMemoryAt(state_ref, "invaders.e", 0x1800);
  
  while (done == 0)
  {
    done = Emulate8080Op(state);
    done = Emulate8080Op_ref(state_ref);
    if (!compare_states(state, state_ref))
    {
      return 1;
    }
  }
  return 0;
}
