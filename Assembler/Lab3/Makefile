assembler: assembler.o
	g++ assembler.o -o riscv_asm

assembler.o: assembler.cpp
	g++ -c assembler.cpp

clean: rm *.o riscv_asm