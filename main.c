#include <stdio.h>
#include "disassembler.h"
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
  int pc = 0;
  
  while (pc < fsize)
  {
#ifdef DEBUG_EN
    printf("%d\n", pc);
#endif
    pc += Disassemble8080Op(buffer, pc);
    //pc += 1;
    
    
  }
  return 0;
}
