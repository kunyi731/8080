#define DBG_REF

#ifdef DBG_REF
#include "emulator.h"
#include "disassembler.h"

ConditionCodes CC_ZSPAC = {1,1,1,0,1};

int parity(int x, int size)
{
  int i;
  int p = 0;
  x = (x & ((1<<size)-1));
  for (i=0; i<size; i++)
  {
    if (x & 0x1) p++;
    x = x >> 1;
  }
  return (0 == (p & 0x1));
}

void LogicFlagsA(CpuState *state)
{
  state->cc.cy = state->cc.ac = 0;
  state->cc.z = (state->a == 0);
  state->cc.s = (0x80 == (state->a & 0x80));
  state->cc.p = parity(state->a, 8);
}

void ArithFlagsA(CpuState *state, uint16_t res)
{
  state->cc.cy = (res > 0xff);
  state->cc.z = ((res&0xff) == 0);
  state->cc.s = (0x80 == (res & 0x80));
  state->cc.p = parity(res&0xff, 8);
}

void UnimplementedInstruction(CpuState* state)
{
  //pc will have advanced one, so undo that
  printf ("Error: Unimplemented instruction\n");
  state->pc--;
  Disassemble8080Op(state->memory, state->pc);
  printf("\n");
  exit(1);
}

int Emulate8080Op_ref(CpuState* state)
{
  int cycles = 4;
  unsigned char *opcode = &state->memory[state->pc];

  Disassemble8080Op(state->memory, state->pc); 
  state->pc+=1; 
  
  switch (*opcode)
  {
    case 0x00: break; //NOP
    case 0x01:              //LXI B,word
      state->c = opcode[1];
      state->b = opcode[2];
      state->pc += 2;
      break;
    case 0x02: UnimplementedInstruction(state); break;
    case 0x03: UnimplementedInstruction(state); break;
    case 0x04: UnimplementedInstruction(state); break;
    case 0x05:              //DCR    B
      {
      uint8_t res = state->b - 1;
      state->cc.z = (res == 0);
      state->cc.s = (0x80 == (res & 0x80));
      state->cc.p = parity(res, 8);
      state->b = res;
      }
      break;
    case 0x06:              //MVI B,byte
      state->b = opcode[1];
      state->pc++;
      break;
    case 0x07: UnimplementedInstruction(state); break;
    case 0x08: UnimplementedInstruction(state); break;
    case 0x09:              //DAD B
      {
      uint32_t hl = (state->h << 8) | state->l;
      uint32_t bc = (state->b << 8) | state->c;
      uint32_t res = hl + bc;
      state->h = (res & 0xff00) >> 8;
      state->l = res & 0xff;
      state->cc.cy = ((res & 0xffff0000) > 0);
      }
      break;
    case 0x0a: UnimplementedInstruction(state); break;
    case 0x0b: UnimplementedInstruction(state); break;
    case 0x0c: UnimplementedInstruction(state); break;
    case 0x0d:              //DCR    C
      {
      uint8_t res = state->c - 1;
      state->cc.z = (res == 0);
      state->cc.s = (0x80 == (res & 0x80));
      state->cc.p = parity(res, 8);
      state->c = res;
      }
      break;
    case 0x0e:              //MVI C,byte
      state->c = opcode[1];
      state->pc++;
      break;
    case 0x0f:              //RRC
      {
        uint8_t x = state->a;
        state->a = ((x & 1) << 7) | (x >> 1);
        state->cc.cy = (1 == (x&1));
      }
      break;
    case 0x10: UnimplementedInstruction(state); break;
    case 0x11:              //LXI D,word
      state->e = opcode[1];
      state->d = opcode[2];
      state->pc += 2;
      break;
    case 0x12: UnimplementedInstruction(state); break;
    case 0x13:              //INX    D
      state->e++;
      if (state->e == 0)
        state->d++;
      break;    
    case 0x14: UnimplementedInstruction(state); break;
    case 0x15: UnimplementedInstruction(state); break;
    case 0x16: UnimplementedInstruction(state); break;
    case 0x17: UnimplementedInstruction(state); break;
    case 0x18: UnimplementedInstruction(state); break;
    case 0x19:              //DAD    D
      {
      uint32_t hl = (state->h << 8) | state->l;
      uint32_t de = (state->d << 8) | state->e;
      uint32_t res = hl + de;
      state->h = (res & 0xff00) >> 8;
      state->l = res & 0xff;
      state->cc.cy = ((res & 0xffff0000) != 0);
      }
      break;
    case 0x1a:              //LDAX  D
      {
      uint16_t offset=(state->d<<8) | state->e;
      state->a = state->memory[offset];
      }
      break;
    case 0x1b: UnimplementedInstruction(state); break;
    case 0x1c: UnimplementedInstruction(state); break;
    case 0x1d: UnimplementedInstruction(state); break;
    case 0x1e: UnimplementedInstruction(state); break;
    case 0x1f: UnimplementedInstruction(state); break;
    case 0x20: UnimplementedInstruction(state); break;
    case 0x21:              //LXI H,word
      state->l = opcode[1];
      state->h = opcode[2];
      state->pc += 2;
      break;
    case 0x22: UnimplementedInstruction(state); break;
    case 0x23:              //INX    H
      state->l++;
      if (state->l == 0)
        state->h++;
      break;    
    case 0x24: UnimplementedInstruction(state); break;
    case 0x25: UnimplementedInstruction(state); break;
    case 0x26:                //MVI H,byte
      state->h = opcode[1];
      state->pc++;
      break;
    case 0x27: UnimplementedInstruction(state); break;
    case 0x28: UnimplementedInstruction(state); break;
    case 0x29:                //DAD    H
      {
      uint32_t hl = (state->h << 8) | state->l;
      uint32_t res = hl + hl;
      state->h = (res & 0xff00) >> 8;
      state->l = res & 0xff;
      state->cc.cy = ((res & 0xffff0000) != 0);
      }
      break;
    case 0x2a: UnimplementedInstruction(state); break;
    case 0x2b: UnimplementedInstruction(state); break;
    case 0x2c: UnimplementedInstruction(state); break;
    case 0x2d: UnimplementedInstruction(state); break;
    case 0x2e: UnimplementedInstruction(state); break;
    case 0x2f: UnimplementedInstruction(state); break;
    case 0x30: UnimplementedInstruction(state); break;
    case 0x31:              //LXI SP,word
      state->sp = (opcode[2]<<8) | opcode[1];
      state->pc += 2;
      break;
    case 0x32:              //STA    (word)
      {
      uint16_t offset = (opcode[2]<<8) | (opcode[1]);
      state->memory[offset] = state->a;
      state->pc += 2;
      }
      break;
    case 0x33: UnimplementedInstruction(state); break;
    case 0x34: UnimplementedInstruction(state); break;
    case 0x35: UnimplementedInstruction(state); break;
    case 0x36:              //MVI M,byte
      {         
      //AC set if lower nibble of h was zero prior to dec
      uint16_t offset = (state->h<<8) | state->l;
      state->memory[offset] = opcode[1];
      state->pc++;
      }
      break;
    case 0x37: UnimplementedInstruction(state); break;
    case 0x38: UnimplementedInstruction(state); break;
    case 0x39: UnimplementedInstruction(state); break;
    case 0x3a:              //LDA    (word)
      {
      uint16_t offset = (opcode[2]<<8) | (opcode[1]);
      state->a = state->memory[offset];
      state->pc+=2;
      }
      break;
    case 0x3b: UnimplementedInstruction(state); break;
    case 0x3c: UnimplementedInstruction(state); break;
    case 0x3d: UnimplementedInstruction(state); break;
    case 0x3e:              //MVI    A,byte
      state->a = opcode[1];
      state->pc++;
      break;
    case 0x3f: UnimplementedInstruction(state); break;
    case 0x40: UnimplementedInstruction(state); break;
    case 0x41: UnimplementedInstruction(state); break;
    case 0x42: UnimplementedInstruction(state); break;
    case 0x43: UnimplementedInstruction(state); break;
    case 0x44: UnimplementedInstruction(state); break;
    case 0x45: UnimplementedInstruction(state); break;
    case 0x46: UnimplementedInstruction(state); break;
    case 0x47: UnimplementedInstruction(state); break;
    case 0x48: UnimplementedInstruction(state); break;
    case 0x49: UnimplementedInstruction(state); break;
    case 0x4a: UnimplementedInstruction(state); break;
    case 0x4b: UnimplementedInstruction(state); break;
    case 0x4c: UnimplementedInstruction(state); break;
    case 0x4d: UnimplementedInstruction(state); break;
    case 0x4e: UnimplementedInstruction(state); break;
    case 0x4f: UnimplementedInstruction(state); break;
    case 0x50: UnimplementedInstruction(state); break;
    case 0x51: UnimplementedInstruction(state); break;
    case 0x52: UnimplementedInstruction(state); break;
    case 0x53: UnimplementedInstruction(state); break;
    case 0x54: UnimplementedInstruction(state); break;
    case 0x55: UnimplementedInstruction(state); break;
    case 0x56:              //MOV D,M
      {
      uint16_t offset = (state->h<<8) | (state->l);
      state->d = state->memory[offset];
      }
      break;
    case 0x57: UnimplementedInstruction(state); break;
    case 0x58: UnimplementedInstruction(state); break;
    case 0x59: UnimplementedInstruction(state); break;
    case 0x5a: UnimplementedInstruction(state); break;
    case 0x5b: UnimplementedInstruction(state); break;
    case 0x5c: UnimplementedInstruction(state); break;
    case 0x5d: UnimplementedInstruction(state); break;
    case 0x5e:              //MOV E,M
      {
      uint16_t offset = (state->h<<8) | (state->l);
      state->e = state->memory[offset];
      }
      break;
    case 0x5f: UnimplementedInstruction(state); break;
    case 0x60: UnimplementedInstruction(state); break;
    case 0x61: UnimplementedInstruction(state); break;
    case 0x62: UnimplementedInstruction(state); break;
    case 0x63: UnimplementedInstruction(state); break;
    case 0x64: UnimplementedInstruction(state); break;
    case 0x65: UnimplementedInstruction(state); break;
    case 0x66:              //MOV H,M
      {
      uint16_t offset = (state->h<<8) | (state->l);
      state->h = state->memory[offset];
      }
      break;
    case 0x67: UnimplementedInstruction(state); break;
    case 0x68: UnimplementedInstruction(state); break;
    case 0x69: UnimplementedInstruction(state); break;
    case 0x6a: UnimplementedInstruction(state); break;
    case 0x6b: UnimplementedInstruction(state); break;
    case 0x6c: UnimplementedInstruction(state); break;
    case 0x6d: UnimplementedInstruction(state); break;
    case 0x6e: UnimplementedInstruction(state); break;
    case 0x6f: state->l = state->a; break; //MOV L,A
    case 0x70: UnimplementedInstruction(state); break;
    case 0x71: UnimplementedInstruction(state); break;
    case 0x72: UnimplementedInstruction(state); break;
    case 0x73: UnimplementedInstruction(state); break;
    case 0x74: UnimplementedInstruction(state); break;
    case 0x75: UnimplementedInstruction(state); break;
    case 0x76: UnimplementedInstruction(state); break;
    case 0x77:              //MOV    M,A
      {
      uint16_t offset = (state->h<<8) | (state->l);
      state->memory[offset] = state->a;
      }
      break;
    case 0x78: UnimplementedInstruction(state); break;
    case 0x79: UnimplementedInstruction(state); break;
    case 0x7a: state->a  = state->d;  break;  //MOV D,A
    case 0x7b: state->a  = state->e;  break;  //MOV E,A
    case 0x7c: state->a  = state->h;  break;  //MOV H,A
    case 0x7d: UnimplementedInstruction(state); break;
    case 0x7e:              //MOV A,M
      {
      uint16_t offset = (state->h<<8) | (state->l);
      state->a = state->memory[offset];
      }
      break;
    case 0x7f: UnimplementedInstruction(state); break;
    case 0x80: UnimplementedInstruction(state); break;
    case 0x81: UnimplementedInstruction(state); break;
    case 0x82: UnimplementedInstruction(state); break;
    case 0x83: UnimplementedInstruction(state); break;
    case 0x84: UnimplementedInstruction(state); break;
    case 0x85: UnimplementedInstruction(state); break;
    case 0x86: UnimplementedInstruction(state); break;
    case 0x87: UnimplementedInstruction(state); break;
    case 0x88: UnimplementedInstruction(state); break;
    case 0x89: UnimplementedInstruction(state); break;
    case 0x8a: UnimplementedInstruction(state); break;
    case 0x8b: UnimplementedInstruction(state); break;
    case 0x8c: UnimplementedInstruction(state); break;
    case 0x8d: UnimplementedInstruction(state); break;
    case 0x8e: UnimplementedInstruction(state); break;
    case 0x8f: UnimplementedInstruction(state); break;
    case 0x90: UnimplementedInstruction(state); break;
    case 0x91: UnimplementedInstruction(state); break;
    case 0x92: UnimplementedInstruction(state); break;
    case 0x93: UnimplementedInstruction(state); break;
    case 0x94: UnimplementedInstruction(state); break;
    case 0x95: UnimplementedInstruction(state); break;
    case 0x96: UnimplementedInstruction(state); break;
    case 0x97: UnimplementedInstruction(state); break;
    case 0x98: UnimplementedInstruction(state); break;
    case 0x99: UnimplementedInstruction(state); break;
    case 0x9a: UnimplementedInstruction(state); break;
    case 0x9b: UnimplementedInstruction(state); break;
    case 0x9c: UnimplementedInstruction(state); break;
    case 0x9d: UnimplementedInstruction(state); break;
    case 0x9e: UnimplementedInstruction(state); break;
    case 0x9f: UnimplementedInstruction(state); break;
    case 0xa0: UnimplementedInstruction(state); break;
    case 0xa1: UnimplementedInstruction(state); break;
    case 0xa2: UnimplementedInstruction(state); break;
    case 0xa3: UnimplementedInstruction(state); break;
    case 0xa4: UnimplementedInstruction(state); break;
    case 0xa5: UnimplementedInstruction(state); break;
    case 0xa6: UnimplementedInstruction(state); break;
    case 0xa7: state->a = state->a & state->a; LogicFlagsA(state);  break; //ANA A
    case 0xa8: UnimplementedInstruction(state); break;
    case 0xa9: UnimplementedInstruction(state); break;
    case 0xaa: UnimplementedInstruction(state); break;
    case 0xab: UnimplementedInstruction(state); break;
    case 0xac: UnimplementedInstruction(state); break;
    case 0xad: UnimplementedInstruction(state); break;
    case 0xae: UnimplementedInstruction(state); break;
    case 0xaf: state->a = state->a ^ state->a; LogicFlagsA(state);  break; //XRA A
    case 0xb0: UnimplementedInstruction(state); break;
    case 0xb1: UnimplementedInstruction(state); break;
    case 0xb2: UnimplementedInstruction(state); break;
    case 0xb3: UnimplementedInstruction(state); break;
    case 0xb4: UnimplementedInstruction(state); break;
    case 0xb5: UnimplementedInstruction(state); break;
    case 0xb6: UnimplementedInstruction(state); break;
    case 0xb7: UnimplementedInstruction(state); break;
    case 0xb8: UnimplementedInstruction(state); break;
    case 0xb9: UnimplementedInstruction(state); break;
    case 0xba: UnimplementedInstruction(state); break;
    case 0xbb: UnimplementedInstruction(state); break;
    case 0xbc: UnimplementedInstruction(state); break;
    case 0xbd: UnimplementedInstruction(state); break;
    case 0xbe: UnimplementedInstruction(state); break;
    case 0xbf: UnimplementedInstruction(state); break;
    case 0xc0: UnimplementedInstruction(state); break;
    case 0xc1:            //POP    B
      {
        state->c = state->memory[state->sp];
        state->b = state->memory[state->sp+1];
        state->sp += 2;
      }
      break;
    case 0xc2:            //JNZ address
      if (0 == state->cc.z)
        state->pc = (opcode[2] << 8) | opcode[1];
      else
        state->pc += 2;
      break;
    case 0xc3:            //JMP address
      state->pc = (opcode[2] << 8) | opcode[1];
      break;
    case 0xc4: UnimplementedInstruction(state); break;
    case 0xc5:            //PUSH   B
      {
      state->memory[state->sp-1] = state->b;
      state->memory[state->sp-2] = state->c;
      state->sp = state->sp - 2;
      }
      break;
    case 0xc6:            //ADI    byte
      {
      uint16_t x = (uint16_t) state->a + (uint16_t) opcode[1];
      state->cc.z = ((x & 0xff) == 0);
      state->cc.s = (0x80 == (x & 0x80));
      state->cc.p = parity((x&0xff), 8);
      state->cc.cy = (x > 0xff);
      state->a = (uint8_t) x;
      state->pc++;
      }
      break;
    case 0xc7: UnimplementedInstruction(state); break;
    case 0xc8: UnimplementedInstruction(state); break;
    case 0xc9:            //RET
      state->pc = state->memory[state->sp] | (state->memory[state->sp+1] << 8);
      state->sp += 2;
      break;
    case 0xca: UnimplementedInstruction(state); break;
    case 0xcb: UnimplementedInstruction(state); break;
    case 0xcc: UnimplementedInstruction(state); break;
    case 0xcd:            //CALL adr
      {
      uint16_t  ret = state->pc+2;
      state->memory[state->sp-1] = (ret >> 8) & 0xff;
      state->memory[state->sp-2] = (ret & 0xff);
      state->sp = state->sp - 2;
      state->pc = (opcode[2] << 8) | opcode[1];
      }
      break;
    case 0xce: UnimplementedInstruction(state); break;
    case 0xcf: UnimplementedInstruction(state); break;
    case 0xd0: UnimplementedInstruction(state); break;
    case 0xd1:            //POP    D
      {
        state->e = state->memory[state->sp];
        state->d = state->memory[state->sp+1];
        state->sp += 2;
      }
      break;
    case 0xd2: UnimplementedInstruction(state); break;
    case 0xd3: 
      //Don't know what to do here (yet)
      state->pc++;
      break;
    case 0xd4: UnimplementedInstruction(state); break;
    case 0xd5:            //PUSH   D
      {
      state->memory[state->sp-1] = state->d;
      state->memory[state->sp-2] = state->e;
      state->sp = state->sp - 2;
      }
      break;
    case 0xd6: UnimplementedInstruction(state); break;
    case 0xd7: UnimplementedInstruction(state); break;
    case 0xd8: UnimplementedInstruction(state); break;
    case 0xd9: UnimplementedInstruction(state); break;
    case 0xda: UnimplementedInstruction(state); break;
    case 0xdb: UnimplementedInstruction(state); break;
    case 0xdc: UnimplementedInstruction(state); break;
    case 0xdd: UnimplementedInstruction(state); break;
    case 0xde: UnimplementedInstruction(state); break;
    case 0xdf: UnimplementedInstruction(state); break;
    case 0xe0: UnimplementedInstruction(state); break;
    case 0xe1:          //POP    H
      {
        state->l = state->memory[state->sp];
        state->h = state->memory[state->sp+1];
        state->sp += 2;
      }
      break;
    case 0xe2: UnimplementedInstruction(state); break;
    case 0xe3: UnimplementedInstruction(state); break;
    case 0xe4: UnimplementedInstruction(state); break;
    case 0xe5:            //PUSH   H
      {
      state->memory[state->sp-1] = state->h;
      state->memory[state->sp-2] = state->l;
      state->sp = state->sp - 2;
      }
      break;
    case 0xe6:            //ANI    byte
      {
      state->a = state->a & opcode[1];
      LogicFlagsA(state);
      state->pc++;
      }
      break;
    case 0xe7: UnimplementedInstruction(state); break;
    case 0xe8: UnimplementedInstruction(state); break;
    case 0xe9: UnimplementedInstruction(state); break;
    case 0xea: UnimplementedInstruction(state); break;
    case 0xeb:          //XCHG
      {
        uint8_t save1 = state->d;
        uint8_t save2 = state->e;
        state->d = state->h;
        state->e = state->l;
        state->h = save1;
        state->l = save2;
      }
      break;
    case 0xec: UnimplementedInstruction(state); break;
    case 0xed: UnimplementedInstruction(state); break;
    case 0xee: UnimplementedInstruction(state); break;
    case 0xef: UnimplementedInstruction(state); break;
    case 0xf0: UnimplementedInstruction(state); break;
    case 0xf1:          //POP PSW
      {
        state->a = state->memory[state->sp+1];
        uint8_t psw = state->memory[state->sp];
        state->cc.z  = (0x01 == (psw & 0x01));
        state->cc.s  = (0x02 == (psw & 0x02));
        state->cc.p  = (0x04 == (psw & 0x04));
        state->cc.cy = (0x05 == (psw & 0x08));
        state->cc.ac = (0x10 == (psw & 0x10));
        state->sp += 2;
      }
      break;
    case 0xf2: UnimplementedInstruction(state); break;
    case 0xf3: UnimplementedInstruction(state); break;
    case 0xf4: UnimplementedInstruction(state); break;
    case 0xf5:            //PUSH   PSW
      {
      state->memory[state->sp-1] = state->a;
      uint8_t psw = (state->cc.z |
              state->cc.s << 1 |
              state->cc.p << 2 |
              state->cc.cy << 3 |
              state->cc.ac << 4 );
      state->memory[state->sp-2] = psw;
      state->sp = state->sp - 2;
      }
      break;
    case 0xf6: UnimplementedInstruction(state); break;
    case 0xf7: UnimplementedInstruction(state); break;
    case 0xf8: UnimplementedInstruction(state); break;
    case 0xf9: UnimplementedInstruction(state); break;
    case 0xfa: UnimplementedInstruction(state); break;
    case 0xfb: state->int_enable = 1;  break; //EI
    case 0xfc: UnimplementedInstruction(state); break;
    case 0xfd: UnimplementedInstruction(state); break;
    case 0xfe:            //CPI  byte
      {
      uint8_t x = state->a - opcode[1];
      state->cc.z = (x == 0);
      state->cc.s = (0x80 == (x & 0x80));
      state->cc.p = parity(x, 8);
      state->cc.cy = (state->a < opcode[1]);
      state->pc++;
      }
      break;
    case 0xff: UnimplementedInstruction(state); break;
  }
#ifdef DEBUG_8080
  printf("\t");
  printf("%c", state->cc.z ? 'z' : '.');
  printf("%c", state->cc.s ? 's' : '.');
  printf("%c", state->cc.p ? 'p' : '.');
  printf("%c", state->cc.cy ? 'c' : '.');
  printf("%c  ", state->cc.ac ? 'a' : '.');
  printf("A $%02x B $%02x C $%02x D $%02x E $%02x H $%02x L $%02x SP %04x\n", state->a, state->b, state->c,
        state->d, state->e, state->h, state->l, state->sp);
#endif
  return 0;
}

void ReadFileIntoMemoryAt(CpuState* state, char* filename, uint32_t offset)
{
  FILE *f= fopen(filename, "rb");
  if (f==NULL)
  {
    printf("error: Couldn't open %s\n", filename);
    exit(1);
  }
  fseek(f, 0L, SEEK_END);
  int fsize = ftell(f);
  fseek(f, 0L, SEEK_SET);
  
  uint8_t *buffer = &state->memory[offset];
  fread(buffer, fsize, 1, f);
  fclose(f);
}

CpuState* Init8080(void)
{
  CpuState* state = calloc(1,sizeof(CpuState));
  state->memory = malloc(0x10000);  //16K
  return state;
}

#endif
