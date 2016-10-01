sources = emulator_ref.c emulator.c disassembler.c main_emulator.c
objects =  emulator_ref.o emulator.o disassembler.o main_emulator.o

all : emulator emulator_ref emulator_test

CFLAGS = -Wall
emulator_ref : CFLAGS += -DDBG_REF
emulator_test : CFLAGS += -DDBG_TEST

emulator : $(objects)
	cc -o emulator $(objects)

emulator_ref : $(sources)
	cc -o emulator_ref $(CFLAGS) $(sources)

emulator_test : $(sources)
	cc -o emulator_test $(CFLAGS) $(sources)

$(objects) : emulator.h
$(objects) : intel8080_opcodes.h
disassembler.o emulator_ref.o : disassembler.h

clean :
	-rm -f emulator emulator_ref emulator_test $(objects)

.PHONY : clean all
