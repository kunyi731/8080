objects = main_emulator.o emulator.o emulator_ref.o disassembler.o

emulator : $(objects)
	cc -o emulator $(objects)

$(objects) : intel8080_opcodes.h
$(objects) : emulator.h
disassembler.o emulator_ref.o : disassembler.h

.PHONY : clean
clean :
	-rm emulator $(objects)
