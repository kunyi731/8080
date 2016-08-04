#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "emulator.h"
#include "disassembler.h"

/* Utility functions */
static inline uint16_t get_offset(State8080 *state)
{
  return (state->h << 8) | (state->l);
}


static inline uint8_t Parity(uint16_t x)
{
  int p = 0;
  while (x)
  {
    // XOR the last bit of x
    p ^= (x & 0x1);

    //clear the bit
    x >>= 1;
  }
  return (1 - p);
}

/* ARITHMETIC: add number x to register a, and set condition codes */
static void add(State8080 *state,  uint8_t x)
{
  uint16_t answer = (uint16_t) state->a + (uint16_t) x;
  /* Answer is zero? */
  state->cc.z = ((answer & 0xff) == 0);
  /* Answer MSB is 1? */
  state->cc.s = (answer & 0x80) ? 1 : 0;
  /* Answer BIT 16 is 1? */
  state->cc.cy = (answer > 0xff);
  /* Answer parity*/
  state->cc.p = Parity(answer&0xff);
  /* Store answer */
  state->a = answer & 0xff;
}

/* ARITHMETIC: add number x to register a, with carry, and set condition codes */
static void adc(State8080 *state, int8_t x, uint8_t carry)
{
  uint16_t answer = (uint16_t) state->a + (uint16_t) x + (uint16_t) carry;
  /* Answer is zero? */
  state->cc.z = ((answer & 0xff) == 0);
  /* Answer MSB is 1? */
  state->cc.s = (answer & 0x80) ? 1 : 0;
  /* Answer BIT 16 is 1? */
  state->cc.cy = (answer > 0xff);
  /* Answer parity*/
  state->cc.p = Parity(answer&0xff);
  /* Store answer */
  state->a = answer & 0xff;
}

/* ARITHMETIC: subtract number x from register a, and set condition codes */
static void sub(State8080 *state, int8_t x, uint8_t carry)
{
  uint16_t answer = (uint16_t) state->a - (uint16_t) x - (uint16_t) carry;
  /* Answer is zero? */
  state->cc.z = ((answer & 0xff) == 0);
  /* Answer MSB is 1? */
  state->cc.s = (answer & 0x80)? 1 : 0;
  /* Answer BIT 16 is 1? */
  state->cc.cy = (answer > 0xff);
  /* Answer parity*/
  state->cc.p = Parity(answer&0xff);
  /* Store answer */
  state->a = answer & 0xff;
}


/* ARITHMETIC: increment register, don't modify CY */
static uint8_t inc(State8080 *state, uint8_t x)
{
  uint16_t answer = (uint16_t) x + 1;
  /* Answer is zero? */
  state->cc.z = ((answer & 0xff) == 0);
  /* Answer MSB is 1? */
  state->cc.s = (answer & 0x80) ? 1 : 0;
  /* Answer parity*/
  state->cc.p = Parity(answer & 0xff);
  
  return (uint8_t) (answer & 0xff);
}

/* ARITHMETIC: decrement register, don't modify CY */
static uint8_t dcr(State8080 *state, uint8_t x)
{
  uint16_t answer = (uint16_t) x - 1;

  //printf("%x answer %x\n", answer, (answer & 0x80));
#if 0 
  printf(" parity 1 2 3 4: %d, %d, %d, %d\n",
        Parity(1),
        Parity(2),
        Parity(3),
        Parity(4)
        );
#endif

  /* Answer is zero? */
  state->cc.z = ((answer & 0xff) == 0);
  /* Answer MSB is 1? */
  state->cc.s = (answer & 0x80) ? 1 : 0;
  /* Answer parity*/
  state->cc.p = Parity(answer & 0xff);
  
  return (uint8_t) (answer & 0xff);
}

void print_state(State8080 *state)
{
  printf("\t");
  printf("%c", state->cc.z ? 'z' : '.');
  printf("%c", state->cc.s ? 's' : '.');
  printf("%c", state->cc.p ? 'p' : '.');
  printf("%c", state->cc.cy ? 'c' : '.');
  printf("%c  ", state->cc.ac ? 'a' : '.');
  printf("A $%02x B $%02x C $%02x D $%02x E $%02x H $%02x L $%02x SP %04x  ", state->a, state->b, state->c,
        state->d, state->e, state->h, state->l, state->sp);

  printf("PC $%04x\n", state->pc);
}

int compare_states(State8080* state1, State8080* state2)
{
  int equal = (state1->a == state2->a) &&
         (state1->b == state2->b) &&
         (state1->c == state2->c) &&
         (state1->d == state2->d) &&
         (state1->e == state2->e) &&
         (state1->h == state2->h) &&
         (state1->l == state2->l) &&
         (state1->sp == state2->sp) &&
         (state1->pc == state2->pc) &&
         (state1->cc.z == state2->cc.z) &&
         (state1->cc.s == state2->cc.s) &&
         (state1->cc.p == state2->cc.p) &&
         (state1->cc.cy == state2->cc.cy);

  if (!equal)
  {
    printf("States not equal!\n");
    print_state(state1);
    print_state(state2);
  }
  return equal;
}

#define BIT(n) (1 << (n))
#define BITMASK(m) (BIT(m) - 1)

//get m bits starting from n to (m+n-1) in number x
#define GET_BITS(x, m, n) ( (x & (BITMASK(m) << n)) >> n)

//get pointer of register based on bit pattern
inline uint8_t *GET_PTR(uint8_t pattern, State8080 *state_ptr)
{
  if (pattern == 0x0)
  {
    return &state_ptr->b;
  }
  else if (pattern == 0x1)
  {
    return &state_ptr->c;
  }
  else if (pattern == 0x2)
  {
    return &state_ptr->d;
  }  
  else if (pattern == 0x3)
  {
    return &state_ptr->e;
  }  
  else if (pattern == 0x4)
  {
    return &state_ptr->h;
  }
  else if (pattern == 0x5)
  {
    return &state_ptr->l;
  }
  else if (pattern == 0x6)
  {     
    //addend is byte pointed by address in HL register pair
    uint16_t offset = (state_ptr->h << 8) | (state_ptr->l);
    return &state_ptr->memory[offset]; 
  }
  else if (pattern == 0x7)
  {
    return &state_ptr->a;
  }
}

/* Compare two 8bit numbers and set flags accordingly */
void cmp(uint8_t a, uint8_t b, State8080 *state)
{
  uint16_t answer = (uint16_t) a - (uint16_t) b;
  /* Answer is zero? */
  state->cc.z = ((answer & 0xff) == 0);
  /* Answer MSB is 1? */
  state->cc.s = (answer & 0x80)? 1 : 0;
  /* Answer BIT 16 is 1? */
  state->cc.cy = (answer > 0xff);
  /* Answer parity*/
  state->cc.p = Parity(answer&0xff);
}
   
/* Set state flags based on 8-bit input x*/
void set_flags(State8080 *state, uint8_t x)
{
  state->cc.z = (x == 0);
  state->cc.s = (x & 0x80)? 1 : 0;
  state->cc.cy = 0;
  state->cc.p = Parity(x);
}

/* Bitwise AND of two 8bit numbers */
uint8_t ana(uint8_t a, uint8_t b)
{
  return (a & b);
}

/* Bitwise XOR */
uint8_t xor(uint8_t a, uint8_t b)
{
  return (a ^ b);
}

/* Bitwise OR */
uint8_t ora(uint8_t a, uint8_t b)
{
  return (a | b);
}

/* Rotate left */
uint8_t rlc(State8080 *state)
{
  state->cc.cy = GET_BITS(state->a, 1, 7);
  return (GET_BITS(state->a, 1, 7) | (GET_BITS(state->a, 7, 0) << 1) );
}

/* Rotate right */
uint8_t rrc(State8080 *state)
{
  state->cc.cy = GET_BITS(state->a, 1, 0);
  return (GET_BITS(state->a, 7, 1) >> 1) | GET_BITS(state->a, 1, 0);
}

/* Rotate left through carry */
uint8_t ral(State8080 *state)
{
  uint8_t temp = (GET_BITS(state->a, 7, 0) << 1) | state->cc.cy;
  state->cc.cy = GET_BITS(state->a, 1, 7);
  return temp;
}

/* Rotate right through carry */
uint8_t rar(State8080 *state)
{
  uint8_t temp = (GET_BITS(state->a, 7, 1) >> 1) | (state->cc.cy << 7);
  state->cc.cy = GET_BITS(state->a, 1, 0);
  return temp;
}

/* Exchange the value pointed to by two pointers */
void exchange(uint8_t *a_ptr, uint8_t *b_ptr)
{
  uint8_t temp = *a_ptr;
  *a_ptr = *b_ptr;
  *b_ptr = temp;
}

int Emulate8080Op(State8080* state)
{
  unsigned char* opcode = &state->memory[state->pc];

  //printf("   %x\n", *opcode & 0xcf);

  uint16_t offset, ccc, condition;
  
  state->pc++;
  
  /* Process opcode */
  
  /* Logical group */
  if ((*opcode & 0xf8) == 0xa0) //ANA
  {
    uint8_t sss = GET_BITS(*opcode, 3, 0);
    uint8_t *b;
    b = GET_PTR(sss, state);

    state->a = ana(state->a, *b);
    set_flags(state, state->a);
    return;
  }

  if (*opcode == 0xe6) //ANI imm
  {
    state->a = ana(state->a, opcode[1]);
    set_flags(state, state->a);
    state->pc++;
    return;
  }

  if ((*opcode & 0xf8) == 0xa8) //XOR
  {
    uint8_t sss = GET_BITS(*opcode, 3, 0);
    uint8_t *b;
    b = GET_PTR(sss, state);
    state->a = xor(state->a, *b);
    set_flags(state, state->a);
    return;
  }

  if (*opcode == 0xee) //XRI imm
  {
    state->a = xor(state->a, opcode[1]);
    set_flags(state, state->a);
    state->pc++;
    return;
  }

  if ((*opcode & 0xf8) == 0xb0) //ORA 
  {
    uint8_t sss = GET_BITS(*opcode, 3, 0);
    uint8_t *b = GET_PTR(sss, state);

    state->a = ora(state->a, *b);
    set_flags(state, state->a);
    return;
  }

  if (*opcode == 0xf6) //ORI imm
  {
    state->a = ora(state->a, opcode[1]);
    set_flags(state, state->a);
    state->pc++;
    return;
  }

  if (*opcode == 0x07) //RLC
  {
    //Rotate left
    state->a = rlc(state);
    return;
  }

  if (*opcode == 0x0f) //RRC
  {
    //Rotate right
    state->a = rrc(state);
    return;
  }

  if (*opcode == 0x17) //RAL
  {
    //Rotate left through carry
    state->a = ral(state);
    return;
  }

  if (*opcode == 0x1f) //RAR
  {
    //Rotate right through carry
    state->a = rar(state);
    return;
  }

  if (*opcode == 0x2f) //CMA
  {
    state->a = 0xff - state->a;
    return;
  }

  if (*opcode == 0x3f) //CMF
  {
    state->cc.cy = 1 - state->cc.cy;
    return;
  }

  if (*opcode == 0x37) //STC
  {
    state->cc.cy = 1;
    return;
  }

  /* Branch group*/
  if (*opcode == 0xe9) //PCHL
  {
    state->pc = state->h << 8 | state->l;
    return;
  }

  /* Data transfer group */
  if ((*opcode & 0xc0) == 0x40) //MOV
  {
    uint8_t ddd = GET_BITS(*opcode, 3, 3);
    uint8_t sss = GET_BITS(*opcode, 3, 0);
    uint8_t *ptr_dest, *ptr_src;
    ptr_dest = GET_PTR(ddd, state);
    ptr_src  = GET_PTR(sss, state);
    *ptr_dest = *ptr_src;
    return;
  }

  if ((*opcode & 0xc7) == 0x6) //MVI
  {
    uint8_t ddd = GET_BITS(*opcode, 3, 3);
    uint8_t *ptr_dest = GET_PTR(ddd, state);
    *ptr_dest = opcode[1];

    state->pc++;
    return;
  }

  if ((*opcode & 0xcf) == 0x1) //LXI
  {
    uint8_t rp = GET_BITS(*opcode, 2, 4);

    uint8_t *ptr_h, *ptr_l;
    if (rp == 0x0)
    {
      ptr_h = &state->b;
      ptr_l = &state->c;
    }
    else if (rp == 0x1)
    {
      ptr_h = &state->d;
      ptr_l = &state->e;
    }
    else if (rp == 0x2)
    {
      ptr_h = &state->h;
      ptr_l = &state->l;
    }
    else if (rp == 0x3)
    {
      state->sp = (opcode[2] << 8) | opcode[1];
      state->pc += 2;
      return;
    }
    *ptr_h = opcode[2];
    *ptr_l = opcode[1];

    state->pc += 2;
    return;
  }

  //puts(" got to switch");
  
  if ((*opcode == 0xa) || (*opcode == 0x1a)) //LDAX
  {
    uint8_t rp = GET_BITS(*opcode, 2, 4);
    uint8_t *ptr_h, *ptr_l;
    if (rp == 0x0)
    {
      ptr_h = &state->b;
      ptr_l = &state->c;
    }
    else if (rp == 0x1)
    {
      ptr_h = &state->d;
      ptr_l = &state->e;
    }
    else
    {
      puts("Unspecified LDAX usage!!");
    }

    unsigned int addr = (((*ptr_h) << 8) | *ptr_l);
    state->a = state->memory[addr];
    return;
  }

  

  if ((*opcode & 0xcf) == 0x3) //INX
  {
    uint8_t rp = GET_BITS(*opcode, 2, 4);
    
    if (rp == 0x0)
    {
      state->c++;
      if (state->c == 0) state->b++;
    }
    else if (rp == 0x1)
    {
      state->e++;
      if (state->e == 0) state->d++;
    }
    else if (rp == 0x2)
    {
      state->l++;
      if (state->l == 0) state->h++;
    }
    else if (rp == 0x3)
    {
      state->sp++;
    }

    return;
  }

  if ((*opcode & 0xcf) == 0x9) //DAD
  {
    uint8_t rp = GET_BITS(*opcode, 2, 4);
    UWord16 addend_1, addend_2;
    uint32_t sum;
    
    if (rp == 0x0)
    {
      addend_1.h = state->b;
      addend_1.l = state->c;
    }
    else if (rp == 0x1)
    {
      addend_1.h = state->d;
      addend_1.l = state->e;
    }
    else if (rp == 0x2)
    {
      addend_1.h = state->h;
      addend_1.l = state->l;
    }
    else if (rp == 0x3)
    {
      addend_1.value = state->sp;
    }

    addend_2.h = state->h;
    addend_2.l = state->l;
    
    sum = (uint32_t)addend_1.value + (uint32_t)addend_2.value;

    state->cc.cy = (sum & 0xffff0000) ? 1 : 0;
    
    state->h = (sum & 0xff00) >> 8;
    state->l = sum & 0xff;

    return;
  }

  if (*opcode == 0xfe) //CPI
  {
    cmp(state->a, opcode[1], state);
    state->pc++;
    return;
  }

  if (*opcode == 0xeb) //XCHG
  {
    exchange(&state->h, &state->d);
    exchange(&state->l, &state->e);
    return;
  }
  
  if (*opcode == 0x32) //STA
  {
    uint16_t offset = opcode[1] | (opcode[2] << 8);
    state->memory[offset] = state->a;
    state->pc += 2;
    return;
  }

  if (*opcode == 0x3a) //LDA addr
  {
    uint16_t offset = opcode[1] | (opcode[2] << 8);
    state->a = state->memory[offset];
    state->pc += 2;
    return;
   }

  /* Stack group */
  if ((*opcode & 0xcf) == 0xc1)                      //POP
  {    
   
    uint8_t rp = GET_BITS(*opcode, 2, 4);
    
    if (rp == 0x0)
    {
      state->c = state->memory[state->sp];    
      state->b = state->memory[state->sp+1];    
    }
    else if (rp == 0x1)
    {
      state->e = state->memory[state->sp];    
      state->d = state->memory[state->sp+1];    
    }
    else if (rp == 0x2)
    {
      state->l = state->memory[state->sp];    
      state->h = state->memory[state->sp+1];    
    }

    state->sp += 2;
    return; 
  }    

  if ((*opcode & 0xcf) == 0xc5)                      //PUSH  
  {

    uint8_t rp = GET_BITS(*opcode, 2, 4);
    
    if (rp == 0x0)
    {
      state->memory[state->sp - 1] = state->b;    
      state->memory[state->sp - 2] = state->c;    
    }
    else if (rp == 0x1)
    {
      state->memory[state->sp - 1] = state->d;    
      state->memory[state->sp - 2] = state->e;    
    }
    else if (rp == 0x2)
    {
      state->memory[state->sp - 1] = state->h;    
      state->memory[state->sp - 2] = state->l;    
    }

    state->sp -= 2;
    return; 

  }    

  if (*opcode == 0xf1)                      //POP PSW    
  {    
    state->a = state->memory[state->sp+1];    
    uint8_t psw = state->memory[state->sp];    
    state->cc.z  = (0x01 == (psw & 0x01));    
    state->cc.s  = (0x02 == (psw & 0x02));    
    state->cc.p  = (0x04 == (psw & 0x04));    
    state->cc.cy = (0x05 == (psw & 0x08));    
    state->cc.ac = (0x10 == (psw & 0x10));    
    state->sp += 2;
    return;
  }    

  if (*opcode == 0xf5)                      //PUSH PSW    
  {    
    state->memory[state->sp-1] = state->a;    
    uint8_t psw = (state->cc.z |    
                state->cc.s << 1 |    
                state->cc.p << 2 |    
                state->cc.cy << 3 |    
                state->cc.ac << 4 );    
    state->memory[state->sp-2] = psw;    
    state->sp = state->sp - 2;
    return;
  }    

  if (*opcode == 0xe3) //XTHL
  {
    uint8_t temp = state->memory[state->sp];
   
    state->memory[state->sp] = state->l;
    state->l = temp;

    temp = state->memory[state->sp + 1];
    state->memory[state->sp + 1] = state->h;
    state->h = temp;
    return;
  }

  if (*opcode == 0xf9) //SPHL
  {
    state->sp = (state->h << 8) | state->l;
    return;
  }

  /* I/O group */
  if (*opcode == 0xd3) //OUT
  {
    printf("OUT %d\n", state->a);
    state->pc++;
    return;
  }

  if (*opcode == 0xdb) //IN
  {
    return;
  }

  if (*opcode == 0xfb) //EI 
  {
    //enable interrupts
    printf("EI: enable interrupts\n");
    return;
  }   

  switch(*opcode)
  {
    /*! NOP group */
    case 0x00:
    case 0x08:
    case 0x10:
    case 0x18:
    case 0x28:
    case 0x38:
    case 0xcb:
    case 0xd9:
    case 0xdd:
    case 0xed:
    case 0xfd:
      break;
    
    /*! ARITHEMATIC group */
    case 0x80: //ADD B
      add(state, state->b);
      break;
    case 0x81: //ADD C
      add(state, state->c);
      break;
    case 0x82: //ADD D
      add(state, state->d);
      break;
    case 0x83: //ADD E
      add(state, state->e);
      break;
    case 0x84: //ADD H
      add(state, state->h);
      break;
    case 0x85: //ADD L
      add(state, state->l);
      break;
    case 0x86: //ADD M
      //addend is byte pointed by address in HL register pair
      offset = (state->h << 8) | (state->l);
      add(state, state->memory[offset]);
      break;
    case 0x87: //ADD A
      add(state, state->a);
      break;

    case 0x88: //ADC B
      adc(state, state->b, state->cc.cy);
      break;
    case 0x89: //ADC C
      adc(state, state->c, state->cc.cy);
      break;
    case 0x8a: //ADC D
      adc(state, state->d, state->cc.cy);
      break;
    case 0x8b: //ADC E
      adc(state, state->e, state->cc.cy);
      break;
    case 0x8c: //ADC H
      adc(state, state->h, state->cc.cy);
      break;
    case 0x8d: //ADC L
      adc(state, state->l, state->cc.cy);
      break;
    case 0x8e: //ADC M
      offset = (state->h << 8) | (state->l);
      adc(state, state->memory[offset], state->cc.cy);
      break;
    case 0x8f: //ADC A
      adc(state, state->a, state->cc.cy);
      break;


    case 0x90: //SUB B
      sub(state, state->b, 0);
      break;
    case 0x91: //SUB C
      sub(state, state->c, 0);
      break;
    case 0x92: //SUB D
      sub(state, state->d, 0);
      break;
    case 0x93: //SUB E
      sub(state, state->e, 0);
      break;
    case 0x94: //SUB H
      sub(state, state->h, 0);
      break;
    case 0x95: //SUB L
      sub(state, state->l, 0);
      break;
    case 0x96: //SUB M
      sub(state, state->memory[get_offset(state)], 0);
      break;
    case 0x97: //SUB A
      sub(state, state->a, 0);
      break;
   

    case 0x98: //SBB B
      sub(state, state->b, state->cc.cy);
      break;
    case 0x99: //SBB C
      sub(state, state->c, state->cc.cy);
      break;
    case 0x9a: //SBB D
      sub(state, state->d, state->cc.cy);
      break;
    case 0x9b: //SBB E
      sub(state, state->e, state->cc.cy);
      break;
    case 0x9c: //SBB H
      sub(state, state->h, state->cc.cy);
      break;
    case 0x9d: //SBB L
      sub(state, state->l, state->cc.cy);
      break;
    case 0x9e: //SBB M
      sub(state, state->memory[get_offset(state)], state->cc.cy);
      break;
    case 0x9f: //SBB A
      sub(state, state->a, state->cc.cy);
      break;
   


      


    case 0xc6: //ADI D8
      add(state, opcode[1]);
      state->pc++;
      break;

    case 0xce: //ACI D8
      adc(state, opcode[1], state->cc.cy);
      state->pc++;
      break;

    case 0xd6: //SUI D8
      sub(state, opcode[1], 0);
      state->pc++;
      break;

    case 0xde: //SBI D8
      sub(state, opcode[1], state->cc.cy);
      state->pc++;
      break;

    case 0x04: //INC B
      state->b = inc(state, state->b);
      break;

    case 0x0c: //INC C
      state->c = inc(state, state->c);
      break;
    
    case 0x14: //INC D
      state->d = inc(state, state->d);
      break;

    case 0x1c: //INC E
      state->e = inc(state, state->e);
      break;

    case 0x24: //INC H
      state->h = inc(state, state->h);
      break;

    case 0x2c: //INC L
      state->l = inc(state, state->l);
      break;

    case 0x34: //INC M
      {
      uint16_t offset = (state->h << 8) | state->l;
      state->memory[offset] = inc(state, state->memory[offset]);
      }
      break;

    case 0x3c: //INC A
      state->a = inc(state, state->a);
      break;



    case 0x05: //DCR B
      state->b = dcr(state, state->b);
      break;

    case 0x0d: //DCR C
      state->c = dcr(state, state->c);
      break;
    
    case 0x15: //DCR D
      state->d = dcr(state, state->d);
      break;

    case 0x1d: //DCR E
      state->e = dcr(state, state->e);
      break;

    case 0x25: //DCR H
      state->h = dcr(state, state->h);
      break;

    case 0x2d: //DCR L
      state->l = dcr(state, state->l);
      break;

    case 0x35: //DCR M
      {
      uint16_t offset = (state->h << 8) | state->l;
      state->memory[offset] = dcr(state, state->memory[offset]);
      }
      break;

    case 0x3d: //DCR A
      state->a = dcr(state, state->a);
      break;


    /* Branch group */
    case 0xc3: //JMP ADDR
      state->pc = (opcode[2] << 8) | opcode[1];
      break;
   
    case 0xc2: //JNZ ADDR
    case 0xca: //JZ  ADDR
    case 0xd2: //JNC ADDR
    case 0xda: //JC  ADDR
    case 0xe2: //JPO ADDR
    case 0xea: //JPE ADDR
    case 0xf2: //JP  ADDR
    case 0xfa: //JM  ADDR
      ccc = (opcode[0] & 0x38) >> 3; //0b111000
      
      if (ccc == 0)
        condition = (state->cc.z == 0);
      else if (ccc == 1)
        condition = (state->cc.z == 1);
      else if (ccc == 2)
        condition = (state->cc.cy == 0);
      else if (ccc == 3)
        condition = (state->cc.cy == 1);
      else if (ccc == 4)
        condition = (state->cc.p == 0);
      else if (ccc == 5)
        condition = (state->cc.p == 1);
      else if (ccc == 6)
        condition = (state->cc.s == 0);
      else if (ccc == 7)
        condition = (state->cc.s == 1);

      if (condition)
      {
        state->pc = (opcode[2] << 8) | opcode[1];
      }
      else
      {
        state->pc += 2; 
      }
      break;

    case 0xcd: //CALL ADDR
      {
      uint16_t ret = state->pc + 2;
      state->memory[state->sp - 1] = (ret >> 8) & 0xff;
      state->memory[state->sp - 2] = (ret & 0xff);
      state->sp = state->sp - 2;
      state->pc = (opcode[2] << 8) | opcode[1];
      }
      break;

    case 0xc9: //RET
      state->pc = state->memory[state->sp] | (state->memory[state->sp + 1] << 8);
      state->sp += 2;
      break;

    case 0xc4: 
    case 0xcc:
    case 0xd4:
    case 0xdc:
    case 0xe4:
    case 0xec:
    case 0xf4:
    case 0xfc:
      ccc = (opcode[0] & 0x38) >> 3; //0b111000      

      if (ccc == 0)
        condition = (state->cc.z == 0);
      else if (ccc == 1)
        condition = (state->cc.z == 1);
      else if (ccc == 2)
        condition = (state->cc.cy == 0);
      else if (ccc == 3)
        condition = (state->cc.cy == 1);
      else if (ccc == 4)
        condition = (state->cc.p == 0);
      else if (ccc == 5)
        condition = (state->cc.p == 1);
      else if (ccc == 6)
        condition = (state->cc.s == 0);
      else if (ccc == 7)
        condition = (state->cc.s == 1);

      if (condition)
      {
        uint16_t ret = state->pc + 2;
        state->memory[state->sp - 1] = (ret >> 8) & 0xff;
        state->memory[state->sp - 2] = (ret & 0xff);
        state->sp = state->sp - 2;
        state->pc = (opcode[2] << 8) | opcode[1];
        break;
      }
      else
      {
        state->pc += 2; 
      }
      break;

    case 0xc0:
    case 0xc8:
    case 0xd0:
    case 0xd8:
    case 0xe0:
    case 0xe8:
    case 0xf0:
    case 0xf8:
      ccc = (opcode[0] & 0x38) >> 3; //0b111000      

      if (ccc == 0)
        condition = (state->cc.z == 0);
      else if (ccc == 1)
        condition = (state->cc.z == 1);
      else if (ccc == 2)
        condition = (state->cc.cy == 0);
      else if (ccc == 3)
        condition = (state->cc.cy == 1);
      else if (ccc == 4)
        condition = (state->cc.p == 0);
      else if (ccc == 5)
        condition = (state->cc.p == 1);
      else if (ccc == 6)
        condition = (state->cc.s == 0);
      else if (ccc == 7)
        condition = (state->cc.s == 1);

      if (condition)
      {
        state->pc = state->memory[state->sp] | (state->memory[state->sp + 1] << 8);
        state->pc += 2;
      }
      else
      {
        state->pc += 2; 
      }
      break;

    case 0xc7: //RST
    case 0xcf:
    case 0xd7:
    case 0xdf:
    case 0xe7:
    case 0xef:
    case 0xf7:
    case 0xff:
      {
      uint8_t nnn = (opcode[0] & 0x38) >> 3; //0b111000
      uint16_t ret = state->pc + 2;
      state->memory[state->sp - 1] = (ret >> 8) & 0xff;
      state->memory[state->sp - 2] = (ret & 0xff);
      state->sp = state->sp - 2;
      state->pc = nnn * 8;
      }
      break;
    
    /* Logical group */

    /* Data transfer group */
    case 0x01:
    case 0x11:
    case 0x21:
    case 0x31:

    default:
      UnimplementedInstruction(state);
      break;
  }
}


#ifdef DBG_REF
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

void LogicFlagsA(State8080 *state)
{
  state->cc.cy = state->cc.ac = 0;
  state->cc.z = (state->a == 0);
  state->cc.s = (0x80 == (state->a & 0x80));
  state->cc.p = parity(state->a, 8);
}

void ArithFlagsA(State8080 *state, uint16_t res)
{
  state->cc.cy = (res > 0xff);
  state->cc.z = ((res&0xff) == 0);
  state->cc.s = (0x80 == (res & 0x80));
  state->cc.p = parity(res&0xff, 8);
}

void UnimplementedInstruction(State8080* state)
{
  //pc will have advanced one, so undo that
  printf ("Error: Unimplemented instruction\n");
  state->pc--;
  Disassemble8080Op(state->memory, state->pc);
  printf("\n");
  exit(1);
}

int Emulate8080Op_ref(State8080* state)
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

void ReadFileIntoMemoryAt(State8080* state, char* filename, uint32_t offset)
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

State8080* Init8080(void)
{
  State8080* state = calloc(1,sizeof(State8080));
  state->memory = malloc(0x10000);  //16K
  return state;
}

#endif
